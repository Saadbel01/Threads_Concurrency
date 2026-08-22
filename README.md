# Threads Concurrency

## Description

Threads Concurrency simulates a group of coders sharing a limited set of USB dongles in a circular co-working hub. Each coder is represented by an independent thread that cycles through three phases: **compiling**, **debugging**, and **refactoring**. Compiling requires two dongles held simultaneously (one "left", one "right"), and there are exactly as many dongles as coders, arranged so that dongle `k` sits between coder `k` and coder `k+1` (with wraparound).

The goal is to keep every coder compiling regularly — a coder that goes too long without starting a new compile **burns out**, which stops the simulation. The project models this as a synchronization problem close to the classic Dining Philosophers problem, extended with:

- **Dongle cooldown** — a released dongle is not immediately reusable.
- **Deterministic arbitration** — contested dongles are granted according to a **FIFO** or **EDF** (Earliest Deadline First) policy, backed by a custom-built priority queue (no standard library structure).
- **Precise burnout detection** — a dedicated monitor thread must detect and log a burnout within 10ms of it actually occurring.

The simulation stops either when a coder burns out, or when every coder has completed at least `number_of_compiles_required` compiles.

## Instructions

### Compilation

```
make          # builds the codexion binary
make clean    # removes object files
make fclean   # removes object files and the binary
make re       # fclean + all
```

The Makefile compiles with `cc`, `-Wall -Wextra -Werror -pthread`.

### Usage

