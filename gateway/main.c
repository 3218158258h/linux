#include "app/app_runner.h"
#include "daemon/daemon_runner.h"
#include "ota/ota_runner.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

static void print_usage()
{
    printf("Usage: gateway app|daemon|ota\n");
}
int main(int argc, char const *argv[])
{
    if (argc <= 1)
    {
        // 参数不足
        print_usage();
        exit(EXIT_FAILURE);
    }

    if (strcmp(argv[1], "app") == 0)
    {
        app_runner_run();
    }
    else{
    return 0;}
}
