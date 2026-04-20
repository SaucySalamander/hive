# AI Agents, Skills, and Tools

Short summary
--

The repository implements a hierarchical, role-based agent model (Orchestrator → Planner → Coder → Tester → Verifier → Editor). Each agent is a static descriptor plus a function table that either runs a custom behaviour or calls the common generation pipeline. Key implementation points: [src/core/agent/agent.h](src/core/agent/agent.h) and [src/core/agent/agent.c](src/core/agent/agent.c).

Agent implementation
--

- **Descriptor + vtable**: `hive_agent_t` holds `kind`, `name`, `instructions`, and a `vtable` with a `run()` hook. Concrete agents either provide their own `run` or delegate to `hive_agent_generate()`.
- **Generation pipeline**: `hive_agent_generate()` composes a structured prompt (workspace, rules, supervisor brief, prior output) and calls the inference adapter via `hive_inference_adapter_generate()`. The raw response is passed through `hive_reflexion_apply()` for a lightweight self-critique/refinement step.
- **Standard agents**: each stage has a small static descriptor, e.g. `Orchestrator` in [src/core/agent/orchestrator.c](src/core/agent/orchestrator.c). Other stage implementations live in `src/core/agent/*.c`.

Tools & execution
--

- **Tools dispatch**: `hive_agent_execute_tool()` (see [src/core/agent/tools_dispatch.c](src/core/agent/tools_dispatch.c)) maps textual operations like `read_file`, `write_file`, `grep`, and `git_status` to `hive_tool_request_t` and calls the tool registry.
- **Tool registry**: `hive_tool_registry_execute()` in [src/tools/registry.c](src/tools/registry.c) performs workspace-relative resolution, optional approval gates (via `hive_tool_approval_fn`) and returns `hive_tool_result_t`. Note: `run_command` is intentionally stubbed/unavailable in the skeleton.
- **Approval gate**: the registry supports `auto_approve` or an `approval_fn` hook. Runtimes that want explicit user consent should pass an approval function into `hive_tool_registry_init()`.

Inference & reflexion
--

- **Inference adapter**: the runtime selects a backend name (env `HIVE_INFERENCE_BACKEND` or `options`) and initializes it via `hive_inference_adapter_init_named()`. A mock backend is provided; see `src/core/inference/*` for the registry and mock implementation.
- **Reflexion**: `hive_reflexion_apply()` appends a brief self-critique and returns a refined output. The evaluator scores stage outputs and drives refinement decisions in the state machine.

Skills model and extensibility
--

- Skills are currently expressed as static agent descriptors (instructions embedded in code). This is explicit and auditable but not runtime-configurable. Suggested improvements:
  - Add a serialized `skills/` manifest to declare agent prompts and instruction variants.
  - Allow dynamic registration of agent descriptors and vtables for plugin-style behaviours.

Files to inspect
--

- Agent core: [src/core/agent/agent.h](src/core/agent/agent.h), [src/core/agent/agent.c](src/core/agent/agent.c)
- Concrete agents: [src/core/agent/*.c](src/core/agent/)
- Tools registry: [src/tools/registry.c](src/tools/registry.c)
- Tools dispatch: [src/core/agent/tools_dispatch.c](src/core/agent/tools_dispatch.c)
- Inference: [src/core/inference/inference.c](src/core/inference/inference.c), [src/core/inference/adapter.h](src/core/inference/adapter.h)

Open questions / next work
--

- Decide whether agent prompts/instructions should be editable at runtime (file-based manifests) or remain in-code descriptors for reproducibility.
- Define a minimal skill manifest format and add a loader that converts it into `hive_agent_t` descriptors at runtime.
