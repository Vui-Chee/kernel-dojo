# Rate Monotonic Scheduler

Kernel Version: v6.18.6

A Linux kernel module implementing a userspace Rate Monotonic Scheduler (RMS) via a procfs interface. Tasks register their period and processing time, and the scheduler assigns CPU priority according to the rate-monotonic policy: shorter period = higher priority.

## Architecture

### ProcFS Interface (`/proc/monotonic_sched/status`)

Userspace communicates through a single procfs file supporting three operations:

- `R,<pid>,<period>,<processing_time>` — register a task
- `Y,<pid>` — yield (task finished its work for this period)
- `D,<pid>` — deregister a task

Reading the file returns all registered tasks with their period and processing time.

### Process Control Block

Each registered task is tracked with `struct task`, wrapping the kernel `task_struct` with RMS-specific state:

- **State machine**: `SLEEPING → READY → RUNNING → SLEEPING`
- **Wakeup timer**: fires at the start of each new period, transitions the task to READY
- **Reference count** (`kref`): prevents use-after-free when the dispatcher and deregistration race

### Admission Control

Tasks are admitted only if the total CPU utilization satisfies the RMS bound:

```
sum(C_i / P_i) <= 0.693
```

Implemented entirely in fixed-point integer arithmetic (no floating point). The check and commit happen atomically under `admission_lock` to prevent TOCTOU races when multiple tasks register concurrently. Failed allocations after admission roll back the utilization sum.

### Dispatching Thread

A kernel thread wakes on every timer expiry or yield event and selects the highest-priority READY task (shortest period). It uses `sched_setattr_nocheck` to assign `SCHED_FIFO/99` to the selected task and demote the preempted task to `SCHED_NORMAL/0`.

Before releasing `processes_lock`, both the current task and best candidate are pinned with `kref_get` to prevent concurrent deregistration from freeing them mid-dispatch.

State transitions that affect scheduling decisions (e.g. `RUNNING → READY` on preemption) are performed inside the lock. A snapshot of the pre-transition state is taken to drive the out-of-lock scheduling actions, avoiding the window where a re-check of `->state` would see the already-updated value.

### Yield

When a task calls yield, the kernel:

1. Sets its state to `SLEEPING` under `processes_lock`
2. Computes `next_release = last_release + period` and arms the wakeup timer
3. Updates `last_release = next_release` to anchor future periods (prevents drift)
4. Wakes the dispatcher, then calls `set_current_state(TASK_KILLABLE) + schedule()`

`TASK_KILLABLE` is used instead of `TASK_UNINTERRUPTIBLE` so the task can be reaped by a signal if the module is removed while tasks are sleeping.

## Concurrency Design

| Shared resource | Lock |
|---|---|
| `processes` list, task state | `processes_lock` (spinlock, `_bh`) |
| `admission_sum` | `admission_lock` (spinlock, `_bh`) |
| `task_struct` lifetime | `kref` + `get_task_struct` / `put_task_struct` |

**Timer handler** runs in softirq context and uses `spin_lock` (not `_bh`) since softirqs are already disabled at that point.

**UAF prevention**: The `kref` on `struct task` ensures memory is not freed while the dispatcher holds a pointer to it. `get_task_struct` is called inside `rcu_read_lock` to safely increment the kernel task's refcount before the RCU grace period ends.

**Deregistration** removes the task from the list under `processes_lock`, cancels the timer with `timer_delete_sync` to drain any pending callbacks, then drops the list's kref — freeing memory only after all other holders release their references.
