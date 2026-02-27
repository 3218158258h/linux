#include "../include/app_message.h"
#include <string.h>
#include "../thirdparty/log.c/log.h"
#include "../thirdparty/cJSON/cJSON.h"
#include <stdlib.h>

static char *bin_to_str(unsigned char *binary, int len)
{
    if (!binary || len <= 0) {
        return NULL;
    }
    
    char *hex_str = malloc(len * 2 + 1);
    if (!hex_str)
    {
        log_warn("Not enough memory");
        return NULL;
    }

    for (int i = 0; i < len; i++)
    {
        sprintf(hex_str + i * 2, "%02X", binary[i]);
    }
    hex_str[len * 2] = '\0';
    return hex_str;
}

static int str_to_bin(const char *hex_str, unsigned char *binary, int len)
{
    if (!hex_str || !binary || len <= 0) {
        return -1;
    }
    
    size_t hex_len = strlen(hex_str);
    if (hex_len % 2 != 0)
    {
        log_warn("Hex string is not valid");
        return -1;
    }
    if (len < (int)(hex_len / 2))
    {
        log_warn("Buffer len is not enough");
        return -1;
    }
    len = hex_len / 2;
    for (int i = 0; i < len; i++)
    {
        char high = hex_str[i * 2];
        char low = hex_str[i * 2 + 1];
        
        binary[i] = 0;

        if (high <= '9' && high >= '0')
        {
            binary[i] = high - '0';
        }
        else if (high >= 'a' && high <= 'f')
        {
            binary[i] = high - 'a' + 10;
        }
        else if (high >= 'A' && high <= 'F')
        {
            binary[i] = high - 'A' + 10;
        }
        else
        {
            log_warn("Invalid hex character: %c", high);
            return -1;
        }
        binary[i] <<= 4;

        if (low <= '9' && low >= '0')
        {
            binary[i] |= low - '0';
        }
        else if (low >= 'a' && low <= 'f')
        {
            binary[i] |= low - 'a' + 10;
        }
        else if (low >= 'A' && low <= 'F')
        {
            binary[i] |= low - 'A' + 10;
        }
        else
        {
            log_warn("Invalid hex character: %c", low);
            return -1;
        }
    }
    return len;
}

int app_message_initByBinary(Message *message, void *binary, int len)
{
    if (!message || !binary || len < 3) {
        log_warn("Invalid input parameters");
        return -1;
    }
    
    memset(message, 0, sizeof(Message));
    memcpy(&message->connection_type, binary, 1);
    memcpy(&message->id_len, binary + 1, 1);
    memcpy(&message->data_len, binary + 2, 1);

    if (len != message->id_len + message->data_len + 3)
    {
        log_warn("Message is not valid: expected %d, got %d", 
                 message->id_len + message->data_len + 3, len);
        return -1;
    }

    message->payload = malloc(message->id_len + message->data_len);

    if (!message->payload)
    {
        log_warn("Not enough for message");
        return -1;
    }

    memcpy(message->payload, binary + 3, message->id_len + message->data_len);
    return 0;
}

