# 32. future, promise, and async

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. Shared State](#2-shared-state)
- [3. Basic promise and future](#3-basic-promise-and-future)
- [4. future::get()](#4-futureget)
- [5. future::wait()](#5-futurewait)
- [6. promise set_value](#6-promise-set_value)
- [7. Passing promise to a thread](#7-passing-promise-to-a-thread)
- [8. Exception propagation](#8-exception-propagation)
- [9. Broken promise](#9-broken-promise)
- [10. std::async](#10-stdasync)
- [11. launch policies](#11-launch-policies)
- [12. Deferred execution](#12-deferred-execution)
- [13. async exception handling](#13-async-exception-handling)
- [14. future destructor and async](#14-future-destructor-and-async)
- [15. future is move-only](#15-future-is-move-only)
- [16. shared_future](#16-shared_future)
- [17. future vs shared_future](#17-future-vs-shared_future)
- [18. packaged_task](#18-packaged_task)
- [19. packaged_task and thread](#19-packaged_task-and-thread)
- [20. promise vs packaged_task vs async](#20-promise-vs-packaged_task-vs-async)
- [21. Waiting with timeout](#21-waiting-with-timeout)
- [22. wait_for example](#22-wait_for-example)
- [23. No automatic cancellation](#23-no-automatic-cancellation)
- [24. Common wrong patterns](#24-common-wrong-patterns)
- [25. Memory synchronization](#25-memory-synchronization)
- [26. Common Interview Questions](#26-common-interview-questions)
- [27. Key Takeaways](#27-key-takeaways)

## Related Code Trap

- Demo file: [future_promise_async.cpp](../code_traps/future_promise_async.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

Threads often need to return values.

A normal `std::thread` does not directly return a value:

```cpp
std::thread t(worker);
```

To communicate a result, C++ provides:

```cpp
std::promise<T>
std::future<T>
std::async
std::packaged_task
```

Core relationship:

```text
promise writes result
future receives result
```

Both refer to a shared state.

---

## 2. Shared State

A promise and its future communicate through shared state.

Conceptually:

```text
promise ----\
             shared state: value / exception / readiness
future  ----/
```

The promise side can set:

```cpp
promise.set_value(result);
promise.set_exception(...);
```

The future side can call:

```cpp
future.get();
future.wait();
```

---

## 3. Basic promise and future

```cpp
std::promise<int> promise;
std::future<int> future = promise.get_future();
```

Then one thread sets the value:

```cpp
promise.set_value(42);
```

Another thread receives it:

```cpp
int result = future.get();
```

If the value is not ready, `get()` blocks.

---

## 4. future::get()

```cpp
int result = future.get();
```

`get()` does two things:

```text
wait until shared state is ready
retrieve the value or rethrow stored exception
```

Important:

```text
future.get() can normally be called only once
```

After `get()`, the future no longer owns a retrievable result.

You can check:

```cpp
future.valid()
```

---

## 5. future::wait()

```cpp
future.wait();
```

This waits until the result is ready, but does not retrieve it.

Later:

```cpp
int result = future.get();
```

Difference:

```text
wait() waits only
get() waits and consumes the result
```

---

## 6. promise set_value

```cpp
std::promise<int> promise;

promise.set_value(42);
```

This marks shared state ready and wakes any waiting future.

Calling `set_value()` more than once is an error.

A promise represents one result, not a stream of results.

---

## 7. Passing promise to a thread

`std::promise` is move-only.

Example:

```cpp
void worker(std::promise<int> promise) {
    promise.set_value(42);
}

std::promise<int> promise;
std::future<int> future = promise.get_future();

std::thread t(worker, std::move(promise));
```

Why move?

Because the thread must take ownership of the producer side.

You cannot copy a promise.

---

## 8. Exception propagation

A worker can send an exception through the promise.

```cpp
try {
    int result = riskyWork();
    promise.set_value(result);
} catch (...) {
    promise.set_exception(std::current_exception());
}
```

Then:

```cpp
future.get();
```

rethrows the same exception in the receiving thread.

This is very useful.

Without this, an uncaught exception inside a thread calls:

```cpp
std::terminate()
```

---

## 9. Broken promise

Suppose a promise is destroyed without setting a value or exception:

```cpp
std::promise<int> promise;
std::future<int> future = promise.get_future();

// promise is destroyed
```

Then:

```cpp
future.get();
```

throws:

```cpp
std::future_error
```

with broken-promise state.

Meaning:

```text
producer disappeared without producing a result
```

---

## 10. std::async

`std::async` is a convenient way to run a callable and get a future.

```cpp
std::future<int> future = std::async([] {
    return 42;
});
```

Then:

```cpp
int result = future.get();
```

The return value automatically becomes the future result.

Exceptions are also automatically stored and rethrown by `get()`.

---

## 11. launch policies

`std::async` can use launch policies.

### std::launch::async

```cpp
auto future = std::async(std::launch::async, work);
```

This requests asynchronous execution, typically on another thread.

### std::launch::deferred

```cpp
auto future = std::async(std::launch::deferred, work);
```

The function does not run immediately.

It runs when:

```cpp
future.get();
future.wait();
```

is first called.

### Default policy

```cpp
std::async(work);
```

The implementation may choose:

```text
async
or deferred
```

So if you require actual concurrent execution, specify:

```cpp
std::launch::async
```

---

## 12. Deferred execution

Example:

```cpp
auto future = std::async(std::launch::deferred, [] {
    return 42;
});
```

The callable runs in the thread that calls:

```cpp
future.get();
```

It may therefore run in the main thread.

Deferred does not mean background thread.

---

## 13. async exception handling

```cpp
auto future = std::async(std::launch::async, []() -> int {
    throw std::runtime_error("failed");
});
```

The exception does not immediately escape into the caller.

It is stored in shared state.

Later:

```cpp
try {
    future.get();
} catch (const std::exception& e) {
    // receives worker exception
}
```

---

## 14. future destructor and async

A subtle behavior:

```cpp
std::async(std::launch::async, work);
```

If the returned future is a temporary and immediately destroyed, its destructor may wait for the async task.

This can accidentally make code synchronous.

Bad:

```cpp
std::async(std::launch::async, work1);
std::async(std::launch::async, work2);
```

These may execute one after another because each temporary future waits on destruction.

Better:

```cpp
auto f1 = std::async(std::launch::async, work1);
auto f2 = std::async(std::launch::async, work2);

f1.get();
f2.get();
```

---

## 15. future is move-only

```cpp
std::future<int> f1 = ...;
std::future<int> f2 = std::move(f1);
```

`std::future` cannot be copied.

Why?

Because it represents exclusive access to retrieve the result once.

After move:

```text
f1 is invalid
f2 owns the future result
```

Check with:

```cpp
f1.valid()
```

---

## 16. shared_future

If multiple consumers need the same result:

```cpp
std::shared_future<int> sf = future.share();
```

A `shared_future` can be copied.

Multiple threads can call:

```cpp
sf.get();
```

Unlike `future`, its result is not consumed by the first `get()`.

---

## 17. future vs shared_future

### future

```text
move-only
single consumer
get normally once
```

### shared_future

```text
copyable
multiple consumers
get multiple times
```

Use `shared_future` when several threads wait for one result.

---

## 18. packaged_task

`std::packaged_task` wraps a callable and connects its return value to a future.

```cpp
std::packaged_task<int(int, int)> task(
    [](int a, int b) {
        return a + b;
    }
);

std::future<int> future = task.get_future();
```

Execute task:

```cpp
task(2, 3);
```

Then:

```cpp
future.get(); // 5
```

This is useful for:

```text
thread pools
task queues
job systems
```

---

## 19. packaged_task and thread

```cpp
std::packaged_task<int()> task([] {
    return 42;
});

std::future<int> future = task.get_future();

std::thread t(std::move(task));

int result = future.get();

t.join();
```

`packaged_task` is move-only because it owns the callable and shared state connection.

---

## 20. promise vs packaged_task vs async

### promise

Use when you manually decide when and how to produce the result.

```text
manual producer
set_value/set_exception explicitly
```

### packaged_task

Use when you already have a callable and want its return value connected to a future.

```text
wrap callable into task
```

### async

Use for convenient one-shot asynchronous function execution.

```text
run callable and get future directly
```

---

## 21. Waiting with timeout

A future supports:

```cpp
future.wait_for(duration);
future.wait_until(deadline);
```

Example:

```cpp
auto status = future.wait_for(std::chrono::seconds(1));
```

Possible results:

```cpp
std::future_status::ready
std::future_status::timeout
std::future_status::deferred
```

---

## 22. wait_for example

```cpp
auto status = future.wait_for(std::chrono::milliseconds(100));

if (status == std::future_status::ready) {
    std::cout << future.get();
} else if (status == std::future_status::timeout) {
    std::cout << "not ready";
} else {
    std::cout << "deferred";
}
```

Note:

```text
wait_for does not cancel the task
```

It only tells you whether the result is ready.

---

## 23. No automatic cancellation

Standard `future` does not provide a general:

```cpp
future.cancel()
```

If cancellation is needed, the task must cooperate using:

```cpp
std::atomic<bool>
std::stop_token
std::jthread
```

The future only communicates completion/result.

---

## 24. Common wrong patterns

### Wrong: forgetting to move promise

```cpp
std::thread t(worker, promise); // error, promise not copyable
```

Correct:

```cpp
std::thread t(worker, std::move(promise));
```

### Wrong: calling get twice

```cpp
int a = future.get();
int b = future.get(); // error
```

### Wrong: ignoring async future

```cpp
std::async(std::launch::async, work);
```

Temporary future destruction may block.

### Wrong: assuming default async always creates thread

```cpp
std::async(work);
```

May use deferred execution.

### Wrong: uncaught exception in raw thread

```cpp
std::thread t([] {
    throw std::runtime_error("error");
});
```

This leads to `std::terminate` if exception escapes the thread function.

Use promise/async or catch inside thread.

---

## 25. Memory synchronization

When a promise sets a value:

```cpp
promise.set_value(result);
```

and future successfully waits/gets it:

```cpp
future.get();
```

the shared state provides synchronization.

Writes performed before setting the result become visible to the receiving side according to the synchronization relationship.

Still, avoid using unrelated unsynchronized shared state.

Prefer passing the result itself.

---

## 26. Common Interview Questions

### Q1. What is the relationship between promise and future?

A promise produces a value or exception, and a future waits for and retrieves it through shared state.

### Q2. Does future::get block?

Yes. If the result is not ready, `get()` blocks until it becomes ready.

### Q3. Can future::get be called more than once?

Normally no. `std::future` provides single-consumer access, and `get()` consumes the result.

Use `shared_future` for multiple consumers.

### Q4. How are exceptions transferred?

The producer stores an exception in shared state. `future::get()` rethrows it in the consumer thread.

### Q5. Difference between async and thread?

`std::thread` only manages execution. `std::async` also captures the function's return value or exception in a future.

### Q6. Does std::async always create a new thread?

Not with the default policy. It may choose async or deferred execution.

Use `std::launch::async` if actual asynchronous execution is required.

---

## 27. Key Takeaways

- `promise` produces a result.
- `future` consumes the result.
- They communicate through shared state.
- `future.get()` waits and retrieves the value.
- `future.get()` rethrows worker exceptions.
- `promise` and `future` are move-only.
- `shared_future` supports multiple consumers.
- `std::async` conveniently returns a future.
- Default async policy may be deferred.
- `packaged_task` connects callable return values to futures.
- Standard future does not provide automatic cancellation.
