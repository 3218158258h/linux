#include "../include/daemon_runner.h"
#include "../include/daemon_process.h"
#include "../thirdparty/log.c/log.h"
#include <sys/wait.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <assert.h>
#include <sys/reboot.h>
#include <string.h>
#include <time.h>
#include <errno.h>

static SubProcess subprocess[2];
static int is_running = 1;
static int crash_count = 0;
static time_t last_crash_time = 0;

#define CRASH_COUNT_RESET_INTERVAL 300

void daemon_runner_close(int sig)
{
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

    if (daemon_process_init(&subprocess[0], "app") != 0) {
        log_error("Failed to init app subprocess");
        return -1;
    }
    if (daemon_process_init(&subprocess[1], "ota") != 0) {
        log_error("Failed to init ota subprocess");
        daemon_process_free(&subprocess[0]);
        return -1;
    }
    
    signal(SIGTERM, daemon_runner_close);
    daemon_process_start(&subprocess[0]);
    daemon_process_start(&subprocess[1]);


    while (is_running)
    {
        int status = 0;
        pid_t pid = waitpid(-1, &status, WNOHANG);
        if (pid < 0)
        {
            if (errno == EINTR) continue;
            log_warn("waitpid error: %s", strerror(errno));
            usleep(100000);
            continue;
        }

        if (pid == 0)
        {
            usleep(100000);
            continue;
        }

        time_t now = time(NULL);
        if (last_crash_time > 0 && 
            (now - last_crash_time) > CRASH_COUNT_RESET_INTERVAL) {
            crash_count = 0;
        }
        // 需要检查进程退出状态
        // 检查是否是正常退出
        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            log_info("Process %d exited normally", pid);
        // 正常退出不计入崩溃
        } else {
            // 异常退出才计入崩溃
            crash_count++;
            last_crash_time = now;
        }
        

        if (crash_count > 10)
        {
            log_error("Crash count exceeded: %d", crash_count);
            int fd = open("/tmp/gateway.error", O_RDWR | O_CREAT, 0644);
            if (fd >= 0) {
                char buf[64];
                snprintf(buf, sizeof(buf), "crash_count=%d\n", crash_count);
                write(fd, buf, strlen(buf));
                close(fd);
            }
        }

        for (int i = 0; i < 2; i++)
        {
            if (pid == subprocess[i].pid)
            {
                log_warn("Subprocess %s (pid=%d) exited, restarting...", 
                         subprocess[i].name, pid);
                subprocess[i].pid = -1;
                daemon_process_start(&subprocess[i]);
                break;
            }
        }
    }

    for (int i = 0; i < 2; i++)
    {
        daemon_process_stop(&subprocess[i]);
        daemon_process_free(&subprocess[i]);
    }

    return 0;
}
