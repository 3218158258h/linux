#include "daemon_process.h"
#include <stdlib.h>
#include "thirdparty/log.c/log.h"
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <string.h>

int daemon_process_init(SubProcess *subprocess, const char *name)
{
    subprocess->args = malloc(sizeof(char *) * 3);
    if (subprocess->args == NULL)
    {
        return -1;
        log_error("Failed to allocate memory for subprocess args");
    }
    subprocess->name = malloc(sizeof(char) * (strlen(name) + 1));
    if (subprocess->name == NULL)
    {
        log_error("Failed to allocate memory for subprocess name");
        free(subprocess->args);
        return -1;
    }

    strcpy(subprocess->name, name);
    subprocess->pid = -1;

    subprocess->args[0] = PROGRAM_NAME;
    subprocess->args[1] = subprocess->name;
    subprocess->args[2] = NULL;
    return 0;
}

int daemon_process_start(SubProcess *subprocess)
{
    log_info("Starting subprocess %s", subprocess->name);
    subprocess->pid = fork();
    if (subprocess->pid < 0)
    {
        log_error("Failed to fork subprocess %s", subprocess->name);
        return -1;
    }
    if (subprocess->pid == 0)
    {
        if (execve(PROGRAM_NAME, subprocess->args, __environ) < 0)
        {
            log_error("Failed to execve subprocess %s", subprocess->name);
            exit(EXIT_FAILURE);
        }
    }

    return 0;
}

int daemon_process_stop(SubProcess *subprocess)
{
    log_info("Stopping subprocess %s", subprocess->name);
    if (kill(subprocess->pid, SIGTERM) < 0)
    {
        log_error("Failed to kill subprocess %s", subprocess->name);
        return -1;
    }
    if (waitpid(subprocess->pid, NULL, 0) < 0)
    {
        log_error("Failed to wait for subprocess %s", subprocess->name);
        return -1;
    }
    return 0;
}
