#include <stdio.h>
#include <string.h>
#include <curl/curl.h>
#include <openssl/sha.h>
#include "ota_http.h"
#include "thirdparty/cJSON/cJSON.h"
#include "thirdparty/log.c/log.h"
#include <stdlib.h>

#define BUFFER_SIZE 1024

typedef struct
{
    char *memory;
    size_t size;
} MemoryStruct;

static size_t WriteMemoryCallback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    MemoryStruct *mem = (MemoryStruct *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (ptr == NULL)
    {
        // Out of memory
        log_error("Not enough memory (realloc returned NULL)");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

int ota_http_getVersion(Version *version)
{
    CURL *curl;
    CURLcode res;
    MemoryStruct chunk;
    long response_code;

    chunk.memory = malloc(1); // initial buffer
    chunk.size = 0;           // no data at this point

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, VERSION_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);

        res = curl_easy_perform(curl);

        if (res != CURLE_OK)
        {
            log_error("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            free(chunk.memory);
            curl_easy_cleanup(curl);
            return -1;
        }

        // Check HTTP response code
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200)
        {
            log_error("HTTP request failed with status code: %ld", response_code);
            free(chunk.memory);
            curl_easy_cleanup(curl);
            return -1;
        }

        // Parse JSON response
        cJSON *json = cJSON_Parse(chunk.memory);
        if (json == NULL)
        {
            log_error("Failed to parse JSON");
            free(chunk.memory);
            curl_easy_cleanup(curl);
            return -1;
        }

        // Extract version info
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
    char buffer[BUFFER_SIZE];
    size_t bytes_read;
    long response_code;

    fp = fopen(file, "wb");
    if (!fp)
    {
        log_error("Failed to open file %s for writing", file);
        return -1;
    }

    curl = curl_easy_init();
    if (curl)
    {
        curl_easy_setopt(curl, CURLOPT_URL, FIRMWARE_URL);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);

        SHA1_Init(&sha_ctx);

        res = curl_easy_perform(curl);
        if (res != CURLE_OK)
        {
            log_error("curl_easy_perform() failed: %s", curl_easy_strerror(res));
            fclose(fp);
            curl_easy_cleanup(curl);
            return -1;
        }

        // Check HTTP response code
        curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response_code);
        if (response_code != 200)
        {
            log_error("HTTP request failed with status code: %ld", response_code);
            fclose(fp);
            curl_easy_cleanup(curl);
            return -1;
        }

        // Calculate SHA1 hash of the downloaded file
        fseek(fp, 0, SEEK_SET);
        while ((bytes_read = fread(buffer, 1, BUFFER_SIZE, fp)) > 0)
        {
            SHA1_Update(&sha_ctx, buffer, bytes_read);
        }

        SHA1_Final(hash, &sha_ctx);

        fclose(fp);
        curl_easy_cleanup(curl);

        return 0;
    }

    fclose(fp);
    return -1;
}