```
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

All arguments are mandatory, positive integers (`0` is rejected), except `scheduler`, which must be exactly `fifo` or `edf`.

Example:
```
./codexion 4 800 200 200 200 5 50 fifo
```

Note: `number_of_coders = 1` is explicitly rejected. With a single coder there is only one dongle on the table, and compiling requires two dongles held *simultaneously* — a single physical dongle cannot satisfy that requirement, so this configuration is treated as unsupported rather than silently reinterpreted.

## Resources

- POSIX Threads Programming (Lawrence Livermore National Laboratory tutorial) — `pthread_create`, `pthread_join`, `pthread_mutex_t` fundamentals.
- `man` pages for `pthread_mutex_init`, `pthread_cond_wait`, `pthread_cond_timedwait`, `gettimeofday`, `clock_gettime`.
- Dijkstra's Dining Philosophers problem and the Coffman conditions for deadlock, as general background for the resource-sharing design.
- Earliest Deadline First (EDF) scheduling, as a general real-time scheduling concept, for the EDF arbitration policy.

### AI usage disclosure

An AI assistant (Claude) was used throughout this project as a design-review and debugging partner, not as a code generator. The workflow was: I wrote an implementation attempt myself, the assistant reviewed it line by line, explained *why* something was incorrect (a race condition, an off-by-one, a deadlock scenario, a platform-specific bug), and I rewrote the code myself based on that explanation. Concretely, AI was used for:

- Explaining concurrency concepts (mutexes, condition variables, `pthread_cond_wait` vs `pthread_cond_timedwait`, Coffman's conditions, deadlock/starvation reasoning) before I implemented them myself.
- Reviewing my own code for bugs across every module (argument parsing, struct design, dongle acquisition/release, the coder and monitor thread loops, the FIFO/EDF heap) and explaining the reasoning behind each fix rather than supplying the fix directly.
- Helping design a systematic debugging process (adding temporary instrumentation, bisecting where a hang occurred) that led to identifying a real platform-specific bug: `long` is 32 bits on Windows/MinGW but 64 bits on Linux, which caused an integer overflow in millisecond timestamps and a resulting deadlock — fixed by switching to `long long` throughout.
- Discussing design trade-offs (e.g., scoping the scheduler per-dongle rather than globally, since each dongle can have at most two possible requesters given the seating layout) where I proposed the idea and the assistant helped verify and refine it.

I wrote every line of the final source code myself and can explain the purpose of every mutex, condition variable, and synchronization decision in it.

## Blocking cases handled

**Deadlock prevention (Coffman's conditions).** A coder always acquires its two dongles in a fixed order determined by comparing their numeric IDs — the lower-numbered dongle is always locked first, regardless of whether it is the coder's "left" or "right" dongle. This breaks the *circular wait* condition: since every coder in the ring follows the same global ordering rule, it is structurally impossible for a cycle of coders to each hold one dongle while waiting on another held by the next coder in the cycle. The single-dongle case (`number_of_coders = 1`) is handled separately by rejecting that configuration outright, since it would otherwise require a coder to lock its own already-held dongle a second time.

**Starvation prevention.** Dongle contention is never resolved by whichever thread the OS happens to wake first. Each dongle owns a small priority queue (a real heap, with sift-up/sift-down operations) holding the pending requests for that dongle — capped at two entries, since the seating layout guarantees a dongle can never be contested by more than two coders. Under `fifo`, the coder that requested earliest is served first; under `edf`, the coder with the earliest burnout deadline (`last_compile_start + time_to_burnout`) is served first. Both comparators include an explicit tie-breaker (coder ID) for the rare case of identical timestamps, ensuring the policy is fully deterministic.

**Cooldown handling.** After release, a dongle is marked unavailable until `dongle_cooldown` milliseconds have elapsed (`available_at = now + cooldown`), independently of whether it is currently held. A coder waiting purely for a cooldown to expire — with no other coder about to release or signal anything — is not woken by a signal (since nothing else will happen); instead it uses `pthread_cond_timedwait` with a deadline computed from `available_at`, so it wakes itself exactly when the cooldown ends and re-checks its condition.

**Precise burnout detection.** A dedicated monitor thread polls every coder's `last_compile_start` roughly every millisecond, comparing it against `time_to_burnout`. This interval was chosen to comfortably satisfy the required 10ms logging precision without excessive lock contention against the coder threads.

**Log serialization.** All log output is protected by a single dedicated mutex, held only for the duration of the `printf` call itself, guaranteeing that two threads' log lines can never interleave mid-line.

**Clean shutdown from any state.** When the simulation stops (burnout or completion), the responsible thread sets a shared `stop_simulation` flag under its own mutex, then broadcasts on every dongle's condition variable. This is necessary because a coder waiting for a dongle is asleep in `pthread_cond_wait` and cannot notice the flag on its own; the broadcast wakes every waiting coder so it can re-check the flag and exit cleanly, releasing any partially-acquired dongle first if needed, rather than hanging forever.

## Thread synchronization mechanisms

| Mechanism | Protects | Notes |
|---|---|---|
| One `pthread_mutex_t` per dongle | that dongle's `is_held`, `available_at`, and heap state | Fine-grained: locking one dongle never blocks unrelated dongles. |
| One `pthread_cond_t` per dongle | signals dongle release / wakes waiters | Paired with the dongle's own mutex; `pthread_cond_timedwait` used specifically for the cooldown-only wait case. |
| One `pthread_mutex_t` per coder (`compile_lock`) | that coder's `last_compile_start` and `compiles_done` | Written by the coder's own thread, read by the monitor thread — a genuine cross-thread race without this lock. Held only for the instant of the read/write, never across a sleep, so the monitor can inspect a coder mid-compile. |
| `mutex_stop` | the shared `stop_simulation` flag | Written by the monitor (or by `main` on a thread-creation failure), read by every coder after each phase. |
| `log_mutex` | all `stdout` output | Held only for the duration of one `printf` call. |

**Example of a race condition prevented.** Without `compile_lock`, the monitor thread could read `last_compile_start` at the exact moment a coder thread is mid-write to it, observing a torn or stale value — potentially causing a false burnout report or, worse, missing a genuine one. Because the write in `coder_routine` and the read in `monitor_routine` both go through the same mutex, one of the two always fully completes before the other begins.

**Example of thread-safe coordination.** When a coder releases a dongle, `release_dongle` locks the dongle's mutex, updates its state, and calls `pthread_cond_signal` while still holding the lock, then unlocks. A different coder waiting in `pthread_cond_wait` on that same mutex/condition pair is guaranteed to either already be asleep (and receive the signal) or not yet have reached the wait call (and will see the updated state on its own next check) — there is no window in which a wakeup can be sent and missed.