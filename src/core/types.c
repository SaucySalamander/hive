#include "core/types.h"

const char *hive_status_to_string(hive_status_t status)
{
    switch (status) {
    case HIVE_STATUS_OK:
        return "ok";
    case HIVE_STATUS_ERROR:
        return "error";
    case HIVE_STATUS_INVALID_ARGUMENT:
        return "invalid_argument";
    case HIVE_STATUS_OUT_OF_MEMORY:
        return "out_of_memory";
    case HIVE_STATUS_IO_ERROR:
        return "io_error";
    case HIVE_STATUS_UNAVAILABLE:
        return "unavailable";
    case HIVE_STATUS_NEEDS_APPROVAL:
        return "needs_approval";
    case HIVE_STATUS_CANCELLED:
        return "cancelled";
    default:
        return "unknown";
    }
}

const char *hive_agent_kind_to_string(hive_agent_kind_t kind)
{
    switch (kind) {
    case HIVE_AGENT_ORCHESTRATOR:
        return "Orchestrator";
    case HIVE_AGENT_PLANNER:
        return "Planner";
    case HIVE_AGENT_CODER:
        return "Coder";
    case HIVE_AGENT_TESTER:
        return "Tester";
    case HIVE_AGENT_VERIFIER:
        return "Verifier";
    case HIVE_AGENT_EDITOR:
        return "Editor";
    default:
        return "Unknown";
    }
}
