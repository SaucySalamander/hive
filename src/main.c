#include "cli/cli.h"

#include <stdlib.h>

int main(int argc, char **argv)
{
    const charness_status_t status = charness_cli_run(argc, argv);
    return status == CHARNESS_STATUS_OK ? EXIT_SUCCESS : EXIT_FAILURE;
}
