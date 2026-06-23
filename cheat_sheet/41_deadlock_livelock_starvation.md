# 41. Deadlock, Livelock, Starvation, and Lock Ordering

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. Basic Deadlock](#2-basic-deadlock)
- [3. Circular Wait](#3-circular-wait)
- [4. Four Necessary Deadlock Conditions](#4-four-necessary-deadlock-conditions)
- [5. Global Lock Ordering](#5-global-lock-ordering)
- [6. Lock Hierarchy](#6-lock-hierarchy)
- [7. std::scoped_lock](#7-stdscoped_lock)
- [8. std::lock](#8-stdlock)
- [9. Same Mutex Twice](#9-same-mutex-twice)
- [10. recursive_mutex](#10-recursive_mutex)
- [11. Deadlock Through join](#11-deadlock-through-join)
- [12. Deadlock Through future::get](#12-deadlock-through-futureget)
- [13. Deadlock Through Callbacks](#13-deadlock-through-callbacks)
- [14. Open Calls](#14-open-calls)
- [15. Keep Critical Sections Small](#15-keep-critical-sections-small)
- [16. Bank Transfer Deadlock](#16-bank-transfer-deadlock)
- [17. Ordering by Object ID](#17-ordering-by-object-id)
- [18. Livelock](#18-livelock)
- [19. try_lock Livelock](#19-try_lock-livelock)
- [20. Livelock Mitigation](#20-livelock-mitigation)
- [21. Starvation](#21-starvation)
- [22. Fairness](#22-fairness)
- [23. Writer Starvation](#23-writer-starvation)
- [24. Lock-Free Starvation](#24-lock-free-starvation)
- [25. Priority Inversion](#25-priority-inversion)
- [26. Timeouts](#26-timeouts)
- [27. try_lock and Rollback](#27-try_lock-and-rollback)
- [28. Condition Variable Deadlocks](#28-condition-variable-deadlocks)
- [29. Shutdown Deadlocks](#29-shutdown-deadlocks)
- [30. Locking During Destruction](#30-locking-during-destruction)
- [31. Debugging Deadlocks](#31-debugging-deadlocks)
- [32. ThreadSanitizer Limitations](#32-threadsanitizer-limitations)
- [33. Common Wrong Patterns](#33-common-wrong-patterns)
- [34. Practical Locking Checklist](#34-practical-locking-checklist)
- [35. Common Interview Questions](#35-common-interview-questions)
- [36. Key Takeaways](#36-key-takeaways)

## Related Code Trap

- Demo file: [deadlock_livelock_starvation.cpp](../code_traps/deadlock_livelock_starvation.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

Concurrent programs can fail to make progress even when there is no data race.

Important progress problems include:

```text
deadlock
livelock
starvation
priority inversion
```

These problems are different.

```text
deadlock:
    threads wait forever for each other

livelock:
    threads keep reacting and retrying but make no progress

starvation:
    the system progresses, but one thread is repeatedly denied resources
```

---

## 2. Basic Deadlock

A classic deadlock uses two mutexes.

```cpp
std::mutex first;
std::mutex second;
```

Thread A:

```cpp
std::lock_guard lock1(first);
std::lock_guard lock2(second);
```

Thread B:

```cpp
std::lock_guard lock2(second);
std::lock_guard lock1(first);
```

Possible execution:

```text
Thread A locks first
Thread B locks second

Thread A waits for second
Thread B waits for first
```

Neither can continue.

---

## 3. Circular Wait

Deadlock often contains a cycle.

Example:

```text
Thread A holds mutex 1 and waits for mutex 2
Thread B holds mutex 2 and waits for mutex 3
Thread C holds mutex 3 and waits for mutex 1
```

This creates:

```text
A -> B -> C -> A
```

The cycle prevents progress.

---

## 4. Four Necessary Deadlock Conditions

Classic deadlock theory identifies four conditions.

### Mutual exclusion

A resource can be used by only one thread at a time.

### Hold and wait

A thread holds one resource while waiting for another.

### No preemption

A resource cannot be forcibly taken away.

### Circular wait

A cycle of threads waits for resources held by one another.

Preventing any one of these conditions prevents that deadlock pattern.

In practice, lock ordering commonly eliminates circular wait.

---

## 5. Global Lock Ordering

Define a consistent ordering for locks.

Example:

```text
mutex A before mutex B
mutex B before mutex C
```

Every thread must acquire locks in that order.

Correct:

```cpp
std::lock_guard lockA(mutexA);
std::lock_guard lockB(mutexB);
```

Every code path must use the same order.

Lock release naturally happens in reverse order through RAII destruction.

---

## 6. Lock Hierarchy

Large systems may assign each lock a hierarchy level.

Example:

```text
Level 1: global configuration lock
Level 2: cache lock
Level 3: individual object lock
```

Allowed acquisition:

```text
1 -> 2 -> 3
```

Forbidden acquisition:

```text
3 -> 2
2 -> 1
```

A lock hierarchy makes dependencies explicit.

Some codebases build debug wrappers that detect hierarchy violations.

---

## 7. std::scoped_lock

When multiple mutexes must be acquired together, use:

```cpp
std::scoped_lock lock(mutexA, mutexB);
```

`std::scoped_lock` uses a deadlock-avoidance locking strategy.

It also releases all owned mutexes automatically when leaving scope.

This is generally safer than:

```cpp
mutexA.lock();
mutexB.lock();
```

---

## 8. std::lock

A lower-level pattern is:

```cpp
std::unique_lock lockA(
    mutexA,
    std::defer_lock
);

std::unique_lock lockB(
    mutexB,
    std::defer_lock
);

std::lock(lockA, lockB);
```

`std::defer_lock` constructs lock objects without acquiring their mutexes.

`std::lock` then attempts to acquire all supplied locks using a deadlock-avoidance algorithm.

For straightforward code, prefer `std::scoped_lock`.

---

## 9. Same Mutex Twice

This is dangerous:

```cpp
std::mutex mutex;

mutex.lock();
mutex.lock();
```

A normal mutex is not recursive.

The second lock may block forever or otherwise violate the mutex requirements.

This can happen indirectly:

```cpp
void outer() {
    std::lock_guard lock(mutex);
    inner();
}

void inner() {
    std::lock_guard lock(mutex);
}
```

The same thread tries to acquire the same mutex again.

---

## 10. recursive_mutex

`std::recursive_mutex` permits the same thread to lock repeatedly.

```cpp
std::recursive_mutex mutex;

mutex.lock();
mutex.lock();

mutex.unlock();
mutex.unlock();
```

The number of unlocks must match the number of successful locks.

However, recursive mutexes can hide poor ownership or call-graph design.

They do not solve circular deadlocks involving different threads and different mutexes.

Use them only when recursive locking is genuinely part of the design.

---

## 11. Deadlock Through join

Deadlock can occur without two explicit mutex acquisitions.

Example:

```cpp
std::mutex mutex;

std::thread worker([&] {
    std::lock_guard lock(mutex);
    // finish work
});

{
    std::lock_guard lock(mutex);
    worker.join();
}
```

Execution:

```text
main holds mutex and waits for worker to finish
worker waits for mutex before it can finish
```

Both wait forever.

Avoid joining a thread while holding a lock that the thread may need.

---

## 12. Deadlock Through future::get

Similar problem:

```cpp
std::lock_guard lock(mutex);

future.get();
```

If the asynchronous task needs the same mutex before producing the future result:

```text
caller waits for future
worker waits for mutex
```

This deadlocks.

Do not hold locks across waits unless the dependency structure is proven safe.

---

## 13. Deadlock Through Callbacks

Dangerous:

```cpp
void notify() {
    std::lock_guard lock(mutex_);
    callback_();
}
```

The callback may:

* call back into the same object
* acquire another lock
* wait for another thread
* destroy the object
* execute user-provided code

This creates hidden lock dependencies.

Safer pattern:

```cpp
Callback callback;

{
    std::lock_guard lock(mutex_);
    callback = callback_;
}

callback();
```

Copy the callable or required state under the lock, then invoke it after unlocking.

---

## 14. Open Calls

Calling external or overridable code without holding internal locks is sometimes called an open call.

General pattern:

```text
lock
copy or update internal state
unlock
call external code
```

Open calls reduce hidden lock-order dependencies.

They may require careful handling because state can change after unlocking.

---

## 15. Keep Critical Sections Small

Bad:

```cpp
std::lock_guard lock(mutex);

performNetworkRequest();
performDiskIO();
runSlowComputation();
updateState();
```

The lock is held during slow operations.

Better:

```cpp
auto result = performSlowWork();

{
    std::lock_guard lock(mutex);
    updateState(result);
}
```

Smaller critical sections reduce:

* contention
* latency
* deadlock opportunities
* starvation risk

---

## 16. Bank Transfer Deadlock

Naive transfer:

```cpp
void transfer(
    Account& from,
    Account& to,
    int amount
) {
    std::lock_guard fromLock(from.mutex);
    std::lock_guard toLock(to.mutex);

    from.balance -= amount;
    to.balance += amount;
}
```

Two calls:

```text
transfer(accountA, accountB)
transfer(accountB, accountA)
```

may acquire locks in opposite order.

Safer:

```cpp
void transfer(
    Account& from,
    Account& to,
    int amount
) {
    if (&from == &to) {
        return;
    }

    std::scoped_lock lock(
        from.mutex,
        to.mutex
    );

    from.balance -= amount;
    to.balance += amount;
}
```

---

## 17. Ordering by Object ID

Another strategy is to order resources explicitly.

```cpp
Account* first = &from;
Account* second = &to;

if (first->id > second->id) {
    std::swap(first, second);
}

std::lock_guard firstLock(first->mutex);
std::lock_guard secondLock(second->mutex);
```

Every transfer acquires lower-ID account first.

This removes circular wait.

Be careful that IDs are unique and ordering is stable.

---

## 18. Livelock

Livelock occurs when threads are active but fail to make useful progress.

Real-world analogy:

```text
two people repeatedly step to the same side
then both step to the other side
neither passes
```

The system is not blocked, but it does not complete work.

---

## 19. try_lock Livelock

Example pattern:

```cpp
while (true) {
    if (first.try_lock()) {
        if (second.try_lock()) {
            break;
        }

        first.unlock();
    }
}
```

Another thread does the opposite order.

They may repeatedly:

```text
acquire one lock
fail the second lock
release
retry simultaneously
```

Neither finishes.

---

## 20. Livelock Mitigation

Possible techniques:

```text
consistent lock ordering
randomized backoff
exponential backoff
scheduler yield
central coordination
std::scoped_lock
```

Randomness or backoff breaks synchronized retry behavior.

However, fixed lock ordering is often the cleaner solution.

---

## 21. Starvation

Starvation means one thread waits for an unreasonably long time while others continue succeeding.

Examples:

```text
one thread repeatedly loses mutex acquisition
one CAS loop repeatedly fails
a writer is delayed by continuous readers
low-priority work is never scheduled
```

The program as a whole may still make progress.

---

## 22. Fairness

A fair lock or scheduler attempts to serve waiting threads in a predictable order, often close to FIFO.

Standard C++ mutexes do not provide a simple portable FIFO fairness guarantee.

Do not assume:

```text
the thread that waited longest will lock next
```

Fairness depends on:

```text
operating system
standard-library implementation
scheduler
contention pattern
```

---

## 23. Writer Starvation

With reader-writer locks:

```text
new readers keep arriving
writer waits for all readers to leave
```

If implementation favors readers, the writer may wait for a long time.

If implementation favors writers, readers may instead suffer.

The C++ standard should not be treated as guaranteeing one universal fairness policy.

---

## 24. Lock-Free Starvation

A lock-free CAS loop may allow one thread to fail repeatedly.

Other threads succeed, so the algorithm remains lock-free.

The unlucky thread has no individual progress guarantee.

This illustrates:

```text
lock-free does not mean starvation-free
```

Wait-free algorithms provide stronger guarantees.

---

## 25. Priority Inversion

Priority inversion occurs when:

```text
high-priority thread waits for a lock
low-priority thread owns the lock
medium-priority thread prevents low-priority thread from running
```

The high-priority thread indirectly waits behind medium-priority work.

Some systems support priority inheritance:

```text
temporarily raise the lock owner's priority
```

Portable standard C++ provides limited direct control over this OS scheduling behavior.

---

## 26. Timeouts

Timed mutexes can limit indefinite waiting.

Example:

```cpp
std::timed_mutex mutex;

if (
    mutex.try_lock_for(
        std::chrono::milliseconds(100)
    )
) {
    // protected work
    mutex.unlock();
} else {
    // timeout handling
}
```

Timeouts can provide:

* error recovery
* logging
* retry
* rollback
* deadlock detection clues

They do not replace correct lock design.

---

## 27. try_lock and Rollback

When manually acquiring multiple resources:

```text
acquire first
try second
if second fails:
    release first
    retry later
```

This avoids permanent blocking but can create livelock.

A backoff or centralized acquisition strategy may be required.

Prefer standard multi-lock utilities where possible.

---

## 28. Condition Variable Deadlocks

Incorrect:

```cpp
std::unique_lock lock(mutex);

while (!ready) {
    // forgot cv.wait(lock)
}
```

The thread holds the mutex while spinning.

The producer cannot acquire the mutex to set `ready`.

Correct:

```cpp
cv.wait(lock, [&] {
    return ready;
});
```

`wait` releases the mutex while sleeping.

---

## 29. Shutdown Deadlocks

A common shutdown error:

```text
destructor holds object mutex
destructor requests worker stop
destructor joins worker
worker needs object mutex to exit
```

Safer sequence:

```text
lock
set stop state
unlock
notify worker
join worker
```

Do not hold the worker's required mutex during join.

---

## 30. Locking During Destruction

Destructors in concurrent objects require careful ownership.

Before destroying synchronization primitives or shared state:

```text
no worker may still access the object
all threads should be stopped and joined
callbacks should be unregistered
```

Destroying a mutex while another thread may use it is undefined behavior.

---

## 31. Debugging Deadlocks

Useful techniques:

```text
thread dumps
debugger stack traces
lock-order logging
timeouts
watchdog threads
ThreadSanitizer
Helgrind
system profilers
```

For each blocked thread, inspect:

```text
which lock it waits for
which thread owns that lock
what that owner waits for
```

Follow dependencies until a cycle appears.

---

## 32. ThreadSanitizer Limitations

ThreadSanitizer is excellent for many data races.

It may detect some lock-order issues depending on environment and execution.

But deadlocks are timing-dependent and may not occur in every run.

Static lock-order discipline and code review remain important.

---

## 33. Common Wrong Patterns

### Opposite lock order

```text
path A locks m1 then m2
path B locks m2 then m1
```

### Holding lock while joining

```cpp
lock_guard lock(m);
worker.join();
```

### Holding lock while invoking callback

```cpp
lock_guard lock(m);
callback();
```

### Holding lock during slow I/O

```cpp
lock_guard lock(m);
networkRead();
```

### Acquiring the same normal mutex recursively

```cpp
m.lock();
m.lock();
```

### Assuming try_lock retry cannot fail forever

Repeated synchronized retries can livelock.

---

## 34. Practical Locking Checklist

Before adding multiple locks, ask:

```text
What invariant does each mutex protect?

Can one mutex protect the whole invariant?

What is the global acquisition order?

Can external code run while a lock is held?

Can a held lock cross join/get/wait/I/O?

Can the same mutex be acquired recursively?

Can a thread exit while owning the lock?

Is fairness important?

What is the shutdown order?

Can scoped_lock acquire the mutexes together?
```

---

## 35. Common Interview Questions

### Q1. What is deadlock?

Deadlock occurs when threads wait indefinitely for resources held by one another, so none can make progress.

### Q2. How can lock ordering prevent deadlock?

If every thread acquires multiple locks in the same global order, circular wait cannot form.

### Q3. What is livelock?

Livelock occurs when threads remain active and repeatedly react or retry, but no operation completes.

### Q4. What is starvation?

Starvation occurs when one thread is repeatedly denied access to a resource while other threads continue making progress.

### Q5. Why avoid callbacks while holding a lock?

The callback may acquire locks or call back into the object, creating hidden lock-order cycles or long critical sections.

### Q6. Does recursive_mutex solve all deadlocks?

No. It only permits the same thread to acquire the same mutex recursively. It does not solve cycles involving multiple mutexes or threads.

### Q7. Why should join not happen under a lock?

The joined thread may require that lock to complete, causing the joining thread and worker to wait for each other.

---

## 36. Key Takeaways

* Deadlock means threads permanently wait for one another.
* Circular wait is a common deadlock cause.
* Use consistent global lock ordering.
* Use `std::scoped_lock` for multiple mutexes.
* Avoid waiting, joining, blocking I/O, and callbacks while holding locks.
* Livelock means active retries without useful progress.
* Starvation means one thread lacks progress while others continue.
* Recursive mutex is not a general deadlock solution.
* Keep critical sections small.
* Design shutdown ordering carefully.
* Correct lock discipline is more important than adding timeouts after the fact.
