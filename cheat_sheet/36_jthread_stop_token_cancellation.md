# 36. std::jthread, stop_token, and Cooperative Cancellation

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. Why std::thread Is Easy to Misuse](#2-why-stdthread-is-easy-to-misuse)
- [3. std::jthread](#3-stdjthread)
- [4. jthread Basic Example](#4-jthread-basic-example)
- [5. stop_token Injection](#5-stop_token-injection)
- [6. request_stop](#6-request_stop)
- [7. Cooperative Cancellation](#7-cooperative-cancellation)
- [8. Why Not Force-Kill a Thread?](#8-why-not-force-kill-a-thread)
- [9. Destructor Behavior of jthread](#9-destructor-behavior-of-jthread)
- [10. jthread Without stop_token](#10-jthread-without-stop_token)
- [11. stop_source and stop_token](#11-stop_source-and-stop_token)
- [12. Stop State Model](#12-stop-state-model)
- [13. request_stop Return Value](#13-request_stop-return-value)
- [14. stop_possible](#14-stop_possible)
- [15. stop_callback](#15-stop_callback)
- [16. Why stop_callback Is Useful](#16-why-stop_callback-is-useful)
- [17. stop_callback Execution Thread](#17-stop_callback-execution-thread)
- [18. stop_token With condition_variable](#18-stop_token-with-condition_variable)
- [19. condition_variable_any Stop-Aware Wait](#19-condition_variable_any-stop-aware-wait)
- [20. Worker Queue With jthread](#20-worker-queue-with-jthread)
- [21. Cancellation Granularity](#21-cancellation-granularity)
- [22. Cancellation Point](#22-cancellation-point)
- [23. Cleanup During Cancellation](#23-cleanup-during-cancellation)
- [24. stop_token Is Not an Exception](#24-stop_token-is-not-an-exception)
- [25. stop_token Is Not an Atomic Bool Replacement in Every Case](#25-stop_token-is-not-an-atomic-bool-replacement-in-every-case)
- [26. Multiple Workers Sharing One Stop Source](#26-multiple-workers-sharing-one-stop-source)
- [27. jthread Has Its Own Stop Source](#27-jthread-has-its-own-stop-source)
- [28. jthread Move Semantics](#28-jthread-move-semantics)
- [29. Explicit join Is Still Allowed](#29-explicit-join-is-still-allowed)
- [30. Dangerous Self-Join Situation](#30-dangerous-self-join-situation)
- [31. Common Wrong Pattern: Worker Ignores Stop](#31-common-wrong-pattern-worker-ignores-stop)
- [32. Common Wrong Pattern: Blocking Forever](#32-common-wrong-pattern-blocking-forever)
- [33. Common Wrong Pattern: Unsafe Callback](#33-common-wrong-pattern-unsafe-callback)
- [34. std::thread vs std::jthread](#34-stdthread-vs-stdjthread)
- [35. Common Interview Questions](#35-common-interview-questions)
- [36. Key Takeaways](#36-key-takeaways)

## Related Code Trap

- Demo file: [jthread_stop_token_cancellation.cpp](../code_traps/jthread_stop_token_cancellation.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

C++20 provides:

```cpp
std::jthread
std::stop_source
std::stop_token
std::stop_callback
```

They support cooperative thread cancellation.

Meaning:

```text
one side requests stop
worker checks the request
worker exits cleanly
```

This is not forced thread termination.

---

## 2. Why std::thread Is Easy to Misuse

With `std::thread`:

```cpp
std::thread t(worker);
```

Before `t` is destroyed, you must:

```cpp
t.join();
```

or:

```cpp
t.detach();
```

Otherwise:

```text
std::terminate()
```

This creates cleanup risk.

---

## 3. std::jthread

`std::jthread` is an RAII-friendly thread.

Example:

```cpp
std::jthread t([] {
    doWork();
});
```

When `t` goes out of scope:

```text
request stop
join thread
```

So you normally do not need to call `join()` manually.

---

## 4. jthread Basic Example

```cpp
void f() {
    std::jthread t([] {
        std::cout << "worker\n";
    });
}
```

At end of `f`:

```text
jthread destructor runs
thread is joined automatically
```

Unlike `std::thread`, destruction does not call terminate just because it is still joinable.

---

## 5. stop_token Injection

A `jthread` can automatically pass a `std::stop_token` to the callable.

```cpp
std::jthread t([](std::stop_token token) {
    while (!token.stop_requested()) {
        doWork();
    }
});
```

The token lets the worker check whether stop was requested.

---

## 6. request_stop

Another thread can request cancellation:

```cpp
t.request_stop();
```

This does not kill the worker immediately.

It changes stop state so:

```cpp
token.stop_requested()
```

becomes true.

The worker must cooperate.

---

## 7. Cooperative Cancellation

Example:

```cpp
std::jthread worker([](std::stop_token token) {
    while (!token.stop_requested()) {
        processOneItem();
    }

    cleanup();
});
```

The worker:

```text
checks token
finishes current safe unit of work
cleans up
returns normally
```

This is cooperative cancellation.

---

## 8. Why Not Force-Kill a Thread?

Forcefully terminating a thread could interrupt it while:

- holding a mutex
- modifying shared data
- allocating memory
- writing a file
- updating an invariant

Then resources and shared state may be left corrupted.

Cooperative cancellation lets the thread stop at safe points.

---

## 9. Destructor Behavior of jthread

Conceptually:

```cpp
~jthread() {
    if (joinable()) {
        request_stop();
        join();
    }
}
```

So destructor may block until worker exits.

Important:

```text
the worker must eventually observe stop and return
```

If it ignores stop forever, destructor may wait forever.

---

## 10. jthread Without stop_token

This is valid:

```cpp
std::jthread t([] {
    doWork();
});
```

The callable does not have to accept a stop token.

But then `request_stop()` has no effect unless the callable accesses the stop state another way.

Automatic join still works.

---

## 11. stop_source and stop_token

A `std::stop_source` owns the ability to request stop.

```cpp
std::stop_source source;
std::stop_token token = source.get_token();
```

Requester:

```cpp
source.request_stop();
```

Observer:

```cpp
if (token.stop_requested()) {
    return;
}
```

Multiple tokens can observe the same stop state.

---

## 12. Stop State Model

Conceptually:

```text
stop_source ----\
                 shared stop state
stop_token  ----/
stop_token  ----/
```

The source can request stop.

Tokens only observe stop state.

---

## 13. request_stop Return Value

```cpp
bool first = source.request_stop();
```

Returns:

```text
true  if this call successfully changed state to requested
false if stop had already been requested or no state exists
```

Stop request is one-way.

Once requested:

```text
it stays requested
```

You cannot reset the same stop state.

---

## 14. stop_possible

A token can check:

```cpp
token.stop_possible()
```

Meaning:

```text
this token is associated with a stop state that could be requested
```

And:

```cpp
token.stop_requested()
```

means:

```text
stop has already been requested
```

---

## 15. stop_callback

`std::stop_callback` runs a callback when stop is requested.

Example:

```cpp
std::stop_callback callback(token, [] {
    std::cout << "stop requested\n";
});
```

If stop is already requested when callback is created, callback may run immediately during construction.

---

## 16. Why stop_callback Is Useful

A worker may be blocked in another API.

A stop callback can trigger some wakeup action:

```cpp
std::stop_callback callback(token, [&] {
    cv.notify_all();
});
```

Then the blocked thread wakes and checks stop state.

The callback should be short and thread-safe.

---

## 17. stop_callback Execution Thread

The callback may execute in the thread that calls:

```cpp
request_stop()
```

Therefore it should not assume it runs in the worker thread.

Avoid:

- long work
- blocking operations
- unsafe access
- acquiring locks in dangerous order

---

## 18. stop_token With condition_variable

A basic condition-variable wait:

```cpp
cv.wait(lock, [&] {
    return stopped || !queue.empty();
});
```

With stop token, the predicate can include:

```cpp
token.stop_requested() || !queue.empty()
```

But ordinary `std::condition_variable` wait still needs a notification to wake.

A stop callback can notify:

```cpp
std::stop_callback callback(token, [&] {
    cv.notify_all();
});
```

---

## 19. condition_variable_any Stop-Aware Wait

C++20 `std::condition_variable_any` has stop-token-aware waiting overloads.

Conceptually:

```cpp
cv.wait(lock, token, predicate);
```

It wakes when:

```text
predicate becomes true
or stop is requested
```

This simplifies cancellation-aware waiting.

---

## 20. Worker Queue With jthread

A worker loop may look like:

```cpp
void worker(std::stop_token token) {
    while (!token.stop_requested()) {
        Task task;

        if (!getTask(task, token)) {
            return;
        }

        task();
    }
}
```

Shutdown:

```cpp
workerThread.request_stop();
```

Then wake any blocking wait and let worker exit.

---

## 21. Cancellation Granularity

This loop:

```cpp
while (!token.stop_requested()) {
    doHugeTaskForOneHour();
}
```

is technically cooperative but responds slowly.

The stop request is checked only once per large task.

Better:

```cpp
for (...) {
    if (token.stop_requested()) {
        return;
    }

    processChunk();
}
```

Cancellation responsiveness depends on how often safe checks occur.

---

## 22. Cancellation Point

A cancellation point is a place where the worker checks:

```cpp
token.stop_requested()
```

Good cancellation points:

- between tasks
- between chunks
- before expensive operation
- after waking from wait
- before acquiring another resource

Avoid leaving invariants half-updated.

---

## 23. Cleanup During Cancellation

Since worker exits normally:

```cpp
return;
```

RAII still works.

Local objects are destroyed:

```text
mutex guards unlock
files close
smart pointers release
buffers clean up
```

This is a major advantage of cooperative cancellation.

---

## 24. stop_token Is Not an Exception

A stop request does not automatically throw.

This:

```cpp
t.request_stop();
```

does not interrupt control flow by itself.

The worker must explicitly:

```cpp
if (token.stop_requested()) {
    return;
}
```

You may design your own cancellation exception, but stop_token itself does not throw one automatically.

---

## 25. stop_token Is Not an Atomic Bool Replacement in Every Case

An atomic bool can also signal stop:

```cpp
std::atomic<bool> stop = false;
```

But stop_token adds:

- standardized shared stop state
- multiple observers
- stop_source
- callbacks
- integration with jthread
- integration with stop-aware waits

For simple code, atomic bool may work.

For structured cancellation, stop_token is cleaner.

---

## 26. Multiple Workers Sharing One Stop Source

```cpp
std::stop_source source;
auto token = source.get_token();
```

Several workers can receive the same token.

Then:

```cpp
source.request_stop();
```

requests stop for all of them.

This is useful for worker groups.

---

## 27. jthread Has Its Own Stop Source

Each `jthread` internally owns a stop source.

You can get its token:

```cpp
std::stop_token token = t.get_stop_token();
```

And request stop through:

```cpp
t.request_stop();
```

---

## 28. jthread Move Semantics

`std::jthread` is move-only.

```cpp
std::jthread a(worker);
std::jthread b = std::move(a);
```

After move:

```text
b owns the thread
a is no longer joinable
```

This is similar to `std::thread`.

---

## 29. Explicit join Is Still Allowed

You can still do:

```cpp
t.join();
```

if you want to wait before scope exit.

But often RAII destruction is enough.

You can check:

```cpp
t.joinable()
```

---

## 30. Dangerous Self-Join Situation

A thread must not join itself.

If a `jthread` object is destroyed from the same thread it represents, joining itself is invalid and can lead to error/deadlock behavior.

Ownership design should ensure the thread object is managed externally.

---

## 31. Common Wrong Pattern: Worker Ignores Stop

```cpp
std::jthread worker([](std::stop_token) {
    while (true) {
        doWork();
    }
});
```

Destructor requests stop, but worker never checks it.

Then destructor waits forever in join.

---

## 32. Common Wrong Pattern: Blocking Forever

```cpp
cv.wait(lock);
```

If worker is blocked and stop is requested but nobody notifies the condition variable, it may never wake.

Solutions:

- notify during shutdown
- use stop_callback
- use stop-aware `condition_variable_any`
- use timed waits

---

## 33. Common Wrong Pattern: Unsafe Callback

```cpp
std::stop_callback callback(token, [&] {
    doLongBlockingWork();
});
```

The callback may run in the requester thread and delay `request_stop()`.

Keep callback short.

---

## 34. std::thread vs std::jthread

### std::thread

```text
manual join/detach
destroying joinable thread terminates program
no built-in stop mechanism
```

### std::jthread

```text
automatic stop request and join on destruction
built-in stop token support
better RAII behavior
```

Use `jthread` by default in C++20 when automatic joining and cooperative stop are useful.

---

## 35. Common Interview Questions

### Q1. What is std::jthread?

`std::jthread` is an RAII thread type that automatically requests stop and joins when destroyed.

### Q2. Does request_stop kill the thread?

No. It only marks the shared stop state as requested. The worker must check the token and exit cooperatively.

### Q3. Why is cooperative cancellation safer?

Because the worker can stop at safe points, restore invariants, release locks, and run destructors normally.

### Q4. What is stop_source?

It is the producer side of cancellation. It can request stop.

### Q5. What is stop_token?

It is the observer side. Workers use it to check whether cancellation was requested.

### Q6. Why can jthread destructor still block?

Because it joins the worker. If the worker ignores stop or remains blocked forever, destruction waits forever.

---

## 36. Key Takeaways

- `jthread` automatically joins.
- Its destructor requests stop before joining.
- Stop is cooperative, not forced.
- Workers must check `stop_requested()`.
- Cancellation should happen at safe points.
- RAII cleanup still runs when worker exits normally.
- `stop_source` requests cancellation.
- `stop_token` observes cancellation.
- `stop_callback` reacts to cancellation.
- Blocked workers still need a wakeup mechanism.
- Prefer `jthread` over raw `thread` for structured C++20 worker lifetimes.
