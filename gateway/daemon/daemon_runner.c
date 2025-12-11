#include "daemon_runner.h"
#include "daemon_process.h"
#include "thirdparty/log.c/log.h"
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/reboot.h>

static SubProcess subprocess[2];
static int is_running = 1;

static int crush_count = 0;

void daemon_runner_close(int sig)
{
    assert(sig == SIGTERM);
    is_running = 0;
}

int daemon_runner_run()
{
    if (daemon(0, 1) < 0)
    {
        log_error("daemon error");
        exit(EXIT_FAILURE);
    }

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);

    open("/dev/null", O_RDWR);
    open(LOG_FILE, O_RDWR | O_CREAT, 0644);
    open(LOG_FILE, O_RDWR | O_CREAT, 0644);

    daemon_process_init(subprocess, "app");
    daemon_process_init(subprocess + 1, "ota");
    daemon_process_start(subprocess);
    daemon_process_start(subprocess + 1);

    signal(SIGTERM, daemon_runner_close);

    while (is_running)
    {
        pid_t pid = waitpid(-1, NULL, WNOHANG);
        if (pid < 0)
        {
            log_warn("waitpid error");
            continue;
        }

        if (pid == 0)
        {
            // 等待100ms开启下一次检查
            usleep(100000);
            continue;
        }

        crush_count++;

        if (crush_count > 10)
        {
            log_error("crush count %d", crush_count);
            // 这一版本的二进制不是很稳定
            int fd = open("/tmp/gateway.error", O_RDWR | O_CREAT, 0644);
            close(fd);

            // reboot(RB_AUTOBOOT);
        }

        for (int i = 0; i < 2; i++)
        {
            if (pid == subprocess[i].pid)
            {
                log_warn("subprocess %s exit, restarting...", subprocess->name);
                daemon_process_start(subprocess + i);
            }
        }
    }

    for (int i = 0; i < 2; i++)
    {
        daemon_process_stop(subprocess + i);
    }

    return 0;
}