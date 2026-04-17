# Inference backend abstraction — Adapters and implementation guidance

Adapter kinds
- HTTP/REST adapter: talks to OpenAI/Anthropic-style endpoints via libcurl. Pros: simple. Cons: network/credential management.
- gRPC adapter: talks to gRPC-based backends (internal inference clusters). Pros: performance and streaming. Cons: dependency.
- Process-based adapter: spawns a subprocess (eg. `./llama.cpp/bin/main`) and speaks stdin/stdout or a simple framed protocol.
- FFI/shared-library adapter: loads a vendor SDK as a shared library and calls into it.
- External plugin: adapter runs as a standalone process exposing a small JSON over HTTP or unix-socket control plane.

Guidelines for adapters
- Config: accept a single JSON blob from `inf_create(backend_name, config_json)` containing credentials, timeouts, rate limits, model mappings.
- Thread-safety: document whether adapter supports concurrent calls. If not, core must serialize requests.
- Timeouts & retries: adapters must support per-request timeout and exponential backoff for transient failures.
- Rate limiting & batching: where supported, implement request pooling and model-specific batching.

Streaming semantics
- Use a small set of event types: `token`/`delta`, `done`, `error`, `meta`. Stream callback receives chunks; core assembles complete result if requested.
- Provide a cancel token integer returned by `inf_complete_stream()` that `inf_cancel()` uses.

Security
- Do not log secrets. Prefer reading API keys from environment variables or secure store.
- Config JSON may include placeholders for secrets; advise adapters to support reading from env or files.

Testing adapters
- Each adapter must pass a contract suite that checks: sync completion, streaming completion (including partial order), cancellation, error propagation, and model listing.

Telemetry & instrumentation
- Emit metrics: request latency, tokens consumed, error counts, success rate. Provide hooks to integrate with existing logging/metrics in Hive.

Model mapping
- Keep an internal logical model name layer (eg. `hive:default-chat`) which maps to provider names per-adapter via config. This lets agent config remain provider-agnostic.
