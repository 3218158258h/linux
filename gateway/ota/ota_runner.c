#include "ota_runner.h"
#include "ota_http.h"
#include "thirdparty/log.c/log.h"
#include <unistd.h>
#include <sys/reboot.h>

static Version current_version = CURRENT_VERSION;

/**
 * @brief 比较新旧两个版本
 *
 * @param version 传入的版本
 * @return int 如果传入的Version较新，返回0, 否则返回-1
 */
int ota_runner_checkNewVersion(Version *version)
{
    if (version->major > current_version.major)
    {
        return 0;
    }

    if (version->major == current_version.major)
    {
        if (version->minor > current_version.minor)
        {
            return 0;
        }
        if (version->minor == current_version.minor)
        {
            if (version->patch >= current_version.patch)
            {
                return 0;
            }
        }
    }
    return -1;
}

int ota_runner_run()
{
    Version version;
    while (1)
    {
        if (ota_http_getVersion(&version) < 0)
        {
            log_error("Failed to get version");
            sleep(86400);
            continue;
        }

        if (ota_runner_checkNewVersion(&version) < 0)
        {
            log_info("No new version");
            sleep(86400 * 7);
            continue;
        }

        if (ota_http_getFirmware(TEMP_FILENAME) < 0)
        {
            log_error("Failed to get firmware");
            sleep(3600);
            continue;
        }

        // 如果新的固件下载成功，直接重启
        reboot(RB_AUTOBOOT);
    }
    return 0;
}