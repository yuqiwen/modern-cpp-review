# 35. std::latch, std::barrier, and Phase Synchronization

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. std::latch](#2-stdlatch)
- [3. Waiting on a Latch](#3-waiting-on-a-latch)
- [4. Basic Latch Example](#4-basic-latch-example)
- [5. arrive_and_wait](#5-arrive_and_wait)
- [6. count_down Without Waiting](#6-count_down-without-waiting)
- [7. arrive_and_wait vs count_down + wait](#7-arrive_and_wait-vs-count_down--wait)
- [8. One-Shot Meaning](#8-one-shot-meaning)
- [9. std::barrier](#9-stdbarrier)
- [10. Barrier Is Reusable](#10-barrier-is-reusable)
- [11. Phase Synchronization](#11-phase-synchronization)
- [12. Completion Function](#12-completion-function)
- [13. Double Buffer Example](#13-double-buffer-example)
- [14. arrive_and_drop](#14-arrive_and_drop)
- [15. arrive vs arrive_and_wait](#15-arrive-vs-arrive_and_wait)
- [16. Latch vs Barrier](#16-latch-vs-barrier)
- [17. Latch vs condition_variable](#17-latch-vs-condition_variable)
- [18. Barrier vs condition_variable](#18-barrier-vs-condition_variable)
- [19. Latch vs semaphore](#19-latch-vs-semaphore)
- [20. Barrier vs semaphore](#20-barrier-vs-semaphore)
- [21. Incorrect Participant Count](#21-incorrect-participant-count)
- [22. Missing count_down on Latch](#22-missing-count_down-on-latch)
- [23. Exception Safety](#23-exception-safety)
- [24. Latch Does Not Transfer Results](#24-latch-does-not-transfer-results)
- [25. Memory Visibility](#25-memory-visibility)
- [26. False Assumption: Barrier Runs Threads Together](#26-false-assumption-barrier-runs-threads-together)
- [27. Common Interview Questions](#27-common-interview-questions)
- [28. Key Takeaways](#28-key-takeaways)

## Related Code Trap

- Demo file: [latch_barrier_phase_sync.cpp](../code_traps/latch_barrier_phase_sync.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

C++20 provides:

```cpp
std::latch
std::barrier
```

They synchronize groups of threads.

Use them when threads must wait for one another at a phase boundary.

Examples:

```text
wait until all workers finish initialization
wait until all tasks finish stage 1
start stage 2 only after everyone reaches the boundary
```

---

## 2. std::latch

Header:

```cpp
#include <latch>
```

A latch contains a counter.

Example:

```cpp
std::latch done(3);
```

Initial count:

```text
3
```

Each completed worker calls:

```cpp
done.count_down();
```

Count changes:

```text
3 -> 2 -> 1 -> 0
```

When count reaches zero, waiting threads are released.

---

## 3. Waiting on a Latch

```cpp
done.wait();
```

If count is greater than zero:

```text
current thread blocks
```

If count is already zero:

```text
wait returns immediately
```

After a latch reaches zero, it stays open permanently.

It cannot be reset.

---

## 4. Basic Latch Example

```cpp
std::latch done(3);

std::thread t1([&] {
    doWork1();
    done.count_down();
});

std::thread t2([&] {
    doWork2();
    done.count_down();
});

std::thread t3([&] {
    doWork3();
    done.count_down();
});

done.wait();

std::cout << "all workers finished\n";
```

The main thread waits until all three workers call `count_down()`.

---

## 5. arrive_and_wait

A latch also provides:

```cpp
latch.arrive_and_wait();
```

Conceptually:

```text
count_down()
then wait until count reaches zero
```

Example:

```cpp
std::latch startLine(4);
```

Four threads:

```cpp
startLine.arrive_and_wait();
```

Each thread arrives, decrements the count, then waits.

When the fourth arrives, all continue.

---

## 6. count_down Without Waiting

```cpp
latch.count_down();
```

This decrements the count but does not block the calling thread.

Useful when:

```text
worker finishes its responsibility
main thread is the only waiter
```

---

## 7. arrive_and_wait vs count_down + wait

These are similar:

```cpp
latch.arrive_and_wait();
```

and:

```cpp
latch.count_down();
latch.wait();
```

But `arrive_and_wait()` expresses the intent directly.

---

## 8. One-Shot Meaning

Once:

```text
latch count == 0
```

it remains zero.

You cannot do:

```cpp
latch.reset();
```

because no such operation exists.

If you need repeated phases, use:

```cpp
std::barrier
```

---

## 9. std::barrier

Header:

```cpp
#include <barrier>
```

A barrier synchronizes a fixed number of participating threads.

Example:

```cpp
std::barrier syncPoint(3);
```

Each thread calls:

```cpp
syncPoint.arrive_and_wait();
```

When all three arrive:

```text
all are released
barrier resets for the next phase
```

---

## 10. Barrier Is Reusable

Suppose three workers execute three rounds:

```cpp
for (int phase = 0; phase < 3; ++phase) {
    doPhaseWork(phase);
    barrier.arrive_and_wait();
}
```

For each phase:

```text
worker 1 arrives
worker 2 arrives
worker 3 arrives
all continue to next phase
```

Then the same barrier is reused.

---

## 11. Phase Synchronization

A barrier divides execution into phases.

Example:

```text
Phase 1:
all threads load data

barrier

Phase 2:
all threads process data

barrier

Phase 3:
all threads write results
```

No thread enters the next phase until all participating threads complete the current phase.

---

## 12. Completion Function

A barrier can run a completion function after all participants arrive.

Example:

```cpp
std::barrier barrier(
    3,
    [] {
        std::cout << "phase completed\n";
    }
);
```

The completion function runs once per phase before waiting threads continue.

Useful for:

```text
swap buffers
advance generation number
aggregate phase state
log phase completion
```

---

## 13. Double Buffer Example

Suppose simulation workers read from one buffer and write to another.

At the end of each phase:

```text
all workers must finish writing
then buffers swap
then next phase starts
```

A barrier completion function can perform:

```cpp
std::swap(currentBuffer, nextBuffer);
```

This ensures no worker starts the next phase before the swap.

---

## 14. arrive_and_drop

A barrier normally expects the same participant count each phase.

A thread can permanently stop participating:

```cpp
barrier.arrive_and_drop();
```

This means:

```text
arrive for current phase
reduce expected participant count for future phases
do not wait
```

Useful when a worker exits early.

---

## 15. arrive vs arrive_and_wait

A barrier also supports:

```cpp
auto token = barrier.arrive();
```

This registers arrival but does not necessarily wait immediately.

Later:

```cpp
barrier.wait(std::move(token));
```

Most code uses:

```cpp
arrive_and_wait()
```

because it is simpler.

---

## 16. Latch vs Barrier

### Latch

```text
one-shot
counter only decreases
does not reset
good for initialization or completion
```

Examples:

```text
wait until all workers initialize
wait until all jobs finish
release all threads once
```

### Barrier

```text
reusable
works in repeated phases
resets after every phase
good for iterative parallel algorithms
```

Examples:

```text
simulation steps
parallel matrix phases
multi-stage computation
```

---

## 17. Latch vs condition_variable

A latch is simpler when the condition is:

```text
N participants must arrive
```

With condition variable, you would manually write:

```cpp
mutex
condition_variable
counter
predicate
```

Latch already encapsulates that pattern.

But condition variable is more flexible for arbitrary predicates:

```text
queue not empty
stop requested
state == ready
```

---

## 18. Barrier vs condition_variable

Barrier is best when:

```text
same set of threads repeatedly synchronize at phase boundaries
```

Condition variable is best when:

```text
threads wait for arbitrary state changes
participant count may vary
one thread produces work for others
```

---

## 19. Latch vs semaphore

Semaphore models:

```text
available permits/resources/events
```

Latch models:

```text
remaining arrivals before one-time release
```

Semaphore can increase and decrease repeatedly.

Latch count only moves toward zero.

---

## 20. Barrier vs semaphore

Barrier does not limit how many threads enter a region.

It waits until all expected participants reach a point.

Semaphore limits concurrent access:

```text
at most N threads inside
```

Barrier synchronizes phases:

```text
all N threads must arrive before any continue
```

---

## 21. Incorrect Participant Count

This is dangerous:

```cpp
std::barrier barrier(4);
```

but only three threads ever call:

```cpp
arrive_and_wait();
```

Then all three wait forever.

The barrier expects a fourth arrival that never comes.

This is a logical deadlock.

---

## 22. Missing count_down on Latch

```cpp
std::latch done(3);
```

If one worker exits without calling:

```cpp
done.count_down();
```

then:

```cpp
done.wait();
```

may block forever.

Use RAII or carefully structured cleanup if exceptions are possible.

---

## 23. Exception Safety

Bad:

```cpp
doWork();          // may throw
done.count_down(); // skipped
```

If the thread catches the exception, it should still signal completion:

```cpp
try {
    doWork();
} catch (...) {
    // record error
}

done.count_down();
```

Or use a small RAII completion guard.

---

## 24. Latch Does Not Transfer Results

A latch only coordinates arrival.

It does not return values.

To transfer results, use:

```cpp
future/promise
shared data protected by synchronization
```

Example:

```text
workers write separate result slots
latch waits until all finish
main reads results after latch opens
```

---

## 25. Memory Visibility

Latch and barrier provide synchronization.

Writes performed before arrival become visible to threads that continue after the synchronization point according to the synchronization guarantees.

Example:

```cpp
results[index] = value;
done.count_down();
```

After:

```cpp
done.wait();
```

returns, the waiting thread can safely observe completed worker writes, assuming each worker writes to properly separated or synchronized locations.

---

## 26. False Assumption: Barrier Runs Threads Together

A barrier does not guarantee threads execute simultaneously afterward.

It only guarantees:

```text
none proceed past the barrier until all arrive
```

After release, scheduling is still controlled by the OS.

---

## 27. Common Interview Questions

### Q1. What is std::latch?

A latch is a one-shot countdown synchronization primitive. Threads decrement its counter, and waiters are released when the counter reaches zero.

### Q2. What is std::barrier?

A barrier is a reusable phase synchronization primitive. All participants must arrive before any proceed to the next phase.

### Q3. Difference between latch and barrier?

A latch is one-shot and cannot reset. A barrier is reusable across multiple phases.

### Q4. What happens if fewer threads arrive than expected?

Waiting threads may block forever because the count never reaches the required value.

### Q5. What is arrive_and_drop?

It marks the current thread as arrived and permanently reduces the number of expected participants in future barrier phases.

---

## 28. Key Takeaways

- `latch` is one-shot.
- `barrier` is reusable.
- Latch count moves only toward zero.
- Barrier resets after each phase.
- Use latch for initialization/completion.
- Use barrier for repeated parallel stages.
- Incorrect participant counts can deadlock.
- Latch/barrier coordinate threads but do not transfer results.
- They provide synchronization and memory visibility.
