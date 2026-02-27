#if !defined(__OTA_HTTP_H__)
#define __OTA_HTTP_H__

#define VERSION_URL "http://example.com/version"
#define FIRMWARE_URL "http://example.com/firmware"

typedef struct VersionStruct
{
    int major;
    int minor;
    int patch;
} Version;

/**
 * @brief 使用curl库请求版本信息
 * 
 * @param version 要填充的版本结构体
 * @return int 0 成功，-1 失败
 */
int ota_http_getVersion(Version *version);

/**
 * @brief 使用curl库请求固件，并使用openssl库的sha1算法进行hash验证
 * 
 * @param file 要保存的文件名
 * @return int 0 下载并验证成功，-1 失败
 */
int ota_http_getFirmware(char *file);

#endif // __OTA_HTTP_H__
