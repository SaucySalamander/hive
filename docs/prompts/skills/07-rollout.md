# Rollout and migration plan

Phases
------
1. Design & docs (this phase) — finalize manifest and API spec.
2. Prototype — implement a minimal Broker + one process-local provider and one agent integration.
3. Pilot — enable a small set of agents to call skills in staging.
4. Incremental rollout — onboard more skills and agents, add adapters.
5. Production — enforce policies, run benchmarks and monitor.

Pilot checklist
---------------
- Validate basic invocation flows end-to-end.
- Ensure policy checks and logs are present.
- Run integration tests and compare results to baseline.

Fallback & rollback
-------------------
- Provide fallback to direct-agent logic when a skill is unavailable.
- During rollout, add traffic-shaping and gradual ramp percentages.

Monitoring
----------
- Track invocation success rates and latencies.
- Alert on high failure rates, timeouts, or provider unhealthy states.

Documentation & training
------------------------
- Publish usage examples for agent authors.
- Provide debugging guidance: tracing, request/response dumps.
