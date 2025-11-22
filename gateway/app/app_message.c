#include "app_message.h"
#include <string.h>
#include "thirdparty/log.c/log.h"
#include "thirdparty/cJSON/cJSON.h"
#include <stdlib.h>

// 将二进制数据转换为可读的十六进制字符串, 调用者需要负责释放str
static char *bin_to_str(unsigned char *binary, int len)
{
    //每个字节（8 位）对应 2 个十六进制字符（1 位十六进制 = 4 位二进制）
    //因此需要 len * 2 个字符存储核心数据，剩下一个字节\0
    char *hex_str = malloc(len * 2 + 1);
    if (!hex_str)
    {
        log_warn("Not enough memory");
        return NULL;
    }

    for (int i = 0; i < len; i++)
    {
        //每个二进制数一个字节，需要两个十六进制字符表示
        sprintf(hex_str + i * 2, "%02X", binary[i]);
    }
    hex_str[len * 2] = '\0';
    return hex_str;
}

// 将十六进制字符串转化为二进制数据，返回实际转化的长度
static int str_to_bin(char *hex_str, unsigned char *binary, int len)
{
    if (strlen(hex_str) % 2 != 0)
    {
        log_warn("Hex string is not valid");
        return -1;
    }
    if (len < (int)(strlen(hex_str) / 2))
    {
        log_warn("Buffer len is not enough");
        return -1;
    }
    len = strlen(hex_str) / 2;
    for (int i = 0; i < len; i++)
    {
        // 现在需要转化的字节有两个，分别是hex_str[i*2]和hex_str[i*2+1]
        char high = hex_str[i * 2];
        char low = hex_str[i * 2 + 1];

            // 处理高位字符：转换为4位数值，左移4位占字节的「高4位」
        if (high <= '9' && high >= '0')
        {
            binary[i] = high - '0'; // 数字0-9：ASCII值相减得到实际数值（如'3'-'0'=3）
        }
        else if (high >= 'a' && high <= 'f')
        {
            binary[i] = high - 'a' + 10; // 小写a-f：转为10-15（如'a'-'a'+10=10）
        }
        else if (high >= 'A' && high <= 'F')
        {
            binary[i] = high - 'A' + 10; // 大写A-F：转为10-15（如'F'-'A'+10=15）
        }
        binary[i] <<= 4;//左移4位：将4位高位数值移到字节的高4位（如3→0x30）

        // 再处理低位
        if (low <= '9' && low >= '0')
        {
            //按位或：不影响高4位，仅填充低4位
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
    }
    return len;
}



int app_message_initByBinary(Message *message, void *binary, int len)
{
    
    if (message == NULL || binary == NULL || len <= 0) {
    log_warn("Invalid input parameters");
    return -1;
}
    // 首先初始化message，置零
    memset(message, 0, sizeof(Message));
    // 首先读取类型和长度
    memcpy(&message->connection_type, binary, 1);
    memcpy(&message->id_len, binary + 1, 1);
    memcpy(&message->data_len, binary + 2, 1);

    if (len != message->id_len + message->data_len + 3)
    {
        log_warn("Message is not valid");
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
    // 使用cJSON解析字符串
    cJSON *json_object = cJSON_ParseWithLength(json_str, len);

    // 首先读取ConnectionType
    cJSON *connection_type = cJSON_GetObjectItem(json_object, "connection_type");
    message->connection_type = connection_type->valueint;

    // 读取id和data的长度
    cJSON *id = cJSON_GetObjectItem(json_object, "id");

    //十六进制的 1 位（1 个字符）对应 4 个二进制位，如0b1111对应0xf
    //2 个十六进制字符才能组成 1 个完整的字节（8 个二进制位）0b11111111对应0xff
    if (strlen(id->valuestring) % 2 != 0)
    {
        log_warn("Message is not valid");
        return -1;
    }
    message->id_len = strlen(id->valuestring) / 2;

    cJSON *data = cJSON_GetObjectItem(json_object, "data");
    if (strlen(data->valuestring) % 2 != 0)
    {
        log_warn("Message is not valid");
        return -1;
    }
    message->data_len = strlen(data->valuestring) / 2;

    // 申请message的payload内存
    message->payload = malloc(message->id_len + message->data_len);

    if (!message->payload)
    {
        log_warn("Not enough memory for message");
        return -1;
    }

    if (str_to_bin(id->valuestring, message->payload, message->id_len) < 0)
    {
        log_warn("Convertion failed");
        free(message->payload);
        return -1;
    }
    if (str_to_bin(data->valuestring, message->payload + message->id_len, message->data_len) < 0)
    {
        log_warn("Convertion failed");
        free(message->payload);
        return -1;
    }

    cJSON_Delete(json_object);
    return 0;
}


int app_message_saveBinary(Message *message, void *binary, int len)
{
    // 将来保存二进制需要的长度为 3个字节元数据和 id_len + data_len长度的数据
    int total_len = message->id_len + message->data_len + 3;
    if (len < total_len)
    {
        log_warn("Buffer not enough for message");
        return -1;
    }

    memcpy(binary, &message->connection_type, 1);
    memcpy(binary + 1, &message->id_len, 1);
    memcpy(binary + 2, &message->data_len, 1);
    memcpy(binary + 3, message->payload, message->id_len + message->data_len);

    return total_len;
}

int app_message_saveJson(Message *message, char *json_str, int len)
{
    // 首先生成Json Object
    cJSON *json_object = cJSON_CreateObject();

    cJSON_AddNumberToObject(json_object, "connection_type", message->connection_type);

    cJSON_AddStringToObject(json_object, "id", bin_to_str(message->payload, message->id_len));
    cJSON_AddStringToObject(json_object, "data", bin_to_str(message->payload + message->id_len, message->data_len));

    char *str = cJSON_PrintUnformatted(json_object);
    if (len < (int)(strlen(str) + 1))
    {
        log_warn("Buffer not enough for message");
        return -1;
    }
    strcpy(json_str, str);

    cJSON_free(str);
    cJSON_Delete(json_object);
    return 0;
}


void app_message_free(Message *message)
{
    if (message->payload)
    {
        free(message->payload);
        message->payload = NULL;
    }
}
