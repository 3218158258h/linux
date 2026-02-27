// ota_runner.c
#include "ota_runner.h"
#include "ota_http.h"
#include "../thirdparty/log.c/log.h"
#include <unistd.h>
#include <sys/reboot.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>

static Version current_version = CURRENT_VERSION;

static int install_firmware(const char *temp_file)
{
    const char *target_path = "/usr/local/bin/gateway";
    const char *backup_path = "/usr/local/bin/gateway.bak";
    
    if (rename(target_path, backup_path) != 0) {
        log_error("Failed to backup current firmware");
        return -1;
    }
    
    if (rename(temp_file, target_path) != 0) {
        log_error("Failed to install new firmware, restoring backup");
        rename(backup_path, target_path);
        return -1;
    }
    
    if (chmod(target_path, 0755) != 0) {
        log_error("Failed to set executable permission");
        if (rename(backup_path, target_path) != 0) {
            log_error("CRITICAL: Failed to restore backup!");
        }
        return -1;
    }
    
    log_info("Firmware installed successfully");
    return 0;
}

int ota_runner_checkNewVersion(Version *version)
{
    if (!version) return -1;
    
    if (version->major > current_version.major)
        return 0;

    if (version->major == current_version.major) {
        if (version->minor > current_version.minor)
            return 0;
        if (version->minor == current_version.minor &&
            version->patch > current_version.patch)
            return 0;
    }
    return -1;
}

int ota_runner_run()
{
    Version version;
    while (1)
    {
        if (ota_http_getVersion(&version) < 0) {
            log_error("Failed to get version");
            sleep(86400);
            continue;
        }

        if (ota_runner_checkNewVersion(&version) < 0) {
            log_info("No new version");
            sleep(86400 * 7);
            continue;
        }

        log_info("New version available: %d.%d.%d", 
                 version.major, version.minor, version.patch);

        if (ota_http_getFirmware(TEMP_FILENAME) < 0) {
            log_error("Failed to get firmware");
            sleep(3600);
            continue;
        }

        log_info("Firmware downloaded successfully");
        
        if (install_firmware(TEMP_FILENAME) != 0) {
            log_error("Failed to install firmware");
            unlink(TEMP_FILENAME);
            sleep(3600);
            continue;
        }
        
        log_info("Rebooting to apply update...");
        sleep(2);
        reboot(RB_AUTOBOOT);
    }
    return 0;
}