int app_message_initByJson(Message *message, char *json_str, int len)
{
    if (!message || !json_str || len <= 0) {
        log_warn("Invalid input parameters");
        return -1;
    }
    
    memset(message, 0, sizeof(Message));
    
    cJSON *json_object = cJSON_ParseWithLength(json_str, len);
    if (!json_object) {
        log_warn("Failed to parse JSON");
        return -1;
    }

    cJSON *connection_type = cJSON_GetObjectItem(json_object, "connection_type");
    if (!connection_type) {
        log_warn("Missing connection_type field");
        cJSON_Delete(json_object);
        return -1;
    }
    message->connection_type = connection_type->valueint;

    cJSON *id = cJSON_GetObjectItem(json_object, "id");
    if (!id || !cJSON_IsString(id)) {
        log_warn("Missing or invalid id field");
        cJSON_Delete(json_object);
        return -1;
    }

    if (strlen(id->valuestring) % 2 != 0)
    {
        log_warn("Message id is not valid");
        cJSON_Delete(json_object);
        return -1;
    }
    message->id_len = strlen(id->valuestring) / 2;

    cJSON *data = cJSON_GetObjectItem(json_object, "data");
    if (!data || !cJSON_IsString(data)) {
        log_warn("Missing or invalid data field");
        cJSON_Delete(json_object);
        return -1;
    }
    if (strlen(data->valuestring) % 2 != 0)
    {
        log_warn("Message data is not valid");
        cJSON_Delete(json_object);
        return -1;
    }
    message->data_len = strlen(data->valuestring) / 2;

    message->payload = malloc(message->id_len + message->data_len);

    if (!message->payload)
    {
        log_warn("Not enough memory for message");
        cJSON_Delete(json_object);
        return -1;
    }

    if (str_to_bin(id->valuestring, message->payload, message->id_len) < 0)
    {
        log_warn("ID conversion failed");
        free(message->payload);
        message->payload = NULL;
        cJSON_Delete(json_object);
        return -1;
    }
    if (str_to_bin(data->valuestring, message->payload + message->id_len, message->data_len) < 0)
    {
        log_warn("Data conversion failed");
        free(message->payload);
        message->payload = NULL;
        cJSON_Delete(json_object);
        return -1;
    }

    cJSON_Delete(json_object);
    return 0;
}

int app_message_saveBinary(Message *message, void *binary, int len)
{
    if (!message || !binary || len < 3) {
        return -1;
    }
    
    int total_len = message->id_len + message->data_len + 3;
    if (len < total_len)
    {
        log_warn("Buffer not enough for message");
        return -1;
    }

    memcpy(binary, &message->connection_type, 1);
    memcpy(binary + 1, &message->id_len, 1);
    memcpy(binary + 2, &message->data_len, 1);
    
    if (message->payload && (message->id_len + message->data_len) > 0) {
        memcpy(binary + 3, message->payload, message->id_len + message->data_len);
    }

    return total_len;
}

int app_message_saveJson(Message *message, char *json_str, int len)
{
    if (!message || !json_str || len <= 0) {
        return -1;
    }

    cJSON *json_object = cJSON_CreateObject();
    if (!json_object) return -1;

    char *id_str = NULL;
    char *data_str = NULL;
    
    if (message->payload && message->id_len > 0) {
        id_str = bin_to_str(message->payload, message->id_len);
    }
    if (message->payload && message->data_len > 0) {
        data_str = bin_to_str(message->payload + message->id_len, message->data_len);
    }

    int ret = -1;
    if ((message->id_len > 0 && !id_str) || (message->data_len > 0 && !data_str)) {
        log_warn("Failed to convert binary to string");
        goto cleanup;
    }

    cJSON_AddNumberToObject(json_object, "connection_type", message->connection_type);
    cJSON_AddStringToObject(json_object, "id", id_str ? id_str : "");
    cJSON_AddStringToObject(json_object, "data", data_str ? data_str : "");

    char *str = cJSON_PrintUnformatted(json_object);
    if (!str) {
        log_warn("Failed to print JSON");
        goto cleanup;
    }

    int str_len = (int)strlen(str);
    if (len < str_len + 1) {
        log_warn("Buffer not enough for message");
        cJSON_free(str);
        goto cleanup;
    }

    memcpy(json_str, str, str_len + 1);
    cJSON_free(str);
    ret = 0;

cleanup:
    if (id_str) free(id_str); 
    if (data_str) free(data_str);
    cJSON_Delete(json_object);
    return ret;
}

void app_message_free(Message *message)
{
    if (message && message->payload)
    {
        free(message->payload);
        message->payload = NULL;
        message->id_len = 0;
        message->data_len = 0;
    }
}
