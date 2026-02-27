/**
 * @file app_ota.c
 * @brief A/B分区OTA升级系统实现
 */

#include "app_ota.h"
#include "app_config.h"
#include "../thirdparty/log.c/log.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/reboot.h>
#include <curl/curl.h>
#include <openssl/sha.h>
#include <openssl/rsa.h>
#include <openssl/pem.h>
#include <openssl/err.h>

/* 启动配置文件格式 */
#define BOOT_CONFIG_MAGIC   0x4F544142  /* "OTAB" */
#define BOOT_CONFIG_VERSION 1

typedef struct {
    uint32_t magic;
    uint32_t version;
    uint32_t active_partition;  /* 0=A, 1=B */
    uint32_t boot_count;
    uint32_t upgrade_pending;
    uint32_t checksum;
} BootConfig;

/* 状态字符串 */
static const char *state_strings[] = {
    "IDLE", "CHECKING", "DOWNLOADING", "VERIFYING",
    "INSTALLING", "REBOOTING", "SUCCESS", "FAILED", "ROLLBACK"
};

const char *ota_statestr(OtaState state)
{
    if (state >= OTA_STATE_IDLE && state <= OTA_STATE_ROLLBACK) {
        return state_strings[state];
    }
    return "UNKNOWN";
}

/* 错误字符串 */
static const char *error_strings[] = {
    "OK",
    "Network error",
    "Download failed",
    "Checksum error",
    "Signature verification failed",
    "Write error",
    "Boot error",
    "Rollback failed",
    "Invalid parameter"
};

const char *ota_strerror(OtaError error)
{
    if (error >= OTA_ERR_NETWORK && error <= OTA_ERR_INVALID) {
        return error_strings[-error];
    }
    return "Unknown error";
}

/* 计算SHA256 */
static int calculate_sha256(const char *file_path, uint8_t *hash)
{
    FILE *fp = fopen(file_path, "rb");
    if (!fp) {
        log_error("Failed to open file: %s", file_path);
        return -1;
    }
    
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    
    uint8_t buffer[4096];
    size_t len;
    
    while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        SHA256_Update(&ctx, buffer, len);
    }
    
    SHA256_Final(hash, &ctx);
    fclose(fp);
    
    return 0;
}

/* 验证RSA签名 */
static int verify_signature(const char *file_path, const char *sig_path,
                            const char *pub_key_path)
{
    /* 计算文件哈希 */
    uint8_t hash[SHA256_DIGEST_LENGTH];
    if (calculate_sha256(file_path, hash) != 0) {
        return -1;
    }
    
    /* 读取签名 */
    FILE *sig_fp = fopen(sig_path, "rb");
    if (!sig_fp) {
        log_error("Failed to open signature file");
        return -1;
    }
    
    uint8_t signature[256];
    size_t sig_len = fread(signature, 1, sizeof(signature), sig_fp);
    fclose(sig_fp);
    
    /* 读取公钥 */
    FILE *pub_fp = fopen(pub_key_path, "r");
    if (!pub_fp) {
        log_error("Failed to open public key file");
        return -1;
    }
    
    RSA *rsa = PEM_read_RSA_PUBKEY(pub_fp, NULL, NULL, NULL);
    fclose(pub_fp);
    
    if (!rsa) {
        log_error("Failed to read public key");
        return -1;
    }
    
    /* 验证签名 */
    int ret = RSA_verify(NID_sha256, hash, SHA256_DIGEST_LENGTH,
                        signature, sig_len, rsa);
    
    RSA_free(rsa);
    
    if (ret != 1) {
        log_error("Signature verification failed");
        return -1;
    }
    
    log_info("Signature verification passed");
    return 0;
}

