# 38. shared_mutex and Reader-Writer Locks

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. Multiple Readers](#2-multiple-readers)
- [3. One Writer](#3-one-writer)
- [4. Basic Pattern](#4-basic-pattern)
- [5. Why mutable mutex?](#5-why-mutable-mutex)
- [6. shared_lock](#6-shared_lock)
- [7. unique_lock With shared_mutex](#7-unique_lock-with-shared_mutex)
- [8. Lock Compatibility](#8-lock-compatibility)
- [9. Compatibility Table](#9-compatibility-table)
- [10. Why Not Always Use shared_mutex?](#10-why-not-always-use-shared_mutex)
- [11. Read-Heavy Example](#11-read-heavy-example)
- [12. Read Lock Does Not Mean Thread-Safe Mutation](#12-read-lock-does-not-mean-thread-safe-mutation)
- [13. Hidden Mutation](#13-hidden-mutation)
- [14. Returning References Is Dangerous](#14-returning-references-is-dangerous)
- [15. Returning Pointer Has Same Problem](#15-returning-pointer-has-same-problem)
- [16. Check-Then-Act Race](#16-check-then-act-race)
- [17. Correct Check-Then-Act](#17-correct-check-then-act)
- [18. Avoid Expensive Work Under Write Lock](#18-avoid-expensive-work-under-write-lock)
- [19. Double-Check Pattern](#19-double-check-pattern)
- [20. Lock Upgrade Problem](#20-lock-upgrade-problem)
- [21. Never Acquire Exclusive While Holding Shared Lock](#21-never-acquire-exclusive-while-holding-shared-lock)
- [22. Writer Starvation](#22-writer-starvation)
- [23. shared_timed_mutex](#23-shared_timed_mutex)
- [24. try_lock_shared](#24-try_lock_shared)
- [25. defer_lock](#25-defer_lock)
- [26. adopt_lock](#26-adopt_lock)
- [27. shared_mutex vs atomic](#27-shared_mutex-vs-atomic)
- [28. shared_mutex vs Copy-on-Write Snapshot](#28-shared_mutex-vs-copy-on-write-snapshot)
- [29. Read Lock Scope](#29-read-lock-scope)
- [30. Write Lock Scope](#30-write-lock-scope)
- [31. Const Does Not Mean Thread-Safe](#31-const-does-not-mean-thread-safe)
- [32. Basic Thread-Safe Cache](#32-basic-thread-safe-cache)
- [33. Common Wrong Patterns](#33-common-wrong-patterns)
- [34. Common Interview Questions](#34-common-interview-questions)
- [35. Key Takeaways](#35-key-takeaways)

## Related Code Trap

- Demo file: [shared_mutex_reader_writer_lock.cpp](../code_traps/shared_mutex_reader_writer_lock.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

Normal mutex:

```cpp
std::mutex
```

allows only one thread inside, regardless of whether the thread is reading or writing.

But some shared data is:

```text
read very often
write rarely
```

For this pattern, C++ provides:

```cpp
std::shared_mutex
```

It supports two locking modes:

```text
shared lock    -> read access
exclusive lock -> write access
```

---

## 2. Multiple Readers

Several threads may hold shared ownership simultaneously.

Example:

```cpp
std::shared_lock<std::shared_mutex> lock(mutex);
```

If three readers acquire shared locks:

```text
reader A enters
reader B enters
reader C enters
```

They can all read concurrently.

This is safe only if none of them modifies the protected data.

---

## 3. One Writer

A writer acquires exclusive ownership:

```cpp
std::unique_lock<std::shared_mutex> lock(mutex);
```

While the writer holds the mutex:

```text
no other writer may enter
no reader may enter
```

The writer has exclusive access.

---

## 4. Basic Pattern

```cpp
class Cache {
private:
    std::unordered_map<int, std::string> data_;
    mutable std::shared_mutex mutex_;

public:
    std::string get(int key) const {
        std::shared_lock lock(mutex_);

        auto it = data_.find(key);

        if (it == data_.end()) {
            return {};
        }

        return it->second;
    }

    void set(int key, std::string value) {
        std::unique_lock lock(mutex_);
        data_[key] = std::move(value);
    }
};
```

Meaning:

```text
get() uses shared/read lock
set() uses exclusive/write lock
```

---

## 5. Why mutable mutex?

A read function is often marked:

```cpp
const
```

Example:

```cpp
std::string get(int key) const;
```

But locking changes the mutex object's internal state.

Without `mutable`, this would not compile:

```cpp
std::shared_lock lock(mutex_);
```

So the mutex is commonly declared:

```cpp
mutable std::shared_mutex mutex_;
```

This does not mean the logical data changes.

Locking is considered synchronization, not logical modification of the object.

---

## 6. shared_lock

`std::shared_lock` is an RAII wrapper for shared ownership.

```cpp
std::shared_lock<std::shared_mutex> lock(mutex);
```

Conceptually:

```text
constructor -> lock_shared()
destructor  -> unlock_shared()
```

It is similar to `unique_lock`, but for shared/read locking.

---

## 7. unique_lock With shared_mutex

A writer uses:

```cpp
std::unique_lock<std::shared_mutex> lock(mutex);
```

Conceptually:

```text
constructor -> lock()
destructor  -> unlock()
```

This is exclusive ownership.

A `lock_guard<std::shared_mutex>` can also take exclusive lock, but `unique_lock` is more flexible.

---

## 8. Lock Compatibility

Suppose the mutex is currently unlocked.

### Reader A acquires shared lock

Allowed.

### Reader B acquires shared lock

Also allowed.

### Writer C tries exclusive lock

Blocked until all readers release.

---

Suppose writer C already owns exclusive lock.

### Reader A tries shared lock

Blocked.

### Writer D tries exclusive lock

Blocked.

---

## 9. Compatibility Table

```text
current holders    new reader    new writer
------------------------------------------------
none               allowed       allowed
reader(s)          allowed       blocked
writer             blocked       blocked
```

---

## 10. Why Not Always Use shared_mutex?

`shared_mutex` is not automatically faster than `mutex`.

It has more complex bookkeeping.

Potential costs:

```text
reader count tracking
writer coordination
more complex implementation
larger lock object
possible starvation
```

For small critical sections or moderate contention, a normal mutex may be faster.

Use `shared_mutex` when:

```text
reads dominate heavily
read sections are meaningful
contention is real
profiling supports it
```

---

## 11. Read-Heavy Example

Suppose:

```text
1000 lookups per second
1 update per second
```

A normal mutex serializes all lookups.

A shared mutex may allow many lookups concurrently.

This can improve throughput.

But if reads are extremely short, the overhead may cancel the benefit.

---

## 12. Read Lock Does Not Mean Thread-Safe Mutation

Wrong:

```cpp
std::shared_lock lock(mutex);
data_[key] = value; // wrong
```

A shared lock only permits read-only access.

Multiple readers may be inside simultaneously.

If one modifies data under a shared lock, it can race with other readers.

Use exclusive lock for any mutation.

---

## 13. Hidden Mutation

A function may look like a read but still mutate internal state.

Example:

```cpp
std::string get(int key) {
    std::shared_lock lock(mutex);
    return data_[key];
}
```

Problem:

```cpp
operator[] may insert a missing key
```

So this is actually a write.

Use:

```cpp
auto it = data_.find(key);
```

instead.

This is a very common bug.

---

## 14. Returning References Is Dangerous

Consider:

```cpp
const std::string& get(int key) const {
    std::shared_lock lock(mutex_);
    return data_.at(key);
}
```

The lock is released when function returns.

The caller receives a reference to internal data without protection.

Another thread may erase or modify that element.

This can cause:

```text
data race
dangling reference
invalid observation
```

Safer:

```cpp
std::optional<std::string> get(int key) const;
```

Return a copy.

Or keep the lock alive through a more carefully designed API.

---

## 15. Returning Pointer Has Same Problem

```cpp
const Value* find(int key) const;
```

If pointer refers to protected internal storage, it becomes unprotected after lock release.

Another writer may invalidate it.

Lock lifetime must cover the entire use of protected data.

---

## 16. Check-Then-Act Race

Bad:

```cpp
if (!cache.contains(key)) {
    cache.insert(key, compute());
}
```

Even if each function internally locks correctly:

```text
contains() lock released
another thread inserts
insert() happens later
```

The combined operation is not atomic.

This is a higher-level race condition.

You need one write lock around the whole sequence.

---

## 17. Correct Check-Then-Act

```cpp
std::unique_lock lock(mutex_);

auto it = data_.find(key);

if (it == data_.end()) {
    data_[key] = compute();
}
```

Now the check and insert are protected as one critical section.

But holding a write lock during expensive `compute()` may hurt performance.

---

## 18. Avoid Expensive Work Under Write Lock

Bad:

```cpp
std::unique_lock lock(mutex_);

Value value = expensiveCompute();
data_[key] = value;
```

All readers and writers are blocked during the computation.

Possible improvement:

```cpp
Value value = expensiveCompute();

std::unique_lock lock(mutex_);
data_[key] = std::move(value);
```

But this changes semantics because another thread may insert meanwhile.

You may need to check again after acquiring the lock.

---

## 19. Double-Check Pattern

```cpp
{
    std::shared_lock lock(mutex_);

    auto it = data_.find(key);

    if (it != data_.end()) {
        return it->second;
    }
}

Value value = expensiveCompute();

{
    std::unique_lock lock(mutex_);

    auto [it, inserted] =
        data_.try_emplace(key, std::move(value));

    return it->second;
}
```

This avoids computing under the write lock.

But two threads may compute the same value concurrently.

Whether this is acceptable depends on the application.

---

## 20. Lock Upgrade Problem

A common desire:

```text
start with read lock
discover missing data
upgrade directly to write lock
```

Standard `std::shared_mutex` does not provide a simple atomic upgrade operation.

Dangerous:

```cpp
std::shared_lock readLock(mutex);

// decide write is needed

readLock.unlock();

std::unique_lock writeLock(mutex);
```

Between unlock and write lock acquisition, shared state may change.

Therefore you must re-check the condition after acquiring exclusive lock.

---

## 21. Never Acquire Exclusive While Holding Shared Lock

Bad:

```cpp
std::shared_lock readLock(mutex);

std::unique_lock writeLock(mutex); // likely deadlock
```

The current thread itself contributes to the active reader count.

The exclusive lock waits for all readers to leave, including itself.

Release shared ownership first, then acquire exclusive ownership and re-check state.

---

## 22. Writer Starvation

Depending on implementation and scheduling, a continuous stream of readers may delay a writer.

This is called:

```text
writer starvation
```

Similarly, some policies may favor writers and delay readers.

The C++ standard does not give a simple universal fairness guarantee you should rely on.

If fairness matters, design and measure carefully.

---

## 23. shared_timed_mutex

C++ also has:

```cpp
std::shared_timed_mutex
```

It supports timed operations such as:

```cpp
try_lock_for()
try_lock_until()
try_lock_shared_for()
try_lock_shared_until()
```

Modern code often uses `shared_mutex` unless timed locking is required.

---

## 24. try_lock_shared

A reader may attempt without blocking:

```cpp
if (mutex.try_lock_shared()) {
    // read
    mutex.unlock_shared();
}
```

Prefer RAII where possible:

```cpp
std::shared_lock lock(
    mutex,
    std::try_to_lock
);

if (lock.owns_lock()) {
    // read
}
```

---

## 25. defer_lock

You can construct without immediately locking:

```cpp
std::shared_lock lock(
    mutex,
    std::defer_lock
);

lock.lock();
```

This is useful when coordinating multiple locks or controlling lock timing.

For normal code, lock immediately.

---

## 26. adopt_lock

If shared ownership is already acquired manually:

```cpp
mutex.lock_shared();

std::shared_lock lock(
    mutex,
    std::adopt_lock
);
```

`adopt_lock` tells the RAII object:

```text
the lock is already held
take responsibility for unlocking it
```

Wrong use leads to undefined behavior.

Use sparingly.

---

## 27. shared_mutex vs atomic

Use atomic when:

```text
one simple independent value
single atomic operation
```

Example:

```cpp
std::atomic<int> count;
```

Use shared mutex when protecting:

```text
container
multiple fields
compound invariants
read-only operations over complex state
```

An unordered_map cannot become safe merely by making one size counter atomic.

---

## 28. shared_mutex vs Copy-on-Write Snapshot

For very read-heavy systems, another design is:

```text
readers access immutable snapshot
writer creates a new snapshot
writer atomically publishes it
```

This can reduce reader locking.

But it increases:

```text
copy cost
memory usage
design complexity
```

Reader-writer locks are only one possible design.

---

## 29. Read Lock Scope

Keep read lock only as long as necessary.

Bad:

```cpp
std::shared_lock lock(mutex_);

Value value = data_.at(key);

slowNetworkCall(value);
```

The reader blocks writers during slow work.

Better:

```cpp
Value value;

{
    std::shared_lock lock(mutex_);
    value = data_.at(key);
}

slowNetworkCall(value);
```

Copy what you need, release the lock, then do slow work.

---

## 30. Write Lock Scope

Write locks block everyone.

Therefore keep them especially small.

Bad:

```cpp
std::unique_lock lock(mutex_);

validateInput();
performIO();
updateData();
logResult();
```

Better:

```cpp
auto prepared = validateAndPrepareOutsideLock();

{
    std::unique_lock lock(mutex_);
    updateData(prepared);
}
```

---

## 31. Const Does Not Mean Thread-Safe

A member function marked:

```cpp
const
```

does not automatically mean:

```text
safe for concurrent calls
```

It only prevents ordinary logical mutation through that interface.

It may still:

- modify mutable state
- access externally shared resources
- race with writers
- return references that later become invalid

Thread safety requires synchronization design.

---

## 32. Basic Thread-Safe Cache

```cpp
class Cache {
private:
    mutable std::shared_mutex mutex_;
    std::unordered_map<int, std::string> data_;

public:
    std::optional<std::string> get(int key) const {
        std::shared_lock lock(mutex_);

        auto it = data_.find(key);

        if (it == data_.end()) {
            return std::nullopt;
        }

        return it->second;
    }

    void put(int key, std::string value) {
        std::unique_lock lock(mutex_);
        data_.insert_or_assign(key, std::move(value));
    }

    void erase(int key) {
        std::unique_lock lock(mutex_);
        data_.erase(key);
    }
};
```

All reads use shared lock.

All mutations use exclusive lock.

---

## 33. Common Wrong Patterns

### Wrong: mutate under shared lock

```cpp
std::shared_lock lock(mutex);
container.push_back(value);
```

### Wrong: use map operator[] in read path

```cpp
std::shared_lock lock(mutex);
return map[key];
```

`operator[]` can insert.

### Wrong: return internal reference

```cpp
const Value& get(...);
```

Lock ends before reference use.

### Wrong: upgrade without re-checking

```text
release read lock
acquire write lock
assume condition unchanged
```

### Wrong: assume shared_mutex is always faster

It may be slower under low contention or write-heavy workloads.

---

## 34. Common Interview Questions

### Q1. What is std::shared_mutex?

It is a reader-writer mutex that allows multiple simultaneous readers or one exclusive writer.

### Q2. Which lock type is used for reading?

Use `std::shared_lock<std::shared_mutex>`.

### Q3. Which lock type is used for writing?

Use `std::unique_lock<std::shared_mutex>` or an exclusive lock guard.

### Q4. Can readers and writers hold the lock simultaneously?

No. Multiple readers may coexist, but a writer requires exclusive access.

### Q5. Why not always use shared_mutex?

It has greater overhead and more complex scheduling. It is useful mainly for sufficiently read-heavy, contended workloads.

### Q6. Why is returning a reference from a locked getter dangerous?

Because the lock is released on return, while another thread may mutate or erase the referenced data afterward.

---

## 35. Key Takeaways

- `shared_mutex` supports shared and exclusive locking.
- Multiple readers may hold shared locks simultaneously.
- One writer excludes all readers and writers.
- Use `shared_lock` for true read-only access.
- Use exclusive lock for every mutation.
- Do not use `operator[]` in a read-only map path.
- Do not return unprotected references to internal data.
- Re-check state after releasing a read lock and acquiring a write lock.
- Keep both read and write lock scopes small.
- Use `shared_mutex` only when workload and profiling justify it.
