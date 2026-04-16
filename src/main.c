#include "cli/cli.h"

#include <stdlib.h>

int main(int argc, char **argv)
{
    const hive_status_t status = hive_cli_run(argc, argv);
    return status == HIVE_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