/* 读取启动配置 */
static int read_boot_config(const char *path, BootConfig *config)
{
    int fd = open(path, O_RDONLY);
    if (fd < 0) {
        return -1;
    }
    
    ssize_t len = read(fd, config, sizeof(BootConfig));
    close(fd);
    
    if (len != sizeof(BootConfig)) {
        return -1;
    }
    
    /* 验证魔数 */
    if (config->magic != BOOT_CONFIG_MAGIC) {
        log_error("Invalid boot config magic");
        return -1;
    }
    
    return 0;
}

/* 写入启动配置 */
static int write_boot_config(const char *path, BootConfig *config)
{
    int fd = open(path, O_WRONLY | O_CREAT, 0644);
    if (fd < 0) {
        log_error("Failed to open boot config for writing");
        return -1;
    }
    
    config->magic = BOOT_CONFIG_MAGIC;
    config->version = BOOT_CONFIG_VERSION;
    
    /* 计算校验和 */
    config->checksum = 0;
    uint8_t *p = (uint8_t *)config;
    uint32_t sum = 0;
    for (size_t i = 0; i < sizeof(BootConfig) - 4; i++) {
        sum += p[i];
    }
    config->checksum = sum;
    
    ssize_t len = write(fd, config, sizeof(BootConfig));
    close(fd);
    
    if (len != sizeof(BootConfig)) {
        return -1;
    }
    
    return 0;
}

/* CURL写入回调 */
static size_t write_callback(void *ptr, size_t size, size_t nmemb, void *userdata)
{
    FILE *fp = (FILE *)userdata;
    return fwrite(ptr, size, nmemb, fp);
}

/* CURL进度回调 */
static int progress_callback(void *userdata, curl_off_t dltotal, curl_off_t dlnow,
                            curl_off_t ultotal, curl_off_t ulnow)
{
    OtaManager *manager = (OtaManager *)userdata;
    
    if (dltotal > 0) {
        manager->download_progress = (int)(dlnow * 100 / dltotal);
        if (manager->on_progress) {
            manager->on_progress(manager, manager->download_progress);
        }
    }
    
    return 0;
}

int ota_init(OtaManager *manager, const OtaConfig *config)
{
    if (!manager) return -1;
    
    memset(manager, 0, sizeof(OtaManager));
    
    if (config) {
        memcpy(&manager->config, config, sizeof(OtaConfig));
    } else {
        /* 默认配置 */
        strcpy(manager->config.partition_a, "/dev/mmcblk0p2");
        strcpy(manager->config.partition_b, "/dev/mmcblk0p3");
        strcpy(manager->config.boot_config, "/etc/gateway/boot.conf");
        strcpy(manager->config.public_key_path, "/etc/gateway/public.key");
        manager->config.check_interval = 86400;  /* 24小时 */
        manager->config.max_boot_attempts = 3;
        manager->config.auto_rollback = 1;
    }
    
    /* 初始化分区信息 */
    strncpy(manager->partition_a.name, "A", sizeof(manager->partition_a.name));
    strncpy(manager->partition_a.device, manager->config.partition_a,
            sizeof(manager->partition_a.device));
    
    strncpy(manager->partition_b.name, "B", sizeof(manager->partition_b.name));
    strncpy(manager->partition_b.device, manager->config.partition_b,
            sizeof(manager->partition_b.device));
    
    /* 读取启动配置 */
    BootConfig boot_cfg;
    if (read_boot_config(manager->config.boot_config, &boot_cfg) == 0) {
        if (boot_cfg.active_partition == 0) {
            manager->active_partition = &manager->partition_a;
            manager->inactive_partition = &manager->partition_b;
        } else {
            manager->active_partition = &manager->partition_b;
            manager->inactive_partition = &manager->partition_a;
        }
        manager->active_partition->boot_count = boot_cfg.boot_count;
    } else {
        /* 默认使用A分区 */
        manager->active_partition = &manager->partition_a;
        manager->inactive_partition = &manager->partition_b;
    }
    
    /* 获取当前版本 */
    FILE *fp = fopen("/etc/gateway/version", "r");
    if (fp) {
        fgets(manager->current_version, sizeof(manager->current_version), fp);
        fclose(fp);
        /* 去除换行 */
        char *nl = strchr(manager->current_version, '\n');
        if (nl) *nl = '\0';
    }
    
    manager->state = OTA_STATE_IDLE;
    manager->is_initialized = 1;
    
    log_info("OTA initialized: current=%s, active=%s",
             manager->current_version, manager->active_partition->name);
    
    return 0;
}

