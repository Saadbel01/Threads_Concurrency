# Threads_concurrency

> Master the race for resources before the deadline masters you.

A multithreaded simulation of coders competing for a limited number of shared USB
dongles, written in C with POSIX threads. It is the classic **Dining Philosophers**
problem with two additions that make it considerably harder: a **cooldown** on every
released resource, and an **explicit arbitration policy** (FIFO or EDF) that the
program must implement itself rather than delegating to the operating system.

---

## Table of contents

- [Description](#description)
- [Instructions](#instructions)
- [Usage](#usage)
- [Output format](#output-format)
- [Architecture](#architecture)
- [Blocking cases handled](#blocking-cases-handled)
- [Thread synchronization mechanisms](#thread-synchronization-mechanisms)
- [Technical choices](#technical-choices)
- [Resources](#resources)

---

## Description

A number of coders sit in a circle in a shared co-working space. Between every pair of
neighbours lies one USB dongle, so with `N` coders there are exactly `N` dongles. To
compile, a coder must hold **two dongles simultaneously** — the one on their left and
the one on their right. Each coder then repeats the same cycle forever:

```
acquire 2 dongles  ->  COMPILE  ->  release both  ->  DEBUG  ->  REFACTOR  ->  repeat
                          |
                          +-- this is the only phase that resets the burnout timer
```

A coder **burns out** if more than `time_to_burnout` milliseconds elapse without them
*starting* a new compile. The clock is reset when a compile **begins**, not when it
ends. The simulation halts the instant one coder burns out, or once every coder has
completed `number_of_compiles_required` cycles.

### Goal

Write a program in which:

- every coder is a thread, and every dongle is protected by its own mutex;
- no **deadlock** is possible, even though every coder needs two resources at once
  around a circular table;
- no coder is **starved** of dongles by its neighbours;
- a released dongle respects a mandatory **cooldown** before it can be taken again;
- contention is resolved by a hand-written **priority queue** implementing either
  FIFO (arrival order) or EDF (earliest burnout deadline first);
- a separate **monitor thread** detects burnout and reports it within 10 ms;
- log lines are **serialised** so that two messages never interleave;
- all memory is freed and every mutex and condition variable is destroyed.

---

## Instructions

### Requirements

- A C compiler (`gcc` or `cc`) and `make`
- A POSIX system with the pthreads library (Linux or macOS)

### Compilation

```bash
make
```

Available rules:

| Rule | Effect |
| --- | --- |
| `make` / `make all` | Builds the `codexion` binary |
| `make clean` | Removes the object files |
| `make fclean` | Removes the object files and the binary |
| `make re` | `fclean` followed by `all` |

The project compiles with `-Wall -Wextra -Werror -pthread` and produces no warnings.
Every object file depends on `codexion.h`, so editing a struct recompiles exactly what
is affected and nothing more.

### Running

```bash
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

All eight arguments are mandatory.

| Argument | Meaning | Constraint |
| --- | --- | --- |
| `number_of_coders` | Number of coders, of dongles and of threads | `1 .. INT_MAX` |
| `time_to_burnout` | Deadline, measured from the **start** of the previous compile (ms) | `1 .. INT_MAX` |
| `time_to_compile` | Duration of a compile; both dongles are held throughout (ms) | `1 .. INT_MAX` |
| `time_to_debug` | Duration of the debug phase; no dongles held (ms) | `1 .. INT_MAX` |
| `time_to_refactor` | Duration of the refactor phase, after which the coder queues again (ms) | `1 .. INT_MAX` |
| `number_of_compiles_required` | Once every coder reaches this count, the simulation stops successfully | `1 .. INT_MAX` |
| `dongle_cooldown` | Time a released dongle stays unavailable (ms) | `0 .. INT_MAX` |
| `scheduler` | Arbitration policy | exactly `fifo` or `edf` |

Invalid input is rejected before a single byte is allocated and before any thread is
created: negative numbers, decimals, non-numeric text, a wrong argument count and any
scheduler string other than `fifo` or `edf` all cause an explanatory message and
`exit(1)`.

---

## Usage

```bash
# Comfortable parameters, even number of coders — should run to completion, no burnout
./codexion 4 410 200 100 100 10 0 edf

# Odd number of coders: not everyone can compile at once, so arbitration matters
./codexion 5 800 200 200 200 7 0 edf

# Tight deadline — compare the two policies on identical parameters
./codexion 4 310 200 100 100 10 0 fifo
./codexion 4 310 200 100 100 10 0 edf

# Cooldown active: watch the gaps where a dongle is free but nobody may take it yet
./codexion 3 600 200 100 100 5 150 edf

# Single-coder edge case: one dongle, so compiling is impossible — burns out at ~800 ms
./codexion 1 800 200 200 200 5 0 fifo

# Rejected by the parser
./codexion 2 0 200 200 200 5 0 edf
./codexion 4 410 200 100 100 10 0 lifo
```

Useful checks:

```bash
# No leaks, no invalid frees
valgrind --leak-check=full --show-leak-kinds=all ./codexion 4 410 200 100 100 5 0 edf

# No data races
valgrind --tool=helgrind ./codexion 4 410 200 100 100 5 0 edf
```

---

## Output format

Every state change is printed as `timestamp_in_ms coder_id message`, where the
timestamp is relative to the start of the simulation:

```
0 1 has taken a dongle
1 1 has taken a dongle
1 1 is compiling
201 1 is debugging
401 1 is refactoring
402 2 has taken a dongle
403 2 has taken a dongle
403 2 is compiling
1204 3 burned out
```

Two `has taken a dongle` lines always precede an `is compiling` line, and a
`burned out` line is always the **last** line of output.

---

## Architecture

Ten source files and one header, each with a single responsibility.

| File | Responsibility |
| --- | --- |
| `codexion.h` | All structs, constants and prototypes |
| `main.c` | Entry point: create threads, join them, destroy and free everything |
| `parser.c` | Validate and convert `argv`; exits before anything is allocated |
| `init.c` | Allocate and initialise shared state, dongles and coders, with rollback on failure |
| `coder.c` | The life of one coder: the compile / debug / refactor cycle |
| `dongle.c` | The acquisition algorithm — the core of the project |
| `dongle_utils.c` | Release, cooldown, queue push/remove, single-coder edge case |
| `heap.c` | The hand-written priority queue used by both schedulers |
| `monitor.c` | The watchdog thread: burnout and completion detection |
| `helpers.c` | Time, the stop-flag reader, the interruptible sleep, string utilities |
| `helpers2.c` | Serialised logging and the global wake-up broadcast |

### Shared state

Global variables are forbidden, so all shared state lives in a single heap-allocated
`t_shared` struct passed through the `void *` argument of `pthread_create`. Each
coder thread receives `&shared->coders[i]`, which carries a back-pointer to `shared`;
the monitor receives `shared` itself. Ownership therefore stays explicit, and which
lock guards which field is visible at a glance.

```
t_shared ── args             (read-only after parsing — needs no mutex)
         ├─ dongle_array[N]  (each: heap, is_held, available_at, its own mutex + cond)
         ├─ coders[N]        (each: compiles_done, last_compile_start, its own mutex)
         ├─ stop_simulation  (guarded by mutex_stop)
         ├─ log_mutex        (serialises stdout)
         └─ start_simulation (time origin for every printed timestamp)
```

### Thread layout

`N + 1` threads run concurrently:

```
coder_routine (x N)                        monitor_routine (every 1 ms)
-------------------                        ---------------------------
even ids sleep 1 ms   <- desynchronise      for each coder:
loop:                                           read compiles_done and
  if compiles_done >= required: break             last_compile_start under compile_lock
  acquire_dongles()        [may block]          if still short of quota and
  last_compile_start = now                         now - last_compile_start >= burnout
  "is compiling"   + sleep                           -> BURNOUT: print, stop, wake all
  release_dongles()                             else if quota reached
  "is debugging"   + sleep                           -> mark this coder finished
  "is refactoring" + sleep                    if every coder finished
  compiles_done++                                    -> stop quietly, wake all
```

---

## Blocking cases handled

This section lists every concurrency hazard the implementation addresses, and the
mechanism that addresses it.

### 1. Deadlock — Coffman's four conditions

Deadlock requires **all four** of the following to hold at once. The implementation
breaks two of them, so deadlock is structurally impossible rather than merely unlikely.

| Coffman condition | Status | Mechanism |
| --- | --- | --- |
| **Mutual exclusion** — one holder per dongle | Kept | It is the definition of the resource |
| **Hold and wait** — hold one dongle, wait for the second | **Broken** | `try_take_both()` takes both dongles or neither, in a single critical section holding both dongle mutexes. A coder is never observable holding exactly one dongle |
| **No pre-emption** — a dongle cannot be snatched mid-compile | Kept | Deliberate |
| **Circular wait** — 1 waits on 2, 2 on 3, …, N on 1 | **Broken** | `assign_dongles()` orders the two mutexes so the **lower dongle index is always locked first**. Every thread uses the same global lock order, so the lock graph is acyclic |

The naive approach — "pick up the left dongle, then wait for the right" — is exactly
the hold-and-wait condition, and with a circular table it deadlocks with every coder
holding one dongle. The common workaround (put the first one back and retry) trades
that deadlock for a **livelock** where everyone grabs and releases forever without
progress. All-or-nothing acquisition removes the failure mode instead of patching it.

### 2. Starvation and liveness

Deadlock freedom is not enough: two neighbours taking turns can leave the coder between
them waiting forever, which produces a burnout. Three properties combine to prevent it:

| Property | Guaranteed by | Consequence |
| --- | --- | --- |
| A claim is registered *before* waiting | `push_both_requests()` runs before the wait loop | A coder arriving later cannot silently overtake you |
| Only the queue's head may take the dongle | the `heap[TOP].coder_id == coder_id` test in `try_take_both()` | The OS wake-up order can never hand the resource to the wrong thread |
| Every waiter is woken on every state change | `pthread_cond_broadcast()` in `release_dongle()` | The rightful winner is never left asleep |

Under **EDF** there is a stronger guarantee: whenever two neighbours compete, the one
closer to burnout wins, which satisfies the subject's requirement that no coder be
starved and burn out under `edf` for feasible parameters. Under **FIFO** nobody waits
indefinitely either, but FIFO is fair rather than urgency-aware, so genuinely
infeasible parameters can still produce a burnout.

### 3. Lost wake-ups and spurious wake-ups

- Every `pthread_cond_wait` sits **inside a loop that re-tests the full condition from
  scratch**, never inside a bare `if`. A spurious wake-up simply re-evaluates and goes
  back to sleep.
- Nothing is remembered across a wait, so a coder that wakes to a changed situation
  never acts on stale information.
- On every stop path, the flag is raised **before** the broadcast
  (`stop_and_join_coders`, `handle_burnout`, `handle_completion`). Broadcasting first
  would let a thread wake, re-check a still-false condition, and sleep forever on a
  signal that has already been consumed.

### 4. Cooldown handling

A released dongle is not immediately reusable, so a coder can be first in the queue and
still not allowed to proceed. Two distinct kinds of "busy" are therefore tracked
separately: `is_held` (someone is compiling with it right now) and `available_at`
(it was put down recently and is still cooling). A dongle is takeable only when neither
is true.

The wait primitive is matched to the nature of the obstacle:

- **Cooldown** — a cooldown ends by itself at a known time and no thread will ever
  signal it, so `wait_dongle()` computes the exact remaining milliseconds and uses
  `pthread_cond_timedwait` with an absolute deadline. A plain `pthread_cond_wait` here
  would sleep straight past the moment the dongle became legal.
- **Contention** — this ends only when another coder releases, which is precisely what
  a condition variable is for, so `pthread_cond_wait` is used and consumes no CPU.

`compute_timeout()` carries nanoseconds into seconds when they exceed 1 000 000 000; an
out-of-range `tv_nsec` makes `pthread_cond_timedwait` fail immediately and silently
turns a timed wait into a busy loop.

### 5. Precise burnout detection

A burnout is the **absence** of an action, and absences cannot be signalled — the coder
about to burn out is asleep waiting for a dongle it will never get, so it cannot report
itself. Detection must therefore be external and time-driven, which is why the subject
mandates a separate monitor thread.

`monitor_routine` sweeps every coder each millisecond. That interval sits an order of
magnitude inside the 10 ms precision requirement while leaving the thread essentially
idle; a 100 µs loop would burn CPU and perturb the very timings it measures, and a
20 ms loop would miss the requirement. The precedence rule is enforced by the order of
the tests in `check_coder_burnout()`: a coder that has already met its required compile
count is never reported as burned out afterwards.

### 6. Log serialisation

Every coder writes to stdout only through `print_log()`, which holds `log_mutex` while
it **both** computes the timestamp and prints. Computing the timestamp inside the lock
is deliberate: it makes the printed timestamps monotonically non-decreasing and
consistent with the order the lines actually appear. Without serialisation, two
concurrent `printf` calls can genuinely produce half a line each.

`print_log()` also re-checks the stop flag inside the lock and prints nothing once the
simulation has ended.

### 7. Nested lock ordering on the stop path

`print_log()` and `handle_burnout()` both take `log_mutex` **first**, then `mutex_stop`.
Two nested locks acquired in inconsistent orders are the most common source of
real-world deadlocks; fixing one global order makes it impossible here. The order also
buys a feature: because `handle_burnout()` prints the burnout line and raises the stop
flag while holding the same `log_mutex`, the two are a single atomic event, so no coder
can slip a state line in afterwards. The `burned out` line is always the last line.

### 8. Graceful shutdown

Shutdown must reach threads in two completely different states:

- threads **asleep on a condition variable** — woken by `wake_all_dongles()`, which
  broadcasts on all `N` condition variables;
- threads **sleeping through a work phase** — `safe_sleep()` sleeps in 10 µs slices and
  re-checks the stop flag each slice, so it returns early instead of owing 150 ms of
  irrelevant sleep.

Handling only one of the two is the classic way to produce a program that prints the
burnout line and then hangs forever. A coder abandoning the queue also calls
`remove_both_requests()`, so no departed coder is left parked at the head of a queue,
blocking a neighbour that is still shutting down.

### 9. The `N == 1` edge case

With one coder there is one dongle, so compiling is physically impossible. This is
branched on **before** the general algorithm, because the two-dongle logic assumes two
*distinct* dongles: with `N == 1` the left and right dongle are the same object, and
locking it twice would be an immediate self-deadlock. `handle_single_coder()` takes the
dongle, logs `has taken a dongle` once, and waits for the inevitable burnout.

### 10. Partial-initialisation and partial-launch failures

- Every initialiser is transactional: if the *i*-th `pthread_mutex_init` fails,
  `destroy_dongles(array, i)` unwinds backwards over the *i* that already succeeded and
  returns `-1`. Half-initialised state is never handed to a caller — destroying an
  uninitialised mutex is undefined behaviour, not a harmless no-op.
- If thread *i* fails to spawn, `stop_and_join_coders()` raises the stop flag,
  broadcasts, and joins the *i* threads that did start. No orphan threads, no hang.
- `cleanup_shared()` is the exact mirror image of `init_shared()`, which is how the
  teardown is kept provably complete.

---

## Thread synchronization mechanisms

### Primitives used

| Primitive | Instances | Protects / signals |
| --- | --- | --- |
| `pthread_mutex_t lock` | one per dongle | That dongle's `heap`, `heap_size`, `is_held` and `available_at` |
| `pthread_cond_t cond` | one per dongle | "This dongle's state changed" — released, cooled down, or simulation over |
| `pthread_mutex_t compile_lock` | one per coder | That coder's `compiles_done` and `last_compile_start`, written by the coder and read by the monitor |
| `pthread_mutex_t mutex_stop` | one | The `stop_simulation` kill switch |
| `pthread_mutex_t log_mutex` | one | Serialises stdout, and makes "print burnout + stop" atomic |

**One mutex per dongle rather than one global table lock.** A single global lock would
serialise the whole simulation: coders 1 and 5 at opposite ends of a ten-person table
contend for nothing, yet would block each other on every attempt. Fine-grained locking
is what makes the program genuinely parallel. Its cost — having to hold two locks at
once, and therefore having to define a lock order — is paid exactly once, in
`assign_dongles()`.

**`args` carries no mutex** because it is written once by `get_arguments()` before any
thread exists and is read-only thereafter. Concurrent reads of immutable data are
always safe.

### How the dongles are acquired

```c
int acquire_dongles(t_coder *coder)
{
    if (coder->shared->args->nb_coders == 1)          /* (A) degenerate table    */
        return (handle_single_coder(coder));
    assign_dongles(coder, &d1, &d2);                  /* (B) lower index first   */
    push_both_requests(coder, d1, d2);                /* (C) queue on both       */
    while (!simulation_stopped(coder->shared))        /* (D) stop flag = exit    */
    {
        pthread_mutex_lock(&d1->lock);                /* (E) always d1 then d2   */
        pthread_mutex_lock(&d2->lock);
        if (try_take_both(coder, d1, d2))             /* (F) all-or-nothing      */
        {
            pthread_mutex_unlock(&d2->lock);
            pthread_mutex_unlock(&d1->lock);
            return (1);
        }
        if (/* d1 is the blocker */)                  /* (G) diagnose the blocker */
        {
            pthread_mutex_unlock(&d2->lock);
            wait_dongle(coder, d1);                   /* (H) sleep on d1          */
        }
        else
        {
            pthread_mutex_unlock(&d1->lock);
            wait_dongle(coder, d2);                   /* (I) sleep on d2          */
        }
    }
    remove_both_requests(coder, d1, d2);              /* (J) clean exit           */
    return (-1);
}
```

- **(B)** `d1` is always the lower-indexed dongle, so every thread acquires mutexes in
  the same global order and the lock graph cannot contain a cycle.
- **(C)** The request — carrying both `arrival_time` and `deadline` — enters both
  queues *before* the wait loop. This is the anti-starvation mechanism.
- **(F)** With both mutexes held, six conditions are checked and, if they all hold,
  both `heap_pop`s and both `is_held = 1` assignments happen with no window in between.
- **(G)** The code works out *which* dongle is blocking — not-my-turn, held, or still
  cooling — and sleeps on **that** dongle's condition variable, the one whose release
  will actually change the answer.
- **(H)/(I)** Only one mutex is held entering the wait, and `pthread_cond_wait` releases
  even that one. **A sleeping coder holds nothing**, so waiters never block neighbours.

### How race conditions are prevented

| Shared data | Race if unprotected | Protection |
| --- | --- | --- |
| `compiles_done`, `last_compile_start` | Written by the coder thread, read by the monitor every 1 ms. A torn or stale read yields a phantom burnout or a missed one | That coder's `compile_lock`, on both sides |
| `is_held`, `available_at`, `heap`, `heap_size` | Two neighbours could both conclude a dongle is free and both take it | That dongle's `lock`, held across the entire test-and-take in `try_take_both()` |
| `stop_simulation` | An unlocked read of a shared `int` is a data race even where the load looks atomic: the compiler may cache it in a register and never re-read it, producing an infinite loop that only appears with optimisations on | `mutex_stop`, always via `simulation_stopped()` — one function, so the discipline applies at every call site for free |
| `stdout` | Two `printf` calls can interleave into half a line each | `log_mutex`, held across timestamp computation *and* printing |

### Thread-safe communication between coders and the monitor

Communication is deliberately one-directional and minimal, which is what keeps it
simple enough to reason about:

- **Coder → monitor**: coders publish `last_compile_start` and `compiles_done` under
  their own `compile_lock`. The monitor only ever reads them, under the same lock. The
  monitor never touches a dongle, a heap or a condition variable belonging to the
  acquisition layer.
- **Monitor → coders**: the monitor raises the single `stop_simulation` flag under
  `mutex_stop` and then calls `wake_all_dongles()`. Coders observe it in three places —
  the `acquire_dongles` loop condition, the checks between phases in `coder_cycle`, and
  inside `safe_sleep` — so the stop reaches a coder whatever it is currently doing.

The `stop_simulation` flag doubles as a broadcast channel: a single boolean plus a
broadcast on every condition variable is enough to unwind `N + 1` threads from any
state they can be in, without any thread needing to know what the others are doing.

### Arbitration: FIFO and EDF

Each dongle owns a hand-written priority heap of pending requests. The subject forbids
any library priority queue, and delegating to the OS is not an option either: the
wake-up order of `pthread_cond_signal` is unspecified and, in practice, unfair — the
same thread frequently wins repeatedly and starves its neighbour into a burnout.

`is_higher_priority()` is the only function in the entire program that knows what the
policies mean:

```c
FIFO -> a.arrival_time < b.arrival_time      /* serve in arrival order            */
EDF  -> a.deadline     < b.deadline          /* deadline = last_compile_start
                                                        + time_to_burnout          */
```

Isolating the policy in one comparator means adding a third scheduler is a one-line
change there plus the string match in the parser; nothing else in the program knows
which policy is active.

The heap holds at most **two** entries, and that bound is proven rather than assumed: a
dongle physically sits between exactly two neighbours, so no third coder can ever
request it. A two-element binary heap collapses into "keep the better one at index 0",
so the sift-up and sift-down reduce to a single conditional swap. The array is embedded
in `t_dongle`, so there is nothing to `malloc` and nothing to leak. The comparator is
still written generically, so the structure could be grown if the model changed.

**Why broadcast rather than signal.** Two neighbours may be waiting on the same dongle
and they are not interchangeable — the heap, not the kernel, decides which one is
entitled to it. `pthread_cond_signal` could wake the *wrong* one, who would see it is
not at the head of the queue and go back to sleep, leaving the rightful winner asleep
through the whole event. `pthread_cond_broadcast` wakes everyone and lets the heap
arbitrate.

---

## Technical choices

| Choice | Rationale |
| --- | --- |
| One mutex + cond per dongle | Genuine parallelism; distant coders never block each other |
| Take both dongles atomically | Eliminates hold-and-wait instead of patching it, and avoids the livelock of put-it-back schemes |
| Lower dongle index locked first | A single global lock order makes the lock graph acyclic |
| Explicit per-dongle priority queue | Deterministic, inspectable arbitration; correctness no longer depends on the kernel scheduler |
| 2-element heap | The exact bound imposed by the table's topology, not a shortcut |
| Polling monitor thread at 1 ms | A burnout is the absence of an event and cannot be signalled; 1 ms sits well inside the 10 ms requirement while the thread stays idle |
| `safe_sleep` in 10 µs slices | Makes work phases interruptible so shutdown is prompt and no stale line is printed after the burnout line |
| `cond_timedwait` for cooldown, `cond_wait` for contention | Matches the primitive to the nature of the wait: precision without spinning |
| Dongles released right after compiling | They are only needed *during* a compile; holding them through debug and refactor would reduce the table to one active coder |
| Single `t_shared` struct | No globals; ownership and lock/field pairing stay visible, and the code stays reentrant |
| `log_mutex` before `mutex_stop`, everywhere | Consistent nested-lock order, and it makes the burnout line atomically the last line |
| 1 ms stagger for even-numbered coders | A contention heuristic that lets the table settle into its natural alternating rhythm. It is an **optimisation, not a correctness mechanism** — the deadlock and starvation guarantees hold without it |
| Validate everything up front | Failing before the first `malloc` and the first thread means the error path has nothing to clean up, so `exit(1)` is genuinely safe there |

### Known trade-offs

Stated plainly rather than glossed over:

- `safe_sleep()` polls a mutex-protected flag roughly every 10 µs. This is a deliberate
  trade of a small amount of CPU for a prompt, clean shutdown; the program is not
  entirely free of polling.
- The monitor thread polls at 1 ms rather than being event-driven. This is inherent to
  deadline detection, not an implementation shortcut.
- The 1 ms stagger in `coder_routine` is a heuristic. It measurably reduces burnouts for
  tight parameters but plays no part in any correctness argument.

---

## Resources

### Concurrency and synchronisation

- Dijkstra, E. W., *Hierarchical Ordering of Sequential Processes* (1971) — the original
  Dining Philosophers problem and the resource-hierarchy solution used here.
- Coffman, E. G., Elphick, M., Shoshani, A., *System Deadlocks* (1971) — the four
  necessary conditions for deadlock referenced throughout this README.
- Silberschatz, Galvin, Gagne, *Operating System Concepts* — chapters on synchronisation
  and deadlock.
- Butenhof, D. R., *Programming with POSIX Threads* — the standard reference for
  mutexes, condition variables and the predicate-loop idiom.
- Downey, A., *The Little Book of Semaphores* — classic synchronisation patterns.

### Real-time scheduling

- Liu, C. L., Layland, J. W., *Scheduling Algorithms for Multiprogramming in a Hard
  Real-Time Environment* (1973) — the optimality proof for Earliest Deadline First.
- Buttazzo, G., *Hard Real-Time Computing Systems* — EDF in practice, and deadline
  feasibility.

### Documentation

- `man 3 pthread_create`, `pthread_join`, `pthread_mutex_init`, `pthread_cond_wait`,
  `pthread_cond_timedwait`, `pthread_cond_broadcast`
- `man 2 gettimeofday`, `man 3 clock_gettime`, `man 3 usleep`
- The POSIX.1-2017 specification for the `pthread_*` interfaces.

### Tools

- `valgrind --leak-check=full` for memory
- `valgrind --tool=helgrind` and `-fsanitize=thread` for data races

### Use of AI

AI tools were used in a bounded way, and every suggestion was reviewed, tested and
reworked before being kept:

- **Explaining concepts** — clarifying the semantics of `pthread_cond_timedwait`'s
  absolute-deadline argument and the reasons a condition wait must always sit inside a
  predicate loop.
- **Reviewing the locking discipline** — as a second pair of eyes on the ordering of
  `log_mutex` and `mutex_stop`, and on whether any code path could hold two dongle
  mutexes in inconsistent order.
- **Documentation** — drafting and structuring this README and the accompanying
  walkthrough document.
- **Test parameters** — suggesting argument combinations likely to expose starvation
  and to make the FIFO / EDF difference observable.

AI was **not** used to generate the acquisition algorithm, the heap, the monitor or the
synchronisation design. Those were designed, written and debugged directly, which is
what makes it possible to defend every decision above.

