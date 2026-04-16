#include "core/types.h"

const char *charness_status_to_string(charness_status_t status)
{
    switch (status) {
    case CHARNESS_STATUS_OK:
        return "ok";
    case CHARNESS_STATUS_ERROR:
        return "error";
    case CHARNESS_STATUS_INVALID_ARGUMENT:
        return "invalid_argument";
    case CHARNESS_STATUS_OUT_OF_MEMORY:
        return "out_of_memory";
    case CHARNESS_STATUS_IO_ERROR:
        return "io_error";
    case CHARNESS_STATUS_UNAVAILABLE:
        return "unavailable";
    case CHARNESS_STATUS_NEEDS_APPROVAL:
        return "needs_approval";
    case CHARNESS_STATUS_CANCELLED:
        return "cancelled";
    default:
        return "unknown";
    }
}

const char *charness_agent_kind_to_string(charness_agent_kind_t kind)
{
    switch (kind) {
    case CHARNESS_AGENT_ORCHESTRATOR:
        return "Orchestrator";
    case CHARNESS_AGENT_PLANNER:
        return "Planner";
    case CHARNESS_AGENT_CODER:
        return "Coder";
    case CHARNESS_AGENT_TESTER:
        return "Tester";
    case CHARNESS_AGENT_VERIFIER:
        return "Verifier";
    case CHARNESS_AGENT_EDITOR:
        return "Editor";
    default:
        return "Unknown";
    }
}