int ota_init_default(OtaManager *manager)
{
    return ota_init(manager, NULL);
}

void ota_close(OtaManager *manager)
{
    if (!manager) return;
    
    manager->is_initialized = 0;
    log_info("OTA closed");
}

int ota_check_update(OtaManager *manager, char *out_version, size_t max_len)
{
    if (!manager || !manager->is_initialized) return -1;
    
    manager->state = OTA_STATE_CHECKING;
    if (manager->on_state_changed) {
        manager->on_state_changed(manager, OTA_STATE_CHECKING);
    }
    
    /* 下载版本信息 */
    CURL *curl = curl_easy_init();
    if (!curl) {
        log_error("Failed to init curl");
        manager->state = OTA_STATE_FAILED;
        return -1;
    }
    
    char version_url[512];
    snprintf(version_url, sizeof(version_url), "%s/version.txt",
             manager->config.version_url);
    
    FILE *fp = tmpfile();
    curl_easy_setopt(curl, CURLOPT_URL, version_url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        log_error("Failed to fetch version info: %s", curl_easy_strerror(res));
        fclose(fp);
        manager->state = OTA_STATE_FAILED;
        return -1;
    }
    
    /* 读取版本号 */
    rewind(fp);
    char new_version[64] = {0};
    fgets(new_version, sizeof(new_version), fp);
    fclose(fp);
    
    char *nl = strchr(new_version, '\n');
    if (nl) *nl = '\0';
    
    /* 比较版本 */
    if (strcmp(new_version, manager->current_version) > 0) {
        strncpy(manager->new_version, new_version, sizeof(manager->new_version) - 1);
        if (out_version && max_len > 0) {
            strncpy(out_version, new_version, max_len - 1);
        }
        
        log_info("New version available: %s", new_version);
        manager->state = OTA_STATE_IDLE;
        return 0;
    }
    
    log_debug("No update available");
    manager->state = OTA_STATE_IDLE;
    return 1;
}

int ota_download(OtaManager *manager)
{
    if (!manager || !manager->is_initialized) return -1;
    
    manager->state = OTA_STATE_DOWNLOADING;
    manager->download_progress = 0;
    
    if (manager->on_state_changed) {
        manager->on_state_changed(manager, OTA_STATE_DOWNLOADING);
    }
    
    /* 构建下载URL */
    char firmware_url[512];
    snprintf(firmware_url, sizeof(firmware_url), "%s/firmware-%s.bin",
             manager->config.firmware_url, manager->new_version);
    
    log_info("Downloading firmware from: %s", firmware_url);
    
    /* 打开目标文件 */
    char tmp_path[256];
    snprintf(tmp_path, sizeof(tmp_path), "/tmp/firmware-%s.bin", manager->new_version);
    
    FILE *fp = fopen(tmp_path, "wb");
    if (!fp) {
        log_error("Failed to create temp file");
        manager->state = OTA_STATE_FAILED;
        return -1;
    }
    
    /* 下载 */
    CURL *curl = curl_easy_init();
    if (!curl) {
        fclose(fp);
        manager->state = OTA_STATE_FAILED;
        return -1;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, firmware_url);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    curl_easy_setopt(curl, CURLOPT_XFERINFODATA, manager);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 300L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    
    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);
    fclose(fp);
    
    if (res != CURLE_OK) {
        log_error("Download failed: %s", curl_easy_strerror(res));
        remove(tmp_path);
        manager->state = OTA_STATE_FAILED;
        
        if (manager->on_error) {
            manager->on_error(manager, OTA_ERR_DOWNLOAD);
        }
        return -1;
    }
    
    log_info("Download complete: %s", tmp_path);
    manager->state = OTA_STATE_IDLE;
    return 0;
}

