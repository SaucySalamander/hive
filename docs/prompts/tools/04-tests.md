# Tests, Fuzzing, and Benchmarks

Unit tests
- Registry unit tests: register/unregister, version compatibility.
- Validation tests: ensure `args` validation rejects malformed payloads.

Integration tests
- End-to-end dispatcher test: agent emits `ToolCall`, dispatcher routes, tool returns expected `ToolResponse`.
- Permission tests: calls from different `agent_id`s with/without permissions.

Fuzzing and property tests
- Fuzz `args` against `inputSchema` to confirm validation rejects bad inputs and doesn't crash.

Benchmarks
- Measure dispatcher latency under varying loads and for sync vs streaming calls.
- Measure memory and CPU for the reference harness.

Test harness ideas
- Provide a lightweight harness that can simulate N agents issuing calls concurrently and collect metrics + errors.

CI integration
- Unit and integration tests must run in CI. For sandboxed tests that require containers, use a separate job with limited concurrency.

Acceptance criteria for tests
- 95%+ of validation branches covered for critical tools.
- No crashes or memory errors under fuzzing runs (run under ASAN/UBSAN for native code).
