#ifndef HIVE_CLI_CLI_H
#define HIVE_CLI_CLI_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parse command line arguments and run the selected entry point.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return HIVE_STATUS_OK on success.
 */
hive_status_t hive_cli_run(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif
