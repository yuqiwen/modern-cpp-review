# 37. std::call_once, std::once_flag, and thread_local

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. The One-Time Initialization Problem](#2-the-one-time-initialization-problem)
- [3. Mutex-Based Initialization](#3-mutex-based-initialization)
- [4. std::once_flag](#4-stdonce_flag)
- [5. Basic call_once](#5-basic-call_once)
- [6. Successful Completion Matters](#6-successful-completion-matters)
- [7. Exception Behavior](#7-exception-behavior)
- [8. Other Threads During Initialization](#8-other-threads-during-initialization)
- [9. Memory Visibility](#9-memory-visibility)
- [10. once_flag Cannot Be Reset](#10-once_flag-cannot-be-reset)
- [11. Function-Local Static Initialization](#11-function-local-static-initialization)
- [12. call_once vs Local Static](#12-call_once-vs-local-static)
- [13. Singleton Warning](#13-singleton-warning)
- [14. What Is thread_local?](#14-what-is-thread_local)
- [15. thread_local Example](#15-thread_local-example)
- [16. thread_local Is Not Shared](#16-thread_local-is-not-shared)
- [17. thread_local Lifetime](#17-thread_local-lifetime)
- [18. thread_local With Non-Trivial Types](#18-thread_local-with-non-trivial-types)
- [19. Common Uses of thread_local](#19-common-uses-of-thread_local)
- [20. Thread-Local Scratch Buffer](#20-thread-local-scratch-buffer)
- [21. thread_local Does Not Mean Function-Local](#21-thread_local-does-not-mean-function-local)
- [22. Normal Local vs static vs thread_local](#22-normal-local-vs-static-vs-thread_local)
- [23. static thread_local](#23-static-thread_local)
- [24. thread_local Does Not Solve All Races](#24-thread_local-does-not-solve-all-races)
- [25. Reducing Lock Contention](#25-reducing-lock-contention)
- [26. Memory Cost](#26-memory-cost)
- [27. Thread Pools and thread_local](#27-thread-pools-and-thread_local)
- [28. Request Context Leakage](#28-request-context-leakage)
- [29. RAII Thread-Local Context Guard](#29-raii-thread-local-context-guard)
- [30. once_flag vs atomic bool](#30-once_flag-vs-atomic-bool)
- [31. Double-Checked Locking](#31-double-checked-locking)
- [32. Reentrant call_once Danger](#32-reentrant-call_once-danger)
- [33. Common Wrong Patterns](#33-common-wrong-patterns)
- [34. Common Interview Questions](#34-common-interview-questions)
- [35. Key Takeaways](#35-key-takeaways)

## Related Code Trap

- Demo file: [call_once_thread_local.cpp](../code_traps/call_once_thread_local.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

C++ provides:

```cpp
std::once_flag
std::call_once
thread_local
```

They solve different concurrency problems.

```text
call_once:
    ensure an initialization function completes successfully once

thread_local:
    give every thread its own instance of a variable
```

Headers:

```cpp
#include <mutex>
#include <thread>
```

---

## 2. The One-Time Initialization Problem

Suppose several threads need the same global configuration:

```cpp
Config config;
bool initialized = false;
```

Bad:

```cpp
if (!initialized) {
    initializeConfig();
    initialized = true;
}
```

Multiple threads can simultaneously observe:

```text
initialized == false
```

Then initialize more than once.

This is a data race and a check-then-act race.

---

## 3. Mutex-Based Initialization

You could use a mutex:

```cpp
std::mutex mutex;
bool initialized = false;

void initialize() {
    std::lock_guard<std::mutex> lock(mutex);

    if (!initialized) {
        initializeConfig();
        initialized = true;
    }
}
```

This can work.

But every later call still locks the mutex.

For one-time initialization, C++ provides a clearer abstraction:

```cpp
std::call_once
```

---

## 4. std::once_flag

Declare a flag:

```cpp
std::once_flag flag;
```

The flag stores one-time execution state.

You do not manually read or modify it.

You pass it to:

```cpp
std::call_once(flag, callable);
```

---

## 5. Basic call_once

```cpp
std::once_flag flag;

void initialize() {
    std::call_once(flag, [] {
        std::cout << "initializing\n";
    });
}
```

If ten threads call `initialize()` concurrently:

```text
one thread executes the lambda
other threads wait
after successful completion, all calls return
future calls do not execute lambda again
```

---

## 6. Successful Completion Matters

The precise idea is not simply:

```text
callable starts once
```

It is:

```text
callable completes successfully once
```

If the callable throws, the flag is not considered completed.

A later `call_once` may try again.

---

## 7. Exception Behavior

Example:

```cpp
std::once_flag flag;
int attempts = 0;

void initialize() {
    std::call_once(flag, [] {
        ++attempts;

        if (attempts < 2) {
            throw std::runtime_error("failed");
        }
    });
}
```

First caller:

```text
callable throws
flag remains incomplete
```

Later caller:

```text
callable runs again
if it succeeds, flag becomes completed
```

So `call_once` means:

```text
exactly one successful execution
```

not necessarily exactly one attempt.

---

## 8. Other Threads During Initialization

Suppose thread A is running the initialization callable.

Threads B and C call:

```cpp
std::call_once(flag, callable);
```

They do not run the callable simultaneously.

They wait until:

```text
A succeeds
or
A throws
```

If A succeeds, B and C return without running it.

If A throws, another caller may attempt it.

---

## 9. Memory Visibility

`call_once` provides synchronization.

Writes made by the successful initialization function become visible to threads whose `call_once` returns afterward.

Example:

```cpp
std::unique_ptr<Config> config;
std::once_flag flag;

void initialize() {
    std::call_once(flag, [] {
        config = std::make_unique<Config>();
        config->load();
    });
}
```

After `initialize()` returns successfully:

```text
the caller can safely observe initialized config
```

assuming later code does not modify it unsafely.

---

## 10. once_flag Cannot Be Reset

`std::once_flag` is intended for one successful execution.

You cannot do:

```cpp
flag.reset();
```

There is no reset operation.

It is also not copyable or assignable.

If you need repeated lifecycle initialization, use another state machine or mutex-based design.

---

## 11. Function-Local Static Initialization

Modern C++ also guarantees thread-safe initialization of function-local statics.

Example:

```cpp
Config& getConfig() {
    static Config config = loadConfig();
    return config;
}
```

Since C++11, concurrent first calls are synchronized.

Only one thread initializes `config`.

Other threads wait.

This is often the simplest singleton-like pattern.

---

## 12. call_once vs Local Static

Use function-local static when:

```text
you want one object initialized on first use
the initialization fits directly in object construction
```

Example:

```cpp
Logger& logger() {
    static Logger instance;
    return instance;
}
```

Use `call_once` when:

```text
initialization modifies multiple objects
initialization logic is separate
you need explicit one-time action
```

Example:

```cpp
std::call_once(flag, [] {
    loadConfig();
    initializeMetrics();
    registerHandlers();
});
```

---

## 13. Singleton Warning

Thread-safe initialization does not automatically make the object thread-safe.

Example:

```cpp
Logger& logger() {
    static Logger instance;
    return instance;
}
```

Initialization is safe.

But if many threads later modify `instance` concurrently:

```text
Logger itself still needs synchronization
```

One-time construction safety and ongoing access safety are separate issues.

---

## 14. What Is thread_local?

A variable declared:

```cpp
thread_local int value = 0;
```

has one instance per thread.

If three threads access it:

```text
thread A has its own value
thread B has its own value
thread C has its own value
```

Changes in A do not change B's copy.

---

## 15. thread_local Example

```cpp
thread_local int requestCount = 0;

void processRequest() {
    ++requestCount;

    std::cout << requestCount;
}
```

Each thread tracks its own count independently.

If two threads each process three requests:

```text
thread A sees 1, 2, 3
thread B sees 1, 2, 3
```

There is no shared counter between them.

---

## 16. thread_local Is Not Shared

Normal global variable:

```cpp
int counter = 0;
```

All threads share one object.

Concurrent modification requires synchronization.

Thread-local global:

```cpp
thread_local int counter = 0;
```

Every thread gets a separate object.

No synchronization is needed between threads for their separate instances.

But code within the same thread still accesses the same local instance.

---

## 17. thread_local Lifetime

A namespace-scope thread-local object is generally:

```text
constructed for a thread when first needed
destroyed when that thread exits
```

Exact initialization timing depends on how it is declared and used.

Each thread runs construction/destruction for its own instance.

---

## 18. thread_local With Non-Trivial Types

You can use:

```cpp
thread_local std::string buffer;
thread_local std::vector<int> cache;
```

Each thread gets its own `string` or `vector`.

Their constructors and destructors run per thread.

This can avoid repeated allocation or shared locking.

---

## 19. Common Uses of thread_local

Examples:

```text
per-thread random number generator
scratch buffer
thread-local cache
request context
logging context
temporary allocator
error state
```

Example:

```cpp
thread_local std::mt19937 rng;
```

Each thread has its own RNG state.

---

## 20. Thread-Local Scratch Buffer

```cpp
thread_local std::string scratch;

std::string_view formatValue(int value) {
    scratch = std::to_string(value);
    return scratch;
}
```

This avoids sharing one global scratch buffer between threads.

But there are still lifetime and reuse concerns.

A later call in the same thread overwrites the buffer.

Returning a view to thread-local storage can therefore still be dangerous if stored long-term.

---

## 21. thread_local Does Not Mean Function-Local

This:

```cpp
void f() {
    thread_local int counter = 0;
    ++counter;
}
```

The variable is declared inside a function, but it is not recreated on every call.

It behaves like:

```text
one persistent counter per thread
```

Each thread remembers its own value between calls.

---

## 22. Normal Local vs static vs thread_local

### Normal local

```cpp
void f() {
    int x = 0;
}
```

One new `x` per function call.

### static local

```cpp
void f() {
    static int x = 0;
}
```

One shared `x` for the whole process.

All threads access the same object.

### thread_local local

```cpp
void f() {
    thread_local int x = 0;
}
```

One persistent `x` per thread.

---

## 23. static thread_local

You may see:

```cpp
static thread_local int x = 0;
```

At block or namespace scope, `thread_local` determines thread storage duration.

`static` may affect linkage depending on the declaration context.

For the basic mental model:

```text
thread_local means one instance per thread
```

---

## 24. thread_local Does Not Solve All Races

Suppose each thread has:

```cpp
thread_local int localCount;
```

That part is independent.

But if threads later update:

```cpp
globalTotal += localCount;
```

the global total is shared and still requires synchronization.

Thread-local storage only protects the variable that is actually thread-local.

---

## 25. Reducing Lock Contention

Instead of every event updating one global counter:

```cpp
std::mutex mutex;
int globalCount;
```

each thread can update:

```cpp
thread_local int localCount;
```

Then occasionally merge into global state under a lock.

This can reduce contention.

But aggregation and thread exit handling must be designed carefully.

---

## 26. Memory Cost

Every thread gets its own instance.

Example:

```cpp
thread_local std::array<char, 1'000'000> buffer;
```

With 100 threads, memory usage can become very large.

Thread-local storage trades shared contention for per-thread memory.

---

## 27. Thread Pools and thread_local

In a thread pool, workers are long-lived.

A thread-local variable persists across different tasks executed by the same worker.

Example:

```cpp
thread_local std::string currentUser;
```

Task A sets:

```text
currentUser = "Alice"
```

Later Task B runs on the same worker.

If Task A did not clear it, Task B may observe stale state.

This is a very important thread-pool pitfall.

---

## 28. Request Context Leakage

Bad:

```cpp
thread_local std::string requestId;

void handleRequest(const Request& req) {
    requestId = req.id;

    process(req);

    // forgot to clear
}
```

In a thread pool, the same worker handles another request later.

The old request context may leak into logs or error paths.

Use RAII to restore or clear thread-local state.

---

## 29. RAII Thread-Local Context Guard

Conceptually:

```cpp
class RequestContextGuard {
private:
    std::string oldValue;

public:
    RequestContextGuard(std::string value)
        : oldValue(std::move(requestId)) {
        requestId = std::move(value);
    }

    ~RequestContextGuard() {
        requestId = std::move(oldValue);
    }
};
```

This safely restores previous thread-local state when scope exits.

---

## 30. once_flag vs atomic bool

Incorrect one-time initialization:

```cpp
std::atomic<bool> initialized = false;

if (!initialized.exchange(true)) {
    initialize();
}
```

Problem:

```text
initialized becomes true before initialize() necessarily succeeds
```

If initialization throws or partially fails, other threads may believe it completed.

`call_once` correctly tracks successful completion and exception retry.

---

## 31. Double-Checked Locking

You may see:

```cpp
if (!initialized) {
    lock_guard lock(mutex);

    if (!initialized) {
        initialize();
        initialized = true;
    }
}
```

Correctly implementing this requires careful atomic and memory-order reasoning.

Prefer:

```cpp
std::call_once
```

or function-local static initialization.

---

## 32. Reentrant call_once Danger

If the callable executed by:

```cpp
std::call_once(flag, callable);
```

recursively calls `call_once` with the same flag before completing, it can deadlock or otherwise fail to make progress.

Avoid recursively depending on the same initialization flag.

---

## 33. Common Wrong Patterns

### Wrong: unsynchronized initialization flag

```cpp
if (!initialized) {
    initialize();
}
```

Multiple threads may initialize concurrently.

### Wrong: assume initialization safety means object safety

```text
constructed once
does not mean safe concurrent mutation forever
```

### Wrong: treat thread_local as per-task

In thread pools:

```text
thread-local state persists across tasks
```

### Wrong: huge thread-local objects

Memory usage scales with thread count.

### Wrong: use atomic bool instead of call_once

It may mishandle exceptions and publication.

---

## 34. Common Interview Questions

### Q1. What does std::call_once guarantee?

It guarantees that among all calls using the same once flag, one invocation completes successfully exactly once. Other callers wait or return after successful completion.

### Q2. What happens if the callable throws?

The once flag is not marked complete. Another call may try the callable again.

### Q3. Is function-local static initialization thread-safe?

Yes, since C++11 its first initialization is synchronized.

### Q4. What does thread_local mean?

Each thread has its own independent instance of the variable.

### Q5. Does thread_local make all surrounding code thread-safe?

No. It only makes that particular variable non-shared. Other shared data still requires synchronization.

### Q6. What is a thread-pool danger with thread_local?

Worker threads persist across tasks, so thread-local values can leak from one task or request into another unless reset.

---

## 35. Key Takeaways

- `once_flag` stores one-time execution state.
- `call_once` guarantees one successful completion.
- If initialization throws, another call may retry.
- Function-local static initialization is thread-safe since C++11.
- One-time initialization safety is not ongoing object thread safety.
- `thread_local` gives each thread its own instance.
- Thread-local objects persist for the lifetime of the thread.
- In thread pools, thread-local state persists across tasks.
- Use RAII to restore per-thread context.
- Thread-local storage can reduce contention but increases memory usage.
