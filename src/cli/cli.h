#ifndef CHARNESS_CLI_CLI_H
#define CHARNESS_CLI_CLI_H

#include "core/types.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Parse command line arguments and run the selected entry point.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return CHARNESS_STATUS_OK on success.
 */
charness_status_t charness_cli_run(int argc, char **argv);

#ifdef __cplusplus
}
#endif

#endif
