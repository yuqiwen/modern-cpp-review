# 33. Atomic Memory Ordering

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. Atomicity vs Visibility](#2-atomicity-vs-visibility)
- [3. Compiler and CPU Reordering](#3-compiler-and-cpu-reordering)
- [4. Sequential Consistency](#4-sequential-consistency)
- [5. memory_order_relaxed](#5-memory_order_relaxed)
- [6. relaxed Does Not Publish Other Data](#6-relaxed-does-not-publish-other-data)
- [7. Release Operation](#7-release-operation)
- [8. Acquire Operation](#8-acquire-operation)
- [9. Release-Acquire Pair](#9-release-acquire-pair)
- [10. Synchronizes-With](#10-synchronizes-with)
- [11. Happens-Before](#11-happens-before)
- [12. memory_order_acq_rel](#12-memory_order_acq_rel)
- [13. compare_exchange](#13-compare_exchange)
- [14. compare_exchange_weak vs strong](#14-compare_exchange_weak-vs-strong)
- [15. CAS Loop Example](#15-cas-loop-example)
- [16. memory_order_consume](#16-memory_order_consume)
- [17. seq_cst vs acquire/release](#17-seq_cst-vs-acquirerelease)
- [18. Mutex Already Provides Ordering](#18-mutex-already-provides-ordering)
- [19. Atomic Counter](#19-atomic-counter)
- [20. Atomic Flag Publication](#20-atomic-flag-publication)
- [21. atomic::wait and notify](#21-atomicwait-and-notify)
- [22. Atomic Is Not Enough for Compound Invariants](#22-atomic-is-not-enough-for-compound-invariants)
- [23. Common Incorrect Assumption](#23-common-incorrect-assumption)
- [24. Data Race With Non-Atomic Data](#24-data-race-with-non-atomic-data)
- [25. Reference Counts](#25-reference-counts)
- [26. Performance Warning](#26-performance-warning)
- [27. Common Interview Questions](#27-common-interview-questions)
- [28. Key Takeaways](#28-key-takeaways)

## Related Code Trap

- Demo file: [atomic_memory_order.cpp](../code_traps/atomic_memory_order.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

`std::atomic<T>` provides:

```text
atomic operations
inter-thread synchronization
memory ordering rules
```

Example:

```cpp
std::atomic<int> counter = 0;
counter.fetch_add(1);
```

The increment is atomic.

But when atomics are used to publish other non-atomic data, memory ordering becomes important.

---

## 2. Atomicity vs Visibility

Suppose:

```cpp
int data = 0;
std::atomic<bool> ready = false;
```

Producer:

```cpp
data = 42;
ready.store(true);
```

Consumer:

```cpp
if (ready.load()) {
    std::cout << data;
}
```

Question:

```text
If consumer sees ready == true,
must it also see data == 42?
```

The answer depends on memory ordering.

Atomicity of `ready` alone does not automatically explain visibility of `data`.

---

## 3. Compiler and CPU Reordering

Compilers and CPUs may reorder operations as long as single-thread behavior appears unchanged.

Source code:

```cpp
data = 42;
ready = true;
```

Another thread may not necessarily observe those writes in the same apparent order without synchronization.

Reasons include:

- compiler optimization
- CPU store buffers
- cache coherence timing
- out-of-order execution

C++ memory ordering defines what reorderings are allowed.

---

## 4. Sequential Consistency

Default atomic operations use:

```cpp
std::memory_order_seq_cst
```

Example:

```cpp
ready.store(true);
bool value = ready.load();
```

Equivalent explicitly:

```cpp
ready.store(true, std::memory_order_seq_cst);
ready.load(std::memory_order_seq_cst);
```

Sequential consistency provides the strongest and easiest mental model.

All seq_cst operations appear to participate in one global total order.

Use it by default unless there is a measured reason to weaken ordering.

---

## 5. memory_order_relaxed

```cpp
counter.fetch_add(1, std::memory_order_relaxed);
```

Relaxed guarantees:

```text
the atomic operation itself is atomic
```

It does not establish ordering for other memory operations.

Good for:

- statistics counters
- approximate metrics
- independent reference-like counters in carefully designed code
- cases where only the final atomic value matters

Example:

```cpp
std::atomic<int> requests = 0;

void recordRequest() {
    requests.fetch_add(1, std::memory_order_relaxed);
}
```

No other data is being published through this counter.

---

## 6. relaxed Does Not Publish Other Data

Producer:

```cpp
data = 42;
ready.store(true, std::memory_order_relaxed);
```

Consumer:

```cpp
if (ready.load(std::memory_order_relaxed)) {
    std::cout << data;
}
```

Even if consumer sees `ready == true`, relaxed ordering does not establish synchronization for `data`.

If `data` is concurrently accessed without proper synchronization, this may be a data race.

Relaxed atomic does not act like a mutex.

---

## 7. Release Operation

A release operation publishes earlier writes.

Producer:

```cpp
data = 42;
ready.store(true, std::memory_order_release);
```

Meaning:

```text
writes before the release store cannot move after it
```

Conceptually:

```text
finish writing data
then publish ready = true
```

Release is commonly used on the producer side.

---

## 8. Acquire Operation

An acquire operation receives published writes.

Consumer:

```cpp
if (ready.load(std::memory_order_acquire)) {
    std::cout << data;
}
```

Meaning:

```text
operations after the acquire load cannot move before it
```

If the acquire load reads the value written by the release store, earlier producer writes become visible to the consumer.

Acquire is commonly used on the consumer side.

---

## 9. Release-Acquire Pair

Correct publication pattern:

```cpp
int data = 0;
std::atomic<bool> ready = false;
```

Producer:

```cpp
data = 42;
ready.store(true, std::memory_order_release);
```

Consumer:

```cpp
if (ready.load(std::memory_order_acquire)) {
    std::cout << data; // sees 42
}
```

If acquire sees the release-written `true`, then:

```text
producer writes before release
happen-before
consumer reads after acquire
```

This is the key synchronization relationship.

---

## 10. Synchronizes-With

If:

```cpp
atomic.store(value, memory_order_release);
```

and another thread:

```cpp
atomic.load(memory_order_acquire);
```

reads that stored value, then the release:

```text
synchronizes-with
```

the acquire.

This creates:

```text
happens-before
```

for surrounding operations.

---

## 11. Happens-Before

`happens-before` is a formal guarantee.

If operation A happens-before operation B, B must observe the effects of A according to the C++ memory model.

In publication:

```text
data = 42
    sequenced-before
ready.store(true, release)

ready.load(acquire) reads true
    sequenced-before
read data
```

Therefore:

```text
write data happens-before read data
```

---

## 12. memory_order_acq_rel

Used for read-modify-write operations that both acquire and release.

Example:

```cpp
state.fetch_add(1, std::memory_order_acq_rel);
```

It combines:

```text
acquire semantics for reads
release semantics for writes
```

Common with:

- `fetch_add`
- `exchange`
- successful `compare_exchange`
- lock-free state transitions

---

## 13. compare_exchange

Compare-and-swap pattern:

```cpp
int expected = 0;

bool success = state.compare_exchange_strong(
    expected,
    1
);
```

Conceptually:

```text
if state == expected:
    state = 1
    return true
else:
    expected = state
    return false
```

Important detail:

On failure, `expected` is updated with the actual value.

---

## 14. compare_exchange_weak vs strong

### strong

```cpp
compare_exchange_strong
```

Fails only when the value comparison fails.

### weak

```cpp
compare_exchange_weak
```

May fail spuriously even when values match.

Therefore weak is normally used in a loop:

```cpp
int expected = 0;

while (!state.compare_exchange_weak(expected, 1)) {
    expected = 0;
}
```

On some architectures, weak may be more efficient.

---

## 15. CAS Loop Example

Atomic maximum:

```cpp
void updateMax(std::atomic<int>& maxValue, int candidate) {
    int current = maxValue.load();

    while (
        candidate > current &&
        !maxValue.compare_exchange_weak(current, candidate)
    ) {
        // current is updated with actual value after failure
    }
}
```

The loop retries only while candidate is still larger than current.

---

## 16. memory_order_consume

C++ defines:

```cpp
std::memory_order_consume
```

But in practice, implementations generally treat it like acquire.

It is subtle and rarely used directly.

For interview and normal code:

```text
know it exists
prefer acquire
```

---

## 17. seq_cst vs acquire/release

### seq_cst

```text
strongest simple global ordering
easiest to reason about
default
```

### acquire/release

```text
orders only the needed producer-consumer relationship
can allow more optimization
harder to reason about
```

Correct acquire/release code can be faster on some architectures, but correctness complexity increases.

---

## 18. Mutex Already Provides Ordering

When one thread unlocks a mutex and another later locks the same mutex:

```text
writes before unlock become visible after lock
```

So mutex provides:

- mutual exclusion
- acquire/release-like synchronization
- memory visibility

You usually do not combine manual memory-order reasoning with ordinary mutex-protected data.

---

## 19. Atomic Counter

For a simple counter:

```cpp
std::atomic<int> counter = 0;

counter.fetch_add(1, std::memory_order_relaxed);
```

Relaxed is often enough if:

```text
counter does not publish other data
only the numeric count matters
```

Example:

```cpp
requestsProcessed.fetch_add(1, relaxed);
```

But a shutdown flag protecting other state may require stronger ordering.

---

## 20. Atomic Flag Publication

Correct:

```cpp
Payload payload;
std::atomic<bool> ready = false;
```

Producer:

```cpp
payload.initialize();
ready.store(true, std::memory_order_release);
```

Consumer:

```cpp
while (!ready.load(std::memory_order_acquire)) {
    std::this_thread::yield();
}

use(payload);
```

This is synchronization, but it is still spinning.

For long waits, condition variable or atomic wait is better.

---

## 21. atomic::wait and notify

C++20 supports:

```cpp
atomic.wait(oldValue);
atomic.notify_one();
atomic.notify_all();
```

Example:

```cpp
std::atomic<bool> ready = false;

ready.wait(false);
```

This waits while the value remains `false`.

Producer:

```cpp
ready.store(true, std::memory_order_release);
ready.notify_one();
```

Consumer:

```cpp
ready.wait(false, std::memory_order_acquire);
```

This can avoid manual busy spinning.

---

## 22. Atomic Is Not Enough for Compound Invariants

Suppose:

```cpp
std::atomic<int> balance;
std::atomic<int> transactionCount;
```

You need:

```text
balance and transactionCount updated together consistently
```

Separate atomics do not make the pair one atomic transaction.

Another thread may observe:

```text
new balance
old transactionCount
```

Use a mutex when multiple values form one invariant.

---

## 23. Common Incorrect Assumption

Incorrect:

```text
Every atomic operation makes all other variables safe.
```

Correct:

```text
Atomic protects its own operation.
Other data requires a valid synchronization relationship.
```

Release/acquire can publish other data, but only when designed correctly.

---

## 24. Data Race With Non-Atomic Data

This is safe:

```cpp
int data;
std::atomic<bool> ready;
```

only if access to `data` is properly ordered:

```cpp
producer writes data
producer release-stores ready
consumer acquire-loads ready
consumer reads data
```

If producer continues modifying `data` while consumer reads it, the release/acquire publication is not enough.

The object must not be concurrently modified unsafely.

---

## 25. Reference Counts

Reference counters often use atomics, but ordering requirements differ by operation.

Incrementing an already-live shared reference may use relaxed ordering.

Decrementing the last reference requires stronger synchronization before destruction.

This is one reason implementing `shared_ptr` correctly is difficult.

Do not build custom reference counting casually.

---

## 26. Performance Warning

Weaker memory ordering does not automatically mean dramatically faster.

On some CPUs:

- relaxed/acquire/release may compile similarly
- seq_cst may add extra barriers or restrictions
- x86 has relatively strong hardware ordering
- ARM has weaker ordering and shows bigger differences

Always prioritize correctness and measure before optimizing.

---

## 27. Common Interview Questions

### Q1. What is memory_order_relaxed?

It guarantees atomicity for that atomic object but does not establish ordering or visibility for unrelated memory operations.

### Q2. What is release-acquire synchronization?

A release operation publishes prior writes. An acquire operation that reads from that release makes those writes visible to the acquiring thread.

### Q3. What is happens-before?

It is a formal ordering guarantee that effects of one operation are visible to another operation.

### Q4. Why use seq_cst by default?

It provides the strongest and simplest ordering model, reducing reasoning complexity and concurrency bugs.

### Q5. When is relaxed appropriate?

For independent atomic counters or statistics where the operation does not publish or synchronize other data.

### Q6. Is atomic enough for multiple related variables?

No. Multiple atomics do not automatically make a compound invariant atomic. Use a mutex if values must change together.

---

## 28. Key Takeaways

- Atomicity and memory ordering are different.
- Default atomics use sequential consistency.
- Relaxed guarantees atomicity only.
- Release publishes previous writes.
- Acquire observes writes published by release.
- A matching release/acquire pair creates happens-before.
- `acq_rel` is used for read-modify-write operations.
- Mutexes already provide synchronization and visibility.
- Use relaxed only for truly independent atomic state.
- Prefer seq_cst until weaker ordering is justified.
- Multiple atomic variables do not create one transaction.
