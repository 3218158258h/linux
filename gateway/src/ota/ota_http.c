#include "ota_http.h"
#include "../thirdparty/cJSON/cJSON.h"
#include "../thirdparty/log.c/log.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <curl/curl.h>
#include <openssl/sha.h>

#define BUFFER_SIZE 1024

typedef struct
{
    char *memory;
    size_t size;
} MemoryStruct;

typedef struct
{
    FILE *fp;
    SHA_CTX *sha_ctx;
    size_t total_bytes;
} DownloadContext;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL)
    {
        log_error("Not enough memory");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static size_t WriteFirmwareCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    DownloadContext *ctx = (DownloadContext *)userp;
    
    size_t written = fwrite(contents, 1, realsize, ctx->fp);
    
    if (written > 0) {
        SHA1_Update(ctx->sha_ctx, contents, written);
        ctx->total_bytes += written;
    }
    
    return written;
}

int ota_http_getVersion(Version *version)
{
    CURL *curl;
    CURLcode res;
    MemoryStruct chunk;
    long response_code;

    if (!version) return -1;
    
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, VERSION_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            log_error("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            free(chunk.memory);
            curl_easy_cleanup(curl);
            return -1;
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200)
        {
            log_error("HTTP request failed: %ld", response_code);
            free(chunk.memory);
            curl_easy_cleanup(curl);
            return -1;
        }

        cJSON *json = cJSON_Parse(chunk.memory);
        if (json == NULL)
        {
            log_error("Failed to parse JSON");
            free(chunk.memory);
            curl_easy_cleanup(curl);
            return -1;
        }

        cJSON *major = cJSON_GetObjectItemCaseSensitive(json, "major");
        cJSON *minor = cJSON_GetObjectItemCaseSensitive(json, "minor");
        cJSON *patch = cJSON_GetObjectItemCaseSensitive(json, "patch");

        if (cJSON_IsNumber(major) && cJSON_IsNumber(minor) && cJSON_IsNumber(patch))
        {
            version->major = major->valueint;
            version->minor = minor->valueint;
            version->patch = patch->valueint;
        }
        else
        {
            log_error("Invalid JSON format");
            cJSON_Delete(json);
            free(chunk.memory);
            curl_easy_cleanup(curl);
            return -1;
        }

        cJSON_Delete(json);
        free(chunk.memory);
        curl_easy_cleanup(curl);
        return 0;
    }
    return -1;
}

int ota_http_getFirmware(char *file)
{
    CURL *curl;
    CURLcode res;
    FILE *fp;
    unsigned char hash[SHA_DIGEST_LENGTH];
    SHA_CTX sha_ctx;
    long response_code;
    int ret = -1;

    if (!file) return -1;

    fp = fopen(file, "wb");
    if (!fp)
    {
        log_error("Failed to open file %s for writing", file);
        return -1;
    }

    curl = curl_easy_init();
    if (curl)
    {
        SHA1_Init(&sha_ctx);
        
        DownloadContext ctx = {
            .fp = fp,
            .sha_ctx = &sha_ctx,
            .total_bytes = 0
        };
        
        curl_easy_setopt(curl, CURLOPT_URL, FIRMWARE_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteFirmwareCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &ctx);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            log_error("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            goto cleanup;
        }

        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200)
        {
            log_error("HTTP request failed: %ld", response_code);
            goto cleanup;
        }

        fflush(fp);
        SHA1_Final(hash, &sha_ctx);
        
        log_info("Firmware downloaded: %zu bytes", ctx.total_bytes);
        
        char hash_str[SHA_DIGEST_LENGTH * 2 + 1];
        for (int i = 0; i < SHA_DIGEST_LENGTH; i++) {
            sprintf(hash_str + i * 2, "%02x", hash[i]);
        }
        hash_str[SHA_DIGEST_LENGTH * 2] = '\0';
        log_info("Firmware SHA1: %s", hash_str);
        
        ret = 0;

cleanup:
        fclose(fp);
        curl_easy_cleanup(curl);
        
        if (ret != 0) {
            remove(file);
        }
        
        return ret;
    }

    fclose(fp);
    return -1;
}
