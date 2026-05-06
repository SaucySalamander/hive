# Phase 2.5: Worker Coordination Signals

## Overview

Worker Coordination Signals enable autonomous task coordination without explicit task assignment. The hive uses two primary signal types:

1. **Waggle Dance Signals** — task priority broadcasting for worker task selection
2. **Alarm Signals** — error detection and worker quarantine for collision avoidance

## Waggle Dance Signals (Task Prioritization)

### Emission

The Queen emits waggle dance signals when work arrives:

```c
void hive_queen_emit_waggle(hive_dynamics_t *d)
```

**Trigger**: `d->demand_buffer_depth > 0` (new work in queue)

**Signal Properties**:
- `waggle_strength` [0-100]: Intensity of work demand (proportional to backlog depth)
- `waggle_direction`: Task type/priority hint (cycles through role types)
- `waggle_duration`: Remaining ticks before natural decay (8 ticks when active)

### Worker Response

Workers observe the waggle signal and adjust task selection priority:

1. **High waggle_strength** (>50): Worker prioritizes new task pickup
2. **Low waggle_strength** (<30): Worker defers task selection, focuses on existing work
3. **No waggle** (strength=0): Worker continues at baseline priority

### Decay Mechanism

Waggle signals decay naturally each tick:

- **When demand_buffer_depth > 0**: Signal refreshed, duration reset to 8
- **When demand_buffer_depth == 0**: Strength decays by 5 units/tick until 0
- **Duration countdown**: `waggle_duration--` each tick when no new demand

This ensures workers gradually deprioritize task pickup when backlog clears, preventing excessive context-switching.

## Alarm Signals (Collision Avoidance)

### Raising Alarms

Workers raise alarm flags in grooming packets when they encounter errors:

```c
hive_groom_packet_t pkt = {
    .alarm_flag = 1,  // Error detected
    .success = 0,     // Task failed
};
hive_queen_receive_groom(d, &pkt, log);
```

**Error conditions**:
- Task collision (two workers picked same task)
- Execution timeout
- Resource exhaustion
- Stage failure (returned non-OK status)

### Alarm Tracking

Queen tracks `consecutive_alarms` per worker:

```c
worker->consecutive_alarms++  // on alarm_flag=1
worker->consecutive_alarms=0  // on alarm_flag=0 (success)
```

### Worker Quarantine

**Threshold**: `HIVE_QUARANTINE_ALARM_THRESHOLD` (default 3)

**Quarantine Action**:
```c
if (worker->consecutive_alarms >= 3) {
    worker->conditioned_ok = false;  // Suppress worker
    worker->perf_score = 0;          // Mark failed
}
```

Quarantined workers:
- Cannot execute tasks (`conditioned_ok == false`)
- Visible in `stats.suppressed_workers`
- Remain quarantined until Queen manually resets or they're retired

### Alarm Decay

Alarms naturally decay when workers succeed:

```c
if (pkt->alarm_flag == 0) {           // No error this tick
    worker->consecutive_alarms = 0;   // Reset counter
    worker->perf_score += recovery;   // Performance improves
}
```

## Collision Avoidance Architecture

### How Waggle Prevents Task Collisions

**Scenario: Multiple workers, one available task**

1. **Worker A observes high waggle signal** → prioritizes task pickup
2. **Worker A claims task T** → emits signal "T is taken"
3. **Worker B observes waggle signal CONSUMED** → skips T, seeks next task
4. **No collision occurs** ✓

### Task Claim Mechanism

The waggle signal acts as a **distributed task registry**:

- **Waggle direction** = task type hint
- **Waggle strength** = task priority/urgency
- Workers observe waggle state and make independent decisions

**Implementation**:
1. When a worker picks a task, it consumes the current waggle signal strength
2. The next worker sees reduced waggle (decayed) and seeks different work
3. Within a single tick, only high-priority tasks see strong waggle

### Collision Detection & Recovery

If a collision occurs (both A and B pick T):

1. **Worker A executes T, emits `success=100`** → alarm_flag=0
2. **Worker B executes T, detects collision** → emits `success=0, alarm_flag=1`
3. **Queen receives B's alarm** → increments `consecutive_alarms`
4. **After N alarms** → Worker B quarantined
5. **Waggle signal refreshes** → available work resurfaces at high strength
6. **Next cycle** → healthy workers pick up failed tasks

## Integration Points

### Dynamics Tick Order

```
1. hive_queen_emit_pheromone()      — vitality control
2. hive_queen_emit_waggle()         — task prioritization
3. hive_queen_requeue_if_needed()   — promote best worker
4. Per-agent updates (age, perf drift)
5. hive_queen_regulate_population() — spawn on demand
```

### Worker Behavior Loop

```
Per-tick:
  1. Observe waggle_strength → adjust task priority
  2. Pick next task (bias towards high-waggle types)
  3. Execute task for 1-N ticks
  4. Emit grooming packet with success/alarm_flag
  5. Queen updates perf_score and consecutive_alarms
```

## Anti-Bloat Design

- **Reuses existing fields**: `active_alarms`, `conditioned_ok`, `consecutive_alarms`
- **Three new fields in stats**: `waggle_strength`, `waggle_direction`, `waggle_duration` (<100 bytes)
- **No new dependencies**: Uses only stdlib + existing dynamics/queen code
- **One new Queen function**: `hive_queen_emit_waggle()` (~30 LOC)
- **Total addition**: ~120 LOC across dynamics.c + queen.c

## Metrics & Observability

Stats exposed for UI/telemetry:

| Metric | Type | Meaning |
|--------|------|---------|
| `active_waggles` | unsigned | Current waggle signal count in colony |
| `waggle_strength` | uint32_t | [0-100] task priority intensity |
| `waggle_direction` | uint32_t | Task type hint (encodes role cycle) |
| `waggle_duration` | uint32_t | Ticks remaining for strong signal |
| `active_alarms` | unsigned | Current alarm activity level |
| `suppressed_workers` | unsigned | Cells where `conditioned_ok==false` |
| `consecutive_alarms` | uint8_t | Per-worker alarm counter (see groom) |

## Testing Strategy

**End-to-End Test**: `test_signal_collision_avoidance()`

1. Spawn multiple workers
2. Inject high demand (`demand_buffer_depth = 100`)
3. Verify waggle_strength rises to 100 (high priority)
4. Simulate 2 workers picking same task
5. Verify one worker raises alarm
6. Verify alarmed worker's `consecutive_alarms` increments
7. Repeat until quarantine threshold hit
8. Verify quarantined worker has `conditioned_ok==false`
9. Inject more demand (waggle refreshes)
10. Verify healthy workers resume work
11. Assert: **zero task collisions occurred** ✓

## Future Extensions

- **Density-based signal tuning**: adjust waggle strength based on worker count
- **Role-specific waggle**: separate signal per role (FORAGER, BUILDER, etc.)
- **Alarm dampening**: reduce false positives via confidence scoring
- **Historical waggle tracking**: log signal patterns for diagnostics
