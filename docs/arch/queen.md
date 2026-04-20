# Queen (Lifecycle Anchor & Spawning Factory)

Short summary
--

The Queen is the single authority responsible for the colony-level vitality signal ("Queen Substance") and for sanctioned creation and regulation of worker agents. Implementation: [src/core/queen/queen.h](src/core/queen/queen.h) and [src/core/queen/queen.c](src/core/queen/queen.c).

Responsibilities
--

- **Emit pheromone**: `hive_queen_emit_pheromone()` computes a rolling vitality checksum and propagates `vitality_seen` and `conditioned_ok` to worker cells.
- **Exclusive spawn factory**: `hive_queen_spawn()` is the only safe path to add entries to `d->agents[]`.
- **Receive grooming feedback**: `hive_queen_receive_groom()` updates per-worker `perf_score` and raises alarms/pheromones.
- **Populate regulation**: `hive_queen_regulate_population()` spawns workers based on backlog/demand and configured spawn ratio.
- **Re-queening**: `hive_queen_requeue_if_needed()` implements the Royal Jelly promotion protocol (promotes best worker when the Queen is weak).

Runtime flow and invariants
--

- The Queen is invoked from the dynamics tick. See `hive_dynamics_tick()` in [src/core/dynamics/dynamics.c](src/core/dynamics/dynamics.c) which calls the Queen in this order: emit pheromone → re-queue check → per-agent updates → demand-driven population regulation.
- Invariants:
  - Only `hive_queen_spawn()` may write to `d->agents[]` or increment `d->agent_count`.
  - `hive_queen_*` functions expect a valid `hive_dynamics_t *` and typically require `d->queen_alive` to be true to operate normally.
  - Re-queening increments `d->lineage_generation` and updates `d->queen_idx`.

Key types & references
--

- `hive_dynamics_t`, `hive_agent_cell_t`, `hive_groom_packet_t`, `hive_agent_traits_t` — see [src/core/dynamics/dynamics.h](src/core/dynamics/dynamics.h).
- Queen public API: [src/core/queen/queen.h](src/core/queen/queen.h).

Open questions / extension points
--

- Concurrency: current implementation is single-threaded; consider synchronization if the dynamics loop is exposed to multiple threads.
- Tunables: spawn demand ratio, requeue threshold, and lifecycle templates are runtime-configurable; documenting recommended values and trade-offs helps maintainers.

---

## Option 3 — Spawn Binding Invariant

With the worker-cell scheduler active (`HIVE_LEGACY_SCHEDULER=0`), every call
to `hive_queen_spawn()` must bind an agent descriptor to the new cell:

```c
const hive_agent_t *desc = hive_agent_descriptor_for_role(initial_role);
cell->bound_agent        = (desc != NULL) ? hive_agent_clone_descriptor(desc) : NULL;
cell->binding_generation = 1U;
```

The **scheduler** owns the lifetime of `bound_agent`:
- `retire_cell()` frees the pointer and zeros the slot when a cell transitions
  to `HIVE_ROLE_EMPTY` or `HIVE_ROLE_DRONE`.
- `hive_scheduler_deinit()` frees any remaining pointers.
- `hive_dynamics_deinit()` skips already-NULL slots.

### Re-queening rebind

When `hive_queen_requeue_if_needed()` promotes a worker to Queen:

```c
hive_agent_free(new_queen->bound_agent);             // free old (worker) descriptor
const hive_agent_t *qdesc = hive_agent_descriptor_for_role(HIVE_ROLE_QUEEN);
new_queen->bound_agent        = hive_agent_clone_descriptor(qdesc);
new_queen->binding_generation++;                     // lineage bump
```

The `binding_generation` counter lets the scheduler detect stale contexts
without walking the full slot table.
