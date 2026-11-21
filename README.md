*This project has been created as part of the 42 curriculum by <taalmeid>.*

# Codexion

## Description

Codexion is an advanced variation of the classic "Dining Philosophers" problem, adapted for the 42 curriculum. Instead of philosophers who eat, sleep, and think, we have "coders" who compile, debug, and refactor, using "dongles" instead of forks.

Coders sit around a circular co-working hub. There are as many USB dongles as coders, one between each pair of neighbours. To compile, a coder must hold **two** dongles at the same time. After compiling they put the dongles back, debug, refactor, and try to compile again. A coder who does not **start** a compile within `time_to_burnout` ms of the start of their previous compile (or of the simulation) burns out, and the simulation stops.

The goal is to write a multithreaded C program (POSIX threads) that:

- keeps every coder alive when the parameters are feasible,
- never deadlocks and never starves a coder,
- respects a mandatory dongle cooldown,
- arbitrates conflicts with a configurable scheduler (`fifo` or `edf`) built on a hand-written priority queue (min-heap),
- detects burnout within 10 ms and prints perfectly serialized logs.

## Instructions

### Compilation

```
make        # builds ./codexion
make clean  # removes object files
make fclean # removes objects and the binary
make re     # fclean + all
```

Compiled with `cc -Wall -Wextra -Werror -pthread`.

### Usage

