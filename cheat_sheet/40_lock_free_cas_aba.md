# 40. Lock-Free Programming, CAS, and the ABA Problem

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. Blocking, Lock-Free, and Wait-Free](#2-blocking-lock-free-and-wait-free)
- [3. Compare-and-Swap](#3-compare-and-swap)
- [4. Failed CAS Updates expected](#4-failed-cas-updates-expected)
- [5. CAS Retry Loop](#5-cas-retry-loop)
- [6. Weak vs Strong CAS](#6-weak-vs-strong-cas)
- [7. Lock-Free Stack Push](#7-lock-free-stack-push)
- [8. Why Push Can Be Lock-Free](#8-why-push-can-be-lock-free)
- [9. Lock-Free Stack Pop](#9-lock-free-stack-pop)
- [10. Memory Reclamation Problem](#10-memory-reclamation-problem)
- [11. Reclamation Is Harder Than Removal](#11-reclamation-is-harder-than-removal)
- [12. Hazard Pointers](#12-hazard-pointers)
- [13. Epoch-Based Reclamation](#13-epoch-based-reclamation)
- [14. Deferred Reclamation](#14-deferred-reclamation)
- [15. ABA Problem](#15-aba-problem)
- [16. Why ABA Is Dangerous](#16-why-aba-is-dangerous)
- [17. Tagged Pointers](#17-tagged-pointers)
- [18. Limitations of Tagged Pointers](#18-limitations-of-tagged-pointers)
- [19. Atomic Does Not Always Mean Lock-Free](#19-atomic-does-not-always-mean-lock-free)
- [20. is_lock_free](#20-is_lock_free)
- [21. Lock-Free Is Not Automatically Faster](#21-lock-free-is-not-automatically-faster)
- [22. CAS Contention](#22-cas-contention)
- [23. Starvation](#23-starvation)
- [24. Memory Ordering](#24-memory-ordering)
- [25. Why seq_cst Is Useful While Learning](#25-why-seq_cst-is-useful-while-learning)
- [26. Lock-Free vs False Sharing](#26-lock-free-vs-false-sharing)
- [27. Lock-Free Queue Complexity](#27-lock-free-queue-complexity)
- [28. When Lock-Free May Be Appropriate](#28-when-lock-free-may-be-appropriate)
- [29. When Mutex Is Better](#29-when-mutex-is-better)
- [30. Common Wrong Assumptions](#30-common-wrong-assumptions)
- [31. Practical Checklist](#31-practical-checklist)
- [32. Common Interview Questions](#32-common-interview-questions)
- [33. Key Takeaways](#33-key-takeaways)

## Related Code Trap

- Demo file: [lock_free_cas_aba.cpp](../code_traps/lock_free_cas_aba.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

Lock-free programming uses atomic operations instead of traditional mutex-based blocking.

Common building blocks:

```cpp
std::atomic<T>
compare_exchange_weak
compare_exchange_strong
fetch_add
exchange
```

A lock-free algorithm does not mean every thread always completes quickly.

The formal idea is:

```text
the system as a whole continues to make progress
```

Even if one thread repeatedly fails and retries, some thread must complete operations.

---

## 2. Blocking, Lock-Free, and Wait-Free

### Blocking

Mutex-based algorithms may block because another thread owns a lock.

```cpp
std::lock_guard<std::mutex> lock(mutex);
```

If the lock holder is paused, delayed, or descheduled, other waiting threads cannot enter.

### Lock-free

A lock-free algorithm guarantees system-wide progress.

```text
some thread completes in a finite number of steps
```

An individual thread may still retry indefinitely and starve.

### Wait-free

A wait-free algorithm guarantees per-thread progress.

```text
every thread completes its operation in a bounded number of steps
```

Wait-free is stronger and generally harder to implement.

Progress guarantee ordering:

```text
blocking < lock-free < wait-free
```

This ordering refers to progress guarantees, not guaranteed real-world speed.

---

## 3. Compare-and-Swap

CAS means:

```text
Compare And Swap
```

C++ provides:

```cpp
compare_exchange_weak
compare_exchange_strong
```

Conceptually:

```cpp
if (atomicValue == expected) {
    atomicValue = desired;
    return true;
} else {
    expected = atomicValue;
    return false;
}
```

Example:

```cpp
std::atomic<int> state = 0;

int expected = 0;

bool success = state.compare_exchange_strong(
    expected,
    1
);
```

If `state` is zero:

```text
state becomes 1
success is true
```

If `state` is not zero:

```text
state remains unchanged
success is false
expected is overwritten with the actual state
```

---

## 4. Failed CAS Updates expected

This behavior is important:

```cpp
int expected = 10;

bool success = value.compare_exchange_strong(
    expected,
    20
);
```

If the actual value is 15:

```text
success == false
expected == 15
```

The caller can use the updated `expected` value for a retry.

---

## 5. CAS Retry Loop

Example atomic update:

```cpp
int current = value.load();

while (!value.compare_exchange_weak(
    current,
    current + 1
)) {
}
```

Possible execution:

```text
Thread A reads 10
Thread B reads 10

Thread A changes 10 -> 11
Thread B tries expected 10 and fails

Thread B's current becomes 11
Thread B retries 11 -> 12
```

For simple arithmetic, prefer specialized operations:

```cpp
value.fetch_add(1);
```

CAS loops are useful for more complex state transformations.

---

## 6. Weak vs Strong CAS

### compare_exchange_strong

A strong CAS fails when the compared value does not match.

It is useful for one-shot attempts:

```cpp
if (state.compare_exchange_strong(
    expected,
    desired
)) {
    // success
}
```

### compare_exchange_weak

A weak CAS may fail spuriously even when the values match.

It is normally used in a retry loop:

```cpp
while (!state.compare_exchange_weak(
    expected,
    desired
)) {
}
```

On some hardware, weak CAS may map more efficiently to native instructions.

---

## 7. Lock-Free Stack Push

A stack node:

```cpp
struct Node {
    int value;
    Node* next;
};
```

Stack head:

```cpp
std::atomic<Node*> head = nullptr;
```

Push operation:

```cpp
void push(int value) {
    Node* node = new Node{
        value,
        head.load()
    };

    while (!head.compare_exchange_weak(
        node->next,
        node
    )) {
    }
}
```

Explanation:

```text
node->next starts as the observed head
CAS attempts to replace head with node
if CAS fails, node->next is updated with current head
the loop retries
```

---

## 8. Why Push Can Be Lock-Free

Several threads can concurrently prepare their own nodes.

Only the atomic head pointer is contested.

If one CAS succeeds:

```text
that thread completes its push
```

Other threads retry using the new head.

No mutex owner can block all other threads indefinitely.

---

## 9. Lock-Free Stack Pop

Conceptual pop:

```cpp
Node* oldHead = head.load();

while (
    oldHead != nullptr &&
    !head.compare_exchange_weak(
        oldHead,
        oldHead->next
    )
) {
}
```

If successful:

```text
head changes from oldHead to oldHead->next
```

The removed node can be returned logically.

However, safely deleting it is difficult.

---

## 10. Memory Reclamation Problem

Suppose Thread A loads:

```text
oldHead = Node X
```

Before A accesses `oldHead->next`, Thread B:

```text
removes Node X
deletes Node X
```

Thread A then dereferences:

```cpp
oldHead->next
```

This is use-after-free and undefined behavior.

Therefore:

> Removing a node from a lock-free structure does not prove that no other thread still observes that node.

---

## 11. Reclamation Is Harder Than Removal

CAS can atomically detach a node.

But safe memory reclamation must determine:

```text
whether any other thread still has a pointer to the removed node
```

Common techniques include:

```text
hazard pointers
epoch-based reclamation
reference counting
read-copy-update
deferred reclamation
```

---

## 12. Hazard Pointers

A hazard pointer lets a thread announce:

```text
I may currently dereference this object
```

Before freeing a retired node, the reclaiming thread checks all hazard pointers.

If any hazard pointer still points to the node:

```text
the node cannot be deleted yet
```

Otherwise it may be reclaimed.

Hazard pointers are precise but require additional bookkeeping.

---

## 13. Epoch-Based Reclamation

Epoch-based reclamation groups operations into logical generations.

A removed node is retired in a particular epoch.

It can be freed only after all relevant threads have advanced beyond the epoch in which they might still reference it.

Advantages:

```text
efficient batch reclamation
low hot-path overhead
```

Tradeoffs:

```text
delayed memory release
thread participation tracking
stalled threads may delay reclamation
```

---

## 14. Deferred Reclamation

Educational implementations sometimes avoid deletion during concurrent operation.

Example:

```text
retired nodes are stored
all nodes are freed after every worker has stopped
```

This avoids use-after-free during the demo.

It is not suitable for unbounded long-running systems unless memory use is controlled.

---

## 15. ABA Problem

Suppose the atomic head contains pointer A:

```text
head = A
```

Thread 1 reads:

```text
expected = A
```

Thread 1 is paused.

Thread 2 performs:

```text
pop A
pop B
push A
```

Now the head is A again.

Thread 1 resumes and compares:

```text
head == expected
```

The pointer values match, so CAS may succeed.

But the stack changed from:

```text
A -> B -> ...
```

to another logical state and then back to pointer A.

CAS cannot detect the intermediate modification.

This is the ABA problem.

---

## 16. Why ABA Is Dangerous

CAS compares current bit patterns.

It does not know the history of the value.

A pointer can return to the same address because:

```text
the old node was removed and reinserted
the old object was freed
a new object reused the same address
```

The pointer bits may match while the logical object or structure has changed.

---

## 17. Tagged Pointers

One ABA mitigation is to compare:

```text
pointer + version counter
```

Instead of:

```text
A
```

the atomic state is:

```text
(A, version 1)
```

After modifications, even if the pointer becomes A again:

```text
(A, version 3)
```

CAS sees that the version changed.

This is also called:

```text
tagged pointer
stamped pointer
versioned pointer
```

---

## 18. Limitations of Tagged Pointers

Version counters may eventually wrap around.

The combined state may require a wider atomic CAS.

Not every platform guarantees that a wide atomic is lock-free.

Other memory reclamation techniques may still be necessary.

Tagged pointers address state-change detection but do not automatically solve every lifetime problem.

---

## 19. Atomic Does Not Always Mean Lock-Free

C++ permits an atomic implementation to use internal locking for unsupported types.

Example:

```cpp
std::atomic<MyLargeStruct> value;
```

You can query:

```cpp
value.is_lock_free();
```

Compile-time query:

```cpp
std::atomic<T>::is_always_lock_free
```

Therefore:

```text
atomic operation
does not necessarily imply lock-free implementation
```

---

## 20. is_lock_free

Example:

```cpp
std::atomic<int> integer;
std::atomic<long long> largeInteger;

std::cout << integer.is_lock_free();
std::cout << largeInteger.is_lock_free();
```

The answer can depend on:

```text
architecture
compiler
standard library
alignment
atomic type size
```

---

## 21. Lock-Free Is Not Automatically Faster

Lock-free code may suffer from:

```text
CAS retry storms
cache-line ping-pong
false sharing
memory reclamation overhead
complex memory ordering
starvation
larger metadata
```

A mutex may be faster when:

```text
contention is low
critical sections are short
the lock implementation has a fast uncontended path
```

Lock-free is primarily a progress property.

It is not a universal performance guarantee.

---

## 22. CAS Contention

Suppose many threads repeatedly CAS the same atomic head.

Only one succeeds at a time.

All others:

```text
fail
reload
retry
```

This produces:

```text
coherence traffic
wasted computation
cache contention
unfairness
```

Backoff strategies may reduce contention:

```text
yield
pause instruction
randomized delay
exponential backoff
```

But backoff changes performance, not correctness.

---

## 23. Starvation

Lock-free guarantees system progress, not individual fairness.

One thread may repeatedly lose every CAS race.

Other threads continue completing operations, so the algorithm remains lock-free.

The unlucky thread may starve.

Wait-free algorithms provide stronger per-thread completion guarantees.

---

## 24. Memory Ordering

Lock-free structures require careful memory ordering.

Typical publication pattern:

```cpp
node->value = value;
node->next = oldHead;

head.compare_exchange_weak(
    oldHead,
    node,
    std::memory_order_release,
    std::memory_order_relaxed
);
```

Reader side may use acquire ordering:

```cpp
Node* node = head.load(
    std::memory_order_acquire
);
```

Release publishes initialized node contents.

Acquire observes those contents.

Using incorrect memory ordering can make a structure data-race-free on the atomic pointer while still incorrectly publishing node data.

---

## 25. Why seq_cst Is Useful While Learning

Sequential consistency provides the strongest simple ordering model.

When initially implementing or reasoning about concurrent structures, using default `seq_cst` operations can reduce complexity.

Only weaken memory ordering after:

```text
the algorithm is proven correct
the required ordering is understood
performance is measured
```

Lock-free code is already difficult without unnecessary ordering optimization.

---

## 26. Lock-Free vs False Sharing

Even a correct lock-free structure may scale poorly if hot atomic variables share cache lines.

Examples:

```text
head and tail in same cache line
per-worker CAS state stored adjacent
multiple atomic counters packed together
```

Lock-free and cache layout must both be considered.

```text
no lock
does not mean no contention
```

---

## 27. Lock-Free Queue Complexity

A concurrent queue commonly has:

```text
head
tail
linked nodes or ring buffer slots
```

Difficult problems include:

```text
multiple producers
multiple consumers
ABA
memory reclamation
full/empty detection
publication ordering
cache-line placement
shutdown
bounded capacity
```

Production-quality lock-free queues should usually come from a mature tested library.

---

## 28. When Lock-Free May Be Appropriate

Possible cases:

```text
very low latency requirements
high-frequency hot paths
threads cannot tolerate dependency on a paused lock owner
runtime, allocator, scheduler, or kernel-like code
well-understood bounded atomic state
mature existing concurrent library implementation
```

---

## 29. When Mutex Is Better

Prefer a mutex when:

```text
critical section is simple
correctness and maintainability matter most
contention is low or moderate
multiple fields form one invariant
memory reclamation would be complex
profiling does not show a locking bottleneck
```

A clear mutex-based solution is often safer and fast enough.

---

## 30. Common Wrong Assumptions

### Wrong: lock-free means no thread waits

A thread may repeatedly retry or starve.

### Wrong: atomic means lock-free

An atomic type may use an internal lock.

### Wrong: CAS success means a node can be deleted

Other threads may still hold pointers to the node.

### Wrong: pointer equality proves nothing changed

The value may have gone through an ABA sequence.

### Wrong: lock-free is always faster

Contention and reclamation costs can make it slower.

---

## 31. Practical Checklist

Before implementing a lock-free structure, ask:

```text
What progress guarantee is actually required?

Can a mutex solve the problem safely?

Which atomic state is being updated?

What happens when CAS fails?

Can one thread starve?

How is memory reclaimed?

Can ABA occur?

What memory ordering is required?

Are hot atomics on separate cache lines?

Is a mature library implementation available?

Has performance been measured?
```

---

## 32. Common Interview Questions

### Q1. What does lock-free mean?

Lock-free means the system as a whole is guaranteed to make progress, although an individual thread may repeatedly retry or starve.

### Q2. What is CAS?

CAS atomically compares an atomic value with an expected value and replaces it with a desired value only if they match.

### Q3. What happens to expected when CAS fails?

It is overwritten with the actual current value of the atomic object.

### Q4. Difference between weak and strong CAS?

Weak CAS may fail spuriously and is usually used in loops. Strong CAS does not fail spuriously and is useful for one-shot attempts.

### Q5. What is the ABA problem?

ABA occurs when an atomic value changes from A to another value and back to A. A CAS that only compares the current value may incorrectly conclude that nothing changed.

### Q6. Why is lock-free memory reclamation difficult?

A node removed from the structure may still be referenced by another thread. Deleting it immediately can cause use-after-free.

### Q7. Is std::atomic always lock-free?

No. Use `is_lock_free()` or `is_always_lock_free` to query the implementation.

---

## 33. Key Takeaways

* Lock-free guarantees system-wide progress, not per-thread completion.
* Wait-free is a stronger per-thread guarantee.
* CAS compares expected state and conditionally installs desired state.
* Failed CAS updates the expected argument.
* Weak CAS may fail spuriously and is commonly used in loops.
* Node removal and node reclamation are separate problems.
* Immediate deletion can create use-after-free.
* ABA means a value returns to the same representation after intermediate changes.
* Tagged pointers and reclamation schemes help address ABA and lifetime issues.
* Atomic does not necessarily mean lock-free.
* Lock-free does not necessarily mean faster.
* Prefer mature implementations for production concurrent containers.
