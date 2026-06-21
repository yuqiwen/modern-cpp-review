# 39. False Sharing and Cache Lines

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. What Is a Cache Line?](#2-what-is-a-cache-line)
- [3. CPU Cache Hierarchy](#3-cpu-cache-hierarchy)
- [4. A False-Sharing Example](#4-a-false-sharing-example)
- [5. Cache-Line Ping-Pong](#5-cache-line-ping-pong)
- [6. Why Is It Called False Sharing?](#6-why-is-it-called-false-sharing)
- [7. False Sharing Is Not a Data Race](#7-false-sharing-is-not-a-data-race)
- [8. Atomic Does Not Prevent False Sharing](#8-atomic-does-not-prevent-false-sharing)
- [9. Fixing False Sharing With Padding](#9-fixing-false-sharing-with-padding)
- [10. alignas](#10-alignas)
- [11. Standard Interference Sizes](#11-standard-interference-sizes)
- [12. Destructive Interference](#12-destructive-interference)
- [13. Constructive Interference](#13-constructive-interference)
- [14. Per-Thread Counters](#14-per-thread-counters)
- [15. Sharded Counters](#15-sharded-counters)
- [16. thread_local as an Alternative](#16-thread_local-as-an-alternative)
- [17. False Sharing With Non-Atomic Variables](#17-false-sharing-with-non-atomic-variables)
- [18. Read-Only Sharing](#18-read-only-sharing)
- [19. False Sharing in Thread Pools](#19-false-sharing-in-thread-pools)
- [20. False Sharing in Queues](#20-false-sharing-in-queues)
- [21. False Sharing in ML and AI Infrastructure](#21-false-sharing-in-ml-and-ai-infrastructure)
- [22. Padding Is Not Always Good](#22-padding-is-not-always-good)
- [23. Alignment Does Not Guarantee Everything](#23-alignment-does-not-guarantee-everything)
- [24. Measuring False Sharing](#24-measuring-false-sharing)
- [25. Benchmarking Warnings](#25-benchmarking-warnings)
- [26. Memory Order and False Sharing](#26-memory-order-and-false-sharing)
- [27. NUMA Considerations](#27-numa-considerations)
- [28. Structure of Arrays vs Array of Structures](#28-structure-of-arrays-vs-array-of-structures)
- [29. Common Wrong Assumptions](#29-common-wrong-assumptions)
- [30. Practical Checklist](#30-practical-checklist)
- [31. Common Interview Questions](#31-common-interview-questions)
- [32. Key Takeaways](#32-key-takeaways)

## Related Code Trap

- Demo file: [false_sharing.cpp](../code_traps/false_sharing.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

False sharing happens when multiple threads modify different variables that happen to reside on the same cache line.

Example:

```cpp
struct Counters {
    std::atomic<long long> first{0};
    std::atomic<long long> second{0};
};
```

Suppose:

```text
Thread A modifies first
Thread B modifies second
```

The threads do not logically share the same variable.

There is no data race because both variables are atomic.

However, if `first` and `second` reside in the same cache line, the cache line may repeatedly move between CPU cores.

This can cause severe performance degradation.

```text
Correct result does not imply good performance.
```

---

## 2. What Is a Cache Line?

CPUs usually move data between memory and cache in fixed-size blocks called cache lines.

A common cache-line size is:

```text
64 bytes
```

Example memory layout:

```text
cache line:

| first | second | remaining bytes... |
```

Even if each variable is only 8 bytes, both may belong to the same 64-byte cache line.

Cache coherence protocols normally track ownership and validity at the cache-line level, not at the individual-variable level.

---

## 3. CPU Cache Hierarchy

A typical CPU has:

```text
registers
L1 cache
L2 cache
L3 cache
main memory
```

Each CPU core often has private L1 and L2 caches.

Several cores may share a larger L3 cache.

When a core modifies data, the cache coherence system ensures other cores do not continue using stale copies.

The coherence granularity is normally the cache line.

---

## 4. A False-Sharing Example

```cpp
struct Counters {
    std::atomic<long long> first{0};
    std::atomic<long long> second{0};
};

Counters counters;
```

Thread A:

```cpp
for (...) {
    counters.first.fetch_add(
        1,
        std::memory_order_relaxed
    );
}
```

Thread B:

```cpp
for (...) {
    counters.second.fetch_add(
        1,
        std::memory_order_relaxed
    );
}
```

Logically:

```text
Thread A owns first
Thread B owns second
```

Physically, both may occupy one cache line.

---

## 5. Cache-Line Ping-Pong

Suppose both variables are in cache line X.

Thread A runs on Core 1.

Thread B runs on Core 2.

Possible sequence:

```text
Core 1 obtains write ownership of line X
Core 2's copy of line X is invalidated

Core 2 obtains write ownership of line X
Core 1's copy of line X is invalidated

Core 1 obtains ownership again
Core 2 loses ownership again
```

This repeats for every update.

The cache line effectively bounces between cores.

This is often called:

```text
cache-line ping-pong
```

The program spends substantial time on coherence traffic instead of useful computation.

---

## 6. Why Is It Called False Sharing?

True sharing means threads actually access the same variable.

Example:

```cpp
std::atomic<int> counter;
```

Multiple threads do:

```cpp
counter.fetch_add(1);
```

They truly share one object.

False sharing means threads access different variables:

```cpp
std::atomic<int> first;
std::atomic<int> second;
```

But those variables share the same cache line.

The sharing is false at the program-data level but real at the hardware cache-line level.

---

## 7. False Sharing Is Not a Data Race

A data race is a C++ correctness error.

Example:

```cpp
int counter = 0;
```

Two threads execute:

```cpp
++counter;
```

without synchronization.

That is undefined behavior.

False sharing is different:

```cpp
std::atomic<int> first;
std::atomic<int> second;
```

Each thread modifies a separate atomic.

The behavior may be fully correct.

The problem is performance.

```text
Data race:
    correctness problem

False sharing:
    hardware-performance problem
```

---

## 8. Atomic Does Not Prevent False Sharing

Atomic operations guarantee properties such as:

```text
no data race on the atomic object
indivisible atomic update
defined synchronization semantics
```

Atomic operations do not guarantee:

```text
good cache layout
no cache-line contention
good scalability
```

Two independent atomic variables in the same cache line may still cause false sharing.

Therefore:

```text
thread-safe does not necessarily mean scalable
```

---

## 9. Fixing False Sharing With Padding

The usual solution is to separate frequently written variables onto different cache lines.

Example:

```cpp
struct alignas(64) PaddedCounter {
    std::atomic<long long> value{0};
};
```

Then:

```cpp
struct Counters {
    PaddedCounter first;
    PaddedCounter second;
};
```

Conceptually:

```text
cache line 1:
| first.value | padding... |

cache line 2:
| second.value | padding... |
```

Now each core can modify its own line without constantly invalidating the other core's line.

---

## 10. alignas

`alignas` requests a particular alignment.

Example:

```cpp
struct alignas(64) Counter {
    std::atomic<long long> value{0};
};
```

This requests that each `Counter` object begin at an address aligned to 64 bytes.

If the object size is also at least 64 bytes, consecutive objects are likely to occupy separate cache lines.

Example:

```cpp
Counter counters[4];
```

Each element can be separated from the others.

---

## 11. Standard Interference Sizes

C++17 provides:

```cpp
std::hardware_destructive_interference_size
std::hardware_constructive_interference_size
```

Header:

```cpp
#include <new>
```

`hardware_destructive_interference_size` represents a recommended separation to avoid destructive cache interference.

Example:

```cpp
struct alignas(
    std::hardware_destructive_interference_size
) Counter {
    std::atomic<long long> value{0};
};
```

This is preferable to hardcoding 64 when supported.

However, compiler and standard-library support may vary.

---

## 12. Destructive Interference

Destructive interference occurs when data used independently by different threads is placed too close together.

Example:

```text
Thread A frequently writes field A
Thread B frequently writes field B
A and B share a cache line
```

The result is cache invalidation and ownership transfer.

False sharing is a common form of destructive interference.

---

## 13. Constructive Interference

Sometimes placing data close together is beneficial.

Suppose one thread frequently reads:

```cpp
struct Point {
    float x;
    float y;
    float z;
};
```

If `x`, `y`, and `z` are usually accessed together, storing them in the same cache line improves locality.

This is constructive interference.

General principle:

```text
data accessed together should often be stored together

independently written cross-thread data should often be separated
```

---

## 14. Per-Thread Counters

A common false-sharing pattern is one counter per worker.

Potentially bad layout:

```cpp
struct Stats {
    std::atomic<long long> worker0{0};
    std::atomic<long long> worker1{0};
    std::atomic<long long> worker2{0};
    std::atomic<long long> worker3{0};
};
```

All counters may fit inside one cache line.

Each worker updates its own field, but the line still bounces between cores.

Improved layout:

```cpp
struct alignas(64) Counter {
    std::atomic<long long> value{0};
};

std::array<Counter, 4> counters;
```

Each worker updates one separated counter.

Later:

```cpp
long long total = 0;

for (const Counter& counter : counters) {
    total += counter.value.load(
        std::memory_order_relaxed
    );
}
```

---

## 15. Sharded Counters

Instead of one global atomic:

```cpp
std::atomic<long long> globalCounter;
```

use several counters:

```cpp
std::array<Counter, ShardCount> shards;
```

Each thread updates one shard.

Later, aggregate all shards.

Advantages:

```text
less contention
less cache-line bouncing
better scalability
```

Tradeoffs:

```text
aggregation cost
more memory
value may not be instantly exact unless aggregated
```

---

## 16. thread_local as an Alternative

Another approach is:

```cpp
thread_local long long localCounter = 0;
```

Each thread updates its own counter.

No atomic operation may be necessary for the local update.

Later, results are merged.

Advantages:

```text
no shared write on hot path
no atomic increment cost
no false sharing between thread-local instances
```

Challenges:

```text
collecting values from all threads
thread-exit handling
thread-pool lifetime
aggregation timing
```

---

## 17. False Sharing With Non-Atomic Variables

False sharing is not limited to atomics.

Example:

```cpp
struct Data {
    int first;
    int second;
};
```

If Thread A writes `first` and Thread B writes `second`, the two fields are distinct memory locations.

With correct program synchronization and ownership, this can be data-race-free.

But the cache line may still bounce between cores.

False sharing is caused by physical layout and write patterns, not specifically by atomic types.

---

## 18. Read-Only Sharing

Multiple threads reading the same cache line is usually efficient.

Cache coherence allows multiple cores to hold shared read-only copies.

False sharing mainly becomes destructive when multiple cores repeatedly write to the same line.

Typical problem:

```text
different cores
different variables
same cache line
frequent writes
```

---

## 19. False Sharing in Thread Pools

Thread pools often have per-worker state:

```text
task counters
queue indices
worker status
timestamps
metrics
```

A layout such as:

```cpp
struct WorkerStats {
    std::atomic<int> tasksCompleted;
};

std::vector<WorkerStats> stats;
```

may place adjacent workers' statistics next to one another.

If all workers update these fields frequently, false sharing may occur.

Potential fix:

```cpp
struct alignas(64) WorkerStats {
    std::atomic<int> tasksCompleted{0};
};
```

---

## 20. False Sharing in Queues

Concurrent queues often contain hot indices:

```cpp
std::atomic<size_t> head;
std::atomic<size_t> tail;
```

Producers frequently update `tail`.

Consumers frequently update `head`.

If `head` and `tail` share a cache line, they may interfere even though they are different variables.

A common layout is:

```cpp
struct Queue {
    alignas(64) std::atomic<size_t> head;
    alignas(64) std::atomic<size_t> tail;
};
```

This separates producer-hot and consumer-hot state.

---

## 21. False Sharing in ML and AI Infrastructure

False sharing can appear in:

```text
per-worker metrics
CPU preprocessing pipelines
thread-pool counters
request schedulers
tokenization workers
batching queues
GPU submission threads
per-shard statistics
reference-count hot paths
```

Example:

```text
16 CPU workers preprocess model inputs
each worker updates its own statistics field
all fields are adjacent
```

The work is logically independent, but cache coherence may reduce scaling.

---

## 22. Padding Is Not Always Good

Padding increases object size.

Example:

```cpp
struct alignas(64) Counter {
    std::atomic<long long> value;
};
```

An 8-byte value may consume 64 bytes.

With one million counters, this wastes substantial memory.

Costs include:

```text
larger memory footprint
more cache lines
more TLB pressure
less useful cache density
```

Only pad genuinely hot, independently written fields.

---

## 23. Alignment Does Not Guarantee Everything

Alignment helps ensure an object begins at a cache-line boundary.

But careful layout is still necessary.

Example:

```cpp
struct alignas(64) Counter {
    std::atomic<long long> value;
};
```

The compiler generally makes the object size a multiple of its alignment, so array elements are appropriately separated.

However, always verify assumptions for performance-critical code.

Possible tools:

```text
sizeof
alignof
address printing
profilers
hardware performance counters
```

---

## 24. Measuring False Sharing

Symptoms may include:

```text
adding threads does not improve throughput
performance becomes worse with more cores
high cache-coherence traffic
high cache-miss or invalidation activity
atomic counters become unexpectedly expensive
```

Useful tools may include:

```text
Linux perf
Intel VTune
AMD uProf
hardware performance counters
microbenchmarks
```

Do not diagnose false sharing only from source code if performance matters.

Measure it.

---

## 25. Benchmarking Warnings

Microbenchmarks can be misleading.

Results depend on:

```text
CPU architecture
cache-line size
thread placement
OS scheduler
compiler optimization
NUMA layout
system load
virtual machines
frequency scaling
```

A benchmark should:

```text
run enough iterations
use optimization
avoid unrelated I/O inside hot loop
run multiple times
pin threads when necessary
compare realistic workloads
```

---

## 26. Memory Order and False Sharing

Using:

```cpp
std::memory_order_relaxed
```

can reduce synchronization constraints compared with stronger ordering.

But relaxed memory order does not eliminate cache-line ownership transfer.

Example:

```cpp
counter.fetch_add(
    1,
    std::memory_order_relaxed
);
```

The atomic still performs a write to the cache line.

Therefore false sharing can remain severe.

```text
weaker memory ordering is not a layout fix
```

---

## 27. NUMA Considerations

On multi-socket systems, memory may be physically closer to one CPU socket than another.

False sharing across sockets can be especially expensive because cache-line ownership may move through an inter-socket interconnect.

For high-performance infrastructure, consider:

```text
thread affinity
NUMA-local allocation
per-socket sharding
per-core data
avoiding cross-socket hot writes
```

False sharing is one part of a broader data-locality problem.

---

## 28. Structure of Arrays vs Array of Structures

Memory layout choices affect cache behavior.

Array of Structures:

```cpp
struct Worker {
    int id;
    std::atomic<long long> counter;
    bool active;
};

std::vector<Worker> workers;
```

Structure of Arrays:

```cpp
std::vector<int> ids;
std::vector<std::atomic<long long>> counters;
std::vector<bool> active;
```

Neither layout is universally best.

Choose based on:

```text
which fields are accessed together
which fields are independently written
which fields are hot
which threads access them
```

---

## 29. Common Wrong Assumptions

### Wrong: separate variables cannot interfere

They can if they occupy the same cache line.

### Wrong: atomic solves all concurrency-performance problems

Atomic solves correctness and synchronization for the atomic object, not cache layout.

### Wrong: padding always makes code faster

Padding can waste cache and memory.

### Wrong: read-only sharing is the main issue

The destructive case is primarily frequent writes from different cores.

### Wrong: more threads always improve performance

More threads can increase coherence traffic and contention.

---

## 30. Practical Checklist

When examining a multithreaded hot path, ask:

```text
Which variables are frequently written?

Which threads write them?

Are independently written variables adjacent?

Do multiple hot atomics occupy one cache line?

Can updates be sharded per thread or per core?

Can aggregation happen less frequently?

Would padding help?

Has performance been measured?
```

---

## 31. Common Interview Questions

### Q1. What is false sharing?

False sharing happens when different threads modify different variables located on the same cache line. Cache coherence causes the line to bounce between cores, hurting performance.

### Q2. Is false sharing a data race?

No. The program can be fully data-race-free and still suffer from false sharing.

### Q3. Can atomic variables suffer from false sharing?

Yes. Independent atomic variables on the same cache line can still cause cache invalidation and ownership transfer.

### Q4. How can false sharing be reduced?

Use padding, alignment, per-thread storage, per-core storage, or sharded data to separate independently written hot variables.

### Q5. Why not pad every variable?

Padding increases memory usage, cache footprint, and TLB pressure. It should be applied only where measurements or access patterns justify it.

### Q6. What is the difference between true sharing and false sharing?

True sharing means threads access the same variable. False sharing means threads access different variables that occupy the same cache line.

---

## 32. Key Takeaways

* CPUs manage cache coherence at cache-line granularity.
* A cache line is commonly 64 bytes.
* Different variables may share one cache line.
* Independent cross-core writes can cause cache-line ping-pong.
* False sharing is a performance problem, not necessarily a correctness problem.
* Atomic operations do not prevent false sharing.
* Separate hot per-thread fields using alignment, padding, sharding, or thread-local storage.
* Keep frequently accessed related read-only data close together.
* Padding has memory and cache costs.
* Measure before and after changing layout.
* Correct synchronization does not guarantee scalable performance.