```
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

| Argument | Meaning | Valid values |
|---|---|---|
| `number_of_coders` | Number of coders (and dongles) | integer ≥ 1 |
| `time_to_burnout` | ms allowed between two compile starts | integer ≥ 1 |
| `time_to_compile` | ms a coder holds two dongles | integer ≥ 1 |
| `time_to_debug` | ms spent debugging | integer ≥ 0 |
| `time_to_refactor` | ms spent refactoring | integer ≥ 0 |
| `number_of_compiles_required` | Simulation ends when every coder compiled this many times | integer ≥ 1 |
| `dongle_cooldown` | ms a released dongle stays unavailable | integer ≥ 0 |
| `scheduler` | Arbitration policy | exactly `fifo` or `edf` |

All arguments are mandatory. Negative numbers, non-integers, overflowing values and any other scheduler are rejected with an error on `stderr`.

### Example

```
$ ./codexion 1 800 200 200 200 5 0 fifo
0 1 has taken a dongle
801 1 burned out
```

With a single coder there is only one dongle, so the coder can never compile and burns out (timestamps vary by a millisecond or two).

Log format: `timestamp_in_ms coder_id message`, where the message is one of `has taken a dongle`, `is compiling`, `is debugging`, `is refactoring`, `burned out`.

## Blocking cases handled

**Deadlock prevention (Coffman's conditions).** A deadlock needs four conditions at once. Three of them cannot be removed here and are accepted:
*mutual exclusion* (a dongle is used by one coder at a time), *hold and wait* (a coder keeps its first dongle while waiting for the second) and *no preemption* (a dongle is never taken from a coder). We break the fourth, **circular wait**: dongles are numbered, coder `i` uses dongles `i` and `(i + 1) % n`, and every coder always requests the dongle with the **lower index first**. Since all coders acquire resources in the same global order, a cycle of waiting coders is impossible. This also covers `n = 2`, where both coders share the same two dongles.

**Starvation prevention.** Each dongle owns a priority queue (min-heap) of pending requests. With `fifo` the key is the arrival sequence number. With `edf` the key is the coder's burnout deadline (`last_compile_start + time_to_burnout`), so the coder closest to burning out is served first. Ties are broken deterministically by arrival sequence and then by coder id. Under `edf`, and with feasible parameters, no coder is starved.

**Dongle cooldown.** On release, a dongle stores `available_at = now + dongle_cooldown`. A waiter at the head of the queue only takes the dongle when it is free *and* the cooldown is over. If the dongle is free but cooling down, the waiter sleeps with `pthread_cond_timedwait` until `available_at`, because nobody would signal the end of a cooldown.

**Precise burnout detection.** A dedicated monitor thread polls every 1 ms, reads each coder's `last_compile_ms` under that coder's lock and declares burnout when `now - last_compile_ms > time_to_burnout`. The 1 ms period keeps the burnout log well within the required 10 ms.

**Log serialization.** All output goes through one `log_mutex`, one `printf` per line, so lines never interleave. Once the simulation is stopped no further state message is printed, so nothing appears after `burned out`. The burnout timestamp is taken while holding the log mutex, so timestamps in the log never go backwards.

**Single coder.** With one coder there is one dongle. The coder takes it, logs it, and waits; the monitor reports the burnout. The coder never tries to lock the same dongle twice.

**Clean shutdown / lost wakeups.** When the simulation stops, coders may be blocked in `pthread_cond_wait`. The monitor sets the stop flag and then locks each dongle mutex and broadcasts its condition variable. Taking the mutex first guarantees the broadcast cannot happen between a coder checking the flag and going to sleep, so no wakeup is lost and every thread can be joined.

**No false burnouts.** Coders do not stop by themselves after reaching `number_of_compiles_required`. They keep cycling until the monitor stops the simulation, so a coder that finished early is never reported as burned out while slower coders are still working.

## Thread synchronization mechanisms

| Primitive | Protects | Used by |
|---|---|---|
| `pthread_mutex_t` in each dongle | `in_use`, `available_at_ms`, the request queue, `next_seq` | coders (acquire/release), monitor (wake) |
| `pthread_cond_t` in each dongle | Waiting for "my turn and dongle free and cooldown over" | coders wait, release/push/monitor broadcast |
| `pthread_mutex_t lock` in each coder | `last_compile_ms`, `compiles_done` | the coder writes, the monitor reads, EDF reads the deadline |
| `state_mutex` | the `stop` flag | everyone reads it, the monitor writes it |
| `log_mutex` | `stdout` and the ordering "no print after stop" | coders and monitor |

No custom event implementation is needed: the stop flag plus a broadcast on each dongle condition variable plays that role.

**Lock hierarchy.** Mutexes are only ever taken in these orders: `log → state`, `dongle → state`, `dongle → coder lock`. Nothing goes in the opposite direction, so the locks themselves cannot deadlock. The monitor releases `log` and `state` before touching any dongle mutex.

**Race conditions prevented (examples).**

- *Two coders grabbing the same dongle:* the check "I am at the head of the queue, the dongle is not `in_use` and the cooldown is over", the pop, and `in_use = 1` all happen with the dongle mutex held. `pthread_cond_wait` releases the mutex only while sleeping and re-acquires it before returning, and the condition is re-checked in a loop, so spurious wakeups are harmless.
- *Monitor vs. coder on `last_compile_ms`:* both access it only under the coder's lock, so the monitor never reads a half-updated value.
- *Printing after burnout:* `stop` is only written while holding `log_mutex`, and `log_state` checks it while holding the same mutex, so no line can slip in between the decision to stop and the `burned out` message.
- *Waiting coders at shutdown:* handled by the locked broadcast described above.

**Coder ↔ monitor communication.** Coders publish their progress (`last_compile_ms`, `compiles_done`) under their own lock. The monitor reads it, and when it decides to stop it sets `stop`, prints the burnout message if needed, and wakes every dongle. Coders see the flag through `sim_is_stopped`, which takes `state_mutex`, and every wait they do (`sleep_ms`, dongle waits) checks it.

## Technical choices

- **Clock:** `clock_gettime(CLOCK_MONOTONIC)` is used for all simulation time, so system clock adjustments cannot cause false burnouts. `pthread_cond_timedwait` expects an absolute `CLOCK_REALTIME` deadline, so it is built from the remaining *relative* wait.
- **Per-dongle sequence number:** the queue is per dongle, so arrival order only has to be comparable inside one dongle. This avoids a global counter and its lock.
- **Fixed-size heap:** a dongle is shared by two coders at most, so its queue never holds more than 2 requests and the heap is allocated once (no `realloc`).
- **`sleep_ms`:** sleeps in slices of at most 1 ms against an absolute end time, so there is no cumulative drift and coders react quickly to the stop flag.
- **No global variables;** all state lives in a `t_sim` structure passed to every thread.
- Values above 1 000 000 000 are rejected at parsing to avoid overflow when computing deadlines.

## Resources

**References**

- `man 3 pthread_mutex_lock`, `pthread_cond_wait`, `pthread_cond_timedwait`, `clock_gettime`
- Remzi & Andrea Arpaci-Dusseau, *Operating Systems: Three Easy Pieces*, concurrency chapters: https://pages.cs.wisc.edu/~remzi/OSTEP/
- Blaise Barney, *POSIX Threads Programming* (LLNL): https://hpc-tutorials.llnl.gov/posix/
- E. G. Coffman et al., System Deadlock*, 1971 (the four deadlock conditions)
- C. L. Liu & J. W. Layland, *Scheduling Algorithms for Multiprogramming in a Hard-Real-Time Environment*, 1973 (Earliest Deadline First)
- Cormen et al., *Introduction to Algorithms*, chapter on heaps and priority queues
- Allen B. Downey, *The Little Book of Semaphores* (Dining Philosophers)

**Use of AI**

AI (Claude) was used as a tutor and reviewer, not as a code generator to copy from:

- *Planning:* breaking the subject into steps (parsing, time, heap, dongles, coders, monitor, logging, README).
- *Review:* finding bugs and risks in my own drafts (uninitialized variable, missing wakeups at shutdown, false burnout after a coder finishes, sleep drift, single-coder case), and checking Norm constraints (function length, functions per file, allowed functions).
- *Suggestions:* some corrected functions came from AI suggestions; I reviewed, tested, and can explain each one.
- *Documentation:* help structuring this README; the content was checked against the final code.
- *Testing:* ideas for test cases and for using Valgrind and ThreadSanitizer.
