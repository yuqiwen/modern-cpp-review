# 30. Concurrency: thread, mutex, lock_guard, atomic

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. std::thread](#2-stdthread)
- [3. join vs detach](#3-join-vs-detach)
- [4. Passing Arguments to Thread](#4-passing-arguments-to-thread)
- [5. Data Race](#5-data-race)
- [6. Why counter++ Is Not Atomic](#6-why-counter-is-not-atomic)
- [7. mutex](#7-mutex)
- [8. lock_guard](#8-lock_guard)
- [9. Keep Critical Sections Small](#9-keep-critical-sections-small)
- [10. unique_lock](#10-unique_lock)
- [11. scoped_lock](#11-scoped_lock)
- [12. Deadlock](#12-deadlock)
- [13. atomic](#13-atomic)
- [14. atomic Is Not a Magic Mutex](#14-atomic-is-not-a-magic-mutex)
- [15. volatile Is Not Synchronization](#15-volatile-is-not-synchronization)
- [16. Race Condition vs Data Race](#16-race-condition-vs-data-race)
- [17. Thread Object Lifetime](#17-thread-object-lifetime)
- [18. jthread](#18-jthread)
- [19. Common Interview Questions](#19-common-interview-questions)
- [20. Key Takeaways](#20-key-takeaways)

## Related Code Trap

- [concurrency / thread / mutex / atomic Demo](../code_traps/concurrency_thread_mutex_atomic.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

C++ supports multi-threading with:

```cpp
#include <thread>
#include <mutex>
#include <atomic>
```

Important rule:

```text
If multiple threads access the same memory,
and at least one thread writes,
and there is no synchronization,
that is a data race.
```

Data race in C++ is:

```text
undefined behavior
```

Not just "wrong result".

---

## 2. std::thread

```cpp
std::thread t([] {
    std::cout << "hello from worker\n";
});

t.join();
```

`std::thread` starts a new thread of execution.

`join()` means:

```text
wait until the thread finishes
```

If a `std::thread` object is destroyed while still joinable, the program calls:

```cpp
std::terminate()
```

So every thread should normally be:

```text
joined or detached
```

---

## 3. join vs detach

### join

```cpp
t.join();
```

Meaning:

```text
current thread waits for t to finish
```

This is usually preferred.

---

### detach

```cpp
t.detach();
```

Meaning:

```text
thread runs independently
std::thread object no longer manages it
```

Danger:

```text
detached thread may access objects that already died
```

So avoid detach unless lifetime is carefully designed.

---

## 4. Passing Arguments to Thread

```cpp
void worker(int x) {
    std::cout << x << std::endl;
}

std::thread t(worker, 42);
t.join();
```

Arguments are copied/moved into the thread by default.

If you need reference:

```cpp
std::thread t(workerRef, std::ref(value));
```

Without `std::ref`, the thread receives a copy.

---

## 5. Data Race

Bad:

```cpp
int counter = 0;

void increment() {
    for (int i = 0; i < 100000; ++i) {
        ++counter;
    }
}
```

If two threads call `increment()` at the same time:

```text
counter is shared
both threads write
no synchronization
```

This is a data race.

The result is undefined behavior.

---

## 6. Why counter++ Is Not Atomic

```cpp
++counter;
```

Conceptually does:

```text
load counter
add 1
store counter
```

Two threads can interleave:

```text
Thread A loads 10
Thread B loads 10
Thread A stores 11
Thread B stores 11
```

Expected +2, actual +1.

This is a lost update.

---

## 7. mutex

A mutex protects critical sections.

```cpp
std::mutex m;
int counter = 0;

void increment() {
    std::lock_guard<std::mutex> lock(m);
    ++counter;
}
```

Only one thread can hold the mutex at a time.

The protected region is called:

```text
critical section
```

---

## 8. lock_guard

Manual lock/unlock:

```cpp
m.lock();
mayThrow();
m.unlock();
```

Danger:

```text
if mayThrow() throws, unlock is skipped
```

RAII version:

```cpp
std::lock_guard<std::mutex> lock(m);
mayThrow();
```

When `lock` goes out of scope, its destructor unlocks the mutex.

---

## 9. Keep Critical Sections Small

Bad:

```cpp
std::lock_guard<std::mutex> lock(m);
doSlowNetworkRequest();
updateCounter();
```

This holds the lock while doing slow work.

Better:

```cpp
auto result = doSlowNetworkRequest();

{
    std::lock_guard<std::mutex> lock(m);
    updateSharedState(result);
}
```

Lock only around shared data access.

---

## 10. unique_lock

`std::unique_lock` is more flexible than `lock_guard`.

```cpp
std::unique_lock<std::mutex> lock(m);
```

It can:

```text
lock later
unlock early
move ownership
work with condition_variable
```

Example:

```cpp
std::unique_lock<std::mutex> lock(m);
sharedWork();
lock.unlock();
doNonSharedWork();
```

Use `lock_guard` by default.

Use `unique_lock` when extra flexibility is needed.

---

## 11. scoped_lock

For locking multiple mutexes:

```cpp
std::scoped_lock lock(m1, m2);
```

This helps avoid deadlock by locking multiple mutexes safely.

Bad:

```cpp
// Thread A
lock m1 then m2

// Thread B
lock m2 then m1
```

This can deadlock.

---

## 12. Deadlock

Deadlock means threads wait forever for each other.

Example:

```text
Thread A holds m1 and waits for m2
Thread B holds m2 and waits for m1
```

Avoid by:

```text
consistent lock order
std::scoped_lock for multiple mutexes
short critical sections
```

---

## 13. atomic

For simple shared variables:

```cpp
std::atomic<int> counter = 0;

counter++;
```

This operation is synchronized and avoids data race.

Good for:

```text
counters
flags
simple state
```

Example:

```cpp
std::atomic<bool> stop = false;
```

One thread writes:

```cpp
stop.store(true);
```

Another thread reads:

```cpp
if (stop.load()) { ... }
```

---

## 14. atomic Is Not a Magic Mutex

This is okay:

```cpp
std::atomic<int> counter;
counter++;
```

But for multiple variables that must be updated together:

```cpp
x++;
y++;
```

atomic alone may not be enough.

If invariant matters:

```text
x and y must change together
```

Use a mutex.

---

## 15. volatile Is Not Synchronization

Bad idea:

```cpp
volatile bool stop = false;
```

`volatile` does not make code thread-safe.

It does not provide atomicity or inter-thread synchronization.

Use:

```cpp
std::atomic<bool> stop = false;
```

---

## 16. Race Condition vs Data Race

These are related but not identical.

### Data race

Technical C++ definition:

```text
unsynchronized conflicting memory access
undefined behavior
```

Example:

```cpp
counter++;
```

from multiple threads without mutex/atomic.

---

### Race condition

Higher-level logic bug where result depends on timing.

Can happen even with no data race.

Example:

```text
check if queue is empty
then later pop
another thread changes queue in between
```

Mutex may prevent data race, but bad logic can still cause race condition.

---

## 17. Thread Object Lifetime

Bad:

```cpp
void f() {
    std::thread t(worker);
} // terminate, because t is still joinable
```

Good:

```cpp
void f() {
    std::thread t(worker);
    t.join();
}
```

Or use C++20:

```cpp
std::jthread t(worker);
```

`jthread` joins automatically in destructor.

---

## 18. jthread

C++20 has:

```cpp
std::jthread
```

It is like `std::thread`, but RAII-friendly:

```text
destructor requests stop and joins
```

This avoids the "forgot to join" terminate problem.

Basic usage:

```cpp
std::jthread t([] {
    doWork();
});
```

When `t` goes out of scope, it joins automatically.

---

## 19. Common Interview Questions

### Q1. What is a data race?

A data race happens when multiple threads access the same memory concurrently, at least one access is a write, and there is no synchronization.

In C++, data race is undefined behavior.

---

### Q2. Why is counter++ not thread-safe?

Because it is read-modify-write: load, increment, store.

Multiple threads can interleave and lose updates.

---

### Q3. How do you protect shared data?

Use a mutex and RAII lock such as `std::lock_guard`.

For simple independent variables, use `std::atomic`.

---

### Q4. What is the difference between mutex and atomic?

A mutex protects a critical section, possibly involving multiple operations or variables.

Atomic protects individual atomic operations on a single object.

---

### Q5. Why is lock_guard RAII?

It locks the mutex in its constructor and unlocks it in its destructor.

So the mutex is released even if exceptions happen.

---

### Q6. What happens if a joinable std::thread is destroyed?

The program calls `std::terminate`.

You must join or detach it before destruction.

---

### Q7. Is volatile enough for multithreading?

No.

`volatile` is not synchronization.

Use `std::atomic` or mutex.

---

## 20. Key Takeaways

- `std::thread` starts a new thread.
- `join()` waits for a thread to finish.
- Destroying a joinable `std::thread` calls `std::terminate`.
- Data race is undefined behavior.
- `counter++` is not atomic for normal int.
- Use `std::mutex` to protect shared data.
- Use `std::lock_guard` for RAII lock/unlock.
- Use `std::unique_lock` when you need flexibility.
- Use `std::scoped_lock` for multiple mutexes.
- Use `std::atomic` for simple shared counters/flags.
- `volatile` is not thread synchronization.
- Keep critical sections small.