int ota_verify(OtaManager *manager)
{
    if (!manager || !manager->is_initialized) return -1;
    
    manager->state = OTA_STATE_VERIFYING;
    
    if (manager->on_state_changed) {
        manager->on_state_changed(manager, OTA_STATE_VERIFYING);
    }
    
    char firmware_path[256];
    snprintf(firmware_path, sizeof(firmware_path), "/tmp/firmware-%s.bin",
             manager->new_version);
    
    char sig_path[256];
    snprintf(sig_path, sizeof(sig_path), "/tmp/firmware-%s.sig",
             manager->new_version);
    
    /* 下载签名文件 */
    char sig_url[512];
    snprintf(sig_url, sizeof(sig_url), "%s/firmware-%s.sig",
             manager->config.firmware_url, manager->new_version);
    
    CURL *curl = curl_easy_init();
    if (curl) {
        FILE *fp = fopen(sig_path, "wb");
        if (fp) {
            curl_easy_setopt(curl, CURLOPT_URL, sig_url);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            curl_easy_perform(curl);
            fclose(fp);
        }
        curl_easy_cleanup(curl);
    }
    
    /* 验证签名 */
    if (verify_signature(firmware_path, sig_path,
                        manager->config.public_key_path) != 0) {
        log_error("Signature verification failed");
        manager->state = OTA_STATE_FAILED;
        
        if (manager->on_error) {
            manager->on_error(manager, OTA_ERR_SIGNATURE);
        }
        return -1;
    }
    
    log_info("Firmware verified successfully");
    manager->state = OTA_STATE_IDLE;
    return 0;
}

int ota_install(OtaManager *manager)
{
    if (!manager || !manager->is_initialized) return -1;
    
    manager->state = OTA_STATE_INSTALLING;
    
    if (manager->on_state_changed) {
        manager->on_state_changed(manager, OTA_STATE_INSTALLING);
    }
    
    char firmware_path[256];
    snprintf(firmware_path, sizeof(firmware_path), "/tmp/firmware-%s.bin",
             manager->new_version);
    
    /* 打开固件文件 */
    FILE *fp = fopen(firmware_path, "rb");
    if (!fp) {
        log_error("Failed to open firmware file");
        manager->state = OTA_STATE_FAILED;
        return -1;
    }
    
    /* 获取文件大小 */
    fseek(fp, 0, SEEK_END);
    size_t firmware_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    /* 打开目标分区 */
    int fd = open(manager->inactive_partition->device, O_WRONLY);
    if (fd < 0) {
        log_error("Failed to open partition: %s",
                  manager->inactive_partition->device);
        fclose(fp);
        manager->state = OTA_STATE_FAILED;
        return -1;
    }
    
    /* 写入固件 */
    uint8_t buffer[4096];
    size_t total_written = 0;
    ssize_t len;
    
    while ((len = fread(buffer, 1, sizeof(buffer), fp)) > 0) {
        ssize_t written = write(fd, buffer, len);
        if (written != len) {
            log_error("Write failed: %zd != %zd", written, len);
            close(fd);
            fclose(fp);
            manager->state = OTA_STATE_FAILED;
            
            if (manager->on_error) {
                manager->on_error(manager, OTA_ERR_WRITE);
            }
            return -1;
        }
        total_written += written;
        
        /* 更新进度 */
        manager->download_progress = (int)(total_written * 100 / firmware_size);
        if (manager->on_progress) {
            manager->on_progress(manager, manager->download_progress);
        }
    }
    
    fsync(fd);
    close(fd);
    fclose(fp);
    
    log_info("Firmware written to partition %s: %zu bytes",
             manager->inactive_partition->name, total_written);
    
    /* 更新启动配置 */
    BootConfig boot_cfg;
    read_boot_config(manager->config.boot_config, &boot_cfg);
    
    boot_cfg.active_partition = (manager->inactive_partition == &manager->partition_b) ? 1 : 0;
    boot_cfg.boot_count = 0;
    boot_cfg.upgrade_pending = 1;
    
    if (write_boot_config(manager->config.boot_config, &boot_cfg) != 0) {
        log_error("Failed to update boot config");
        manager->state = OTA_STATE_FAILED;
        return -1;
    }
    
    /* 清理临时文件 */
    remove(firmware_path);
    
    log_info("Installation complete, upgrade pending");
    manager->state = OTA_STATE_SUCCESS;
    
    if (manager->on_state_changed) {
        manager->on_state_changed(manager, OTA_STATE_SUCCESS);
    }
    
    return 0;
}

int ota_upgrade(OtaManager *manager)
{
    if (!manager || !manager->is_initialized) return -1;
    
    log_info("Starting OTA upgrade...");
    
    /* 检查更新 */
    char version[64];
    if (ota_check_update(manager, version, sizeof(version)) != 0) {
        log_info("No update available");
        return 1;
    }
    
    /* 下载 */
    if (ota_download(manager) != 0) {
        log_error("Download failed");
        return -1;
    }
    
    /* 验证 */
    if (ota_verify(manager) != 0) {
        log_error("Verification failed");
        return -1;
    }
    
    /* 安装 */
    if (ota_install(manager) != 0) {
        log_error("Installation failed");
        return -1;
    }
    
    log_info("OTA upgrade successful, reboot required");
    return 0;
}

int ota_rollback(OtaManager *manager)
{
    if (!manager || !manager->is_initialized) return -1;
    
    manager->state = OTA_STATE_ROLLBACK;
    
    if (manager->on_state_changed) {
        manager->on_state_changed(manager, OTA_STATE_ROLLBACK);
    }
    
    log_warn("Performing rollback...");
    
    /* 更新启动配置 */
    BootConfig boot_cfg;
    read_boot_config(manager->config.boot_config, &boot_cfg);
    
    /* 切换回之前的分区 */
    boot_cfg.active_partition = (boot_cfg.active_partition == 0) ? 1 : 0;
    boot_cfg.boot_count = 0;
    boot_cfg.upgrade_pending = 0;
    
    if (write_boot_config(manager->config.boot_config, &boot_cfg) != 0) {
        log_error("Failed to update boot config for rollback");
        manager->state = OTA_STATE_FAILED;
        return -1;
    }
    
    log_info("Rollback complete, reboot required");
    manager->state = OTA_STATE_IDLE;
    
    return 0;
}

OtaState ota_get_state(OtaManager *manager)
{
    return manager ? manager->state : OTA_STATE_IDLE;
}

int ota_get_progress(OtaManager *manager)
{
    return manager ? manager->download_progress : 0;
}

int ota_get_current_version(OtaManager *manager, char *out_version, size_t max_len)
{
    if (!manager || !out_version) return -1;
    
    strncpy(out_version, manager->current_version, max_len - 1);
    out_version[max_len - 1] = '\0';
    return 0;
}

void ota_get_partitions(OtaManager *manager, OtaPartition *partition_a, OtaPartition *partition_b)
{
    if (!manager) return;
    
    if (partition_a) memcpy(partition_a, &manager->partition_a, sizeof(OtaPartition));
    if (partition_b) memcpy(partition_b, &manager->partition_b, sizeof(OtaPartition));
}

void ota_on_state_changed(OtaManager *manager,
                          void (*callback)(OtaManager *, OtaState))
{
    if (manager) manager->on_state_changed = callback;
}

void ota_on_progress(OtaManager *manager,
                     void (*callback)(OtaManager *, int))
{
    if (manager) manager->on_progress = callback;
}

void ota_on_error(OtaManager *manager,
                  void (*callback)(OtaManager *, OtaError))
{
    if (manager) manager->on_error = callback;
}
