# 34. Semaphore, Workers, and Thread Pool Basics

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. What Is a Worker?](#2-what-is-a-worker)
- [3. Worker Is Not the Same as Task](#3-worker-is-not-the-same-as-task)
- [4. Thread Pool](#4-thread-pool)
- [5. Why Use a Thread Pool?](#5-why-use-a-thread-pool)
- [6. Mutex vs Semaphore](#6-mutex-vs-semaphore)
- [7. counting_semaphore](#7-counting_semaphore)
- [8. Semaphore Example](#8-semaphore-example)
- [9. Semaphore Does Not Protect Arbitrary Shared Data](#9-semaphore-does-not-protect-arbitrary-shared-data)
- [10. binary_semaphore](#10-binary_semaphore)
- [11. Semaphore vs Mutex Ownership](#11-semaphore-vs-mutex-ownership)
- [12. RAII for Semaphore](#12-raii-for-semaphore)
- [13. try_acquire](#13-try_acquire)
- [14. Limiting Concurrent Requests](#14-limiting-concurrent-requests)
- [15. Workers and Semaphore Together](#15-workers-and-semaphore-together)
- [16. Does One Worker Mean One Thread?](#16-does-one-worker-mean-one-thread)
- [17. Worker Loop](#17-worker-loop)
- [18. Critical Thread Pool Detail](#18-critical-thread-pool-detail)
- [19. submit()](#19-submit)
- [20. Graceful Shutdown](#20-graceful-shutdown)
- [21. Immediate vs Graceful Shutdown](#21-immediate-vs-graceful-shutdown)
- [22. Backpressure](#22-backpressure)
- [23. Semaphore vs Condition Variable](#23-semaphore-vs-condition-variable)
- [24. Semaphore vs Atomic Counter](#24-semaphore-vs-atomic-counter)
- [25. CPU Concurrency vs Parallelism](#25-cpu-concurrency-vs-parallelism)
- [26. Too Many Workers](#26-too-many-workers)
- [27. CPU-Bound vs I/O-Bound](#27-cpu-bound-vs-io-bound)
- [28. Thread Pool and Future](#28-thread-pool-and-future)
- [29. Common Wrong Patterns](#29-common-wrong-patterns)
- [30. Common Interview Questions](#30-common-interview-questions)
- [31. Key Takeaways](#31-key-takeaways)

## Related Code Trap

- Demo file: [semaphore_worker_thread_pool.cpp](../code_traps/semaphore_worker_thread_pool.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

Concurrency systems commonly use:

```cpp
std::thread
std::mutex
std::condition_variable
std::semaphore
```

Their roles are different:

```text
thread                 executes work
mutex                  allows one thread into critical section
condition_variable     lets threads sleep until state changes
semaphore              allows up to N threads/resources at once
```

A worker is usually one thread repeatedly taking tasks from a queue.

Multiple workers mean multiple threads can process different tasks concurrently.

---

## 2. What Is a Worker?

A worker is not a special C++ type.

It is usually just:

```text
one thread
running a loop
waiting for tasks
processing tasks
```

Example:

```cpp
void workerLoop() {
    while (true) {
        Task task = queue.pop();
        task();
    }
}
```

If a thread pool has:

```text
3 workers
```

that usually means:

```text
3 threads
```

Each worker thread can process one task at a time.

Therefore up to 3 tasks may run concurrently.

---

## 3. Worker Is Not the Same as Task

Suppose:

```text
workers = 3
tasks = 100
```

The system does not create 100 threads.

Instead:

```text
worker 1 processes task 1
worker 2 processes task 2
worker 3 processes task 3
```

When one finishes, it takes the next task.

So workers are reusable execution threads.

Tasks are units of work.

---

## 4. Thread Pool

A thread pool usually contains:

```text
task queue
mutex
condition_variable
worker threads
shutdown flag
```

Conceptual model:

```text
producers submit tasks
        |
        v
    task queue
        |
        v
worker threads pop tasks and execute them
```

Example:

```cpp
pool.submit(taskA);
pool.submit(taskB);
pool.submit(taskC);
```

Workers wait on the queue using condition variable.

---

## 5. Why Use a Thread Pool?

Creating a thread for every task can be expensive.

Costs include:

```text
thread creation
thread destruction
stack allocation
scheduler overhead
too many runnable threads
```

Thread pool advantages:

```text
reuse worker threads
limit concurrency
avoid unbounded thread creation
central task queue
```

---

## 6. Mutex vs Semaphore

### mutex

```cpp
std::mutex m;
```

Only one thread may own it.

Used to protect shared state:

```cpp
{
    std::lock_guard<std::mutex> lock(m);
    sharedData.push_back(value);
}
```

### semaphore

```cpp
std::counting_semaphore<3> sem(3);
```

Up to 3 threads may pass simultaneously.

Used to limit access to a finite resource:

```text
3 database connections
4 GPUs
10 network requests
5 concurrent file operations
```

---

## 7. counting_semaphore

C++20:

```cpp
#include <semaphore>
```

Example:

```cpp
std::counting_semaphore<3> sem(3);
```

The constructor argument is the initial count.

Operations:

```cpp
sem.acquire();
sem.release();
```

`acquire()`:

```text
if count > 0:
    decrement count and continue
else:
    block until another thread releases
```

`release()`:

```text
increment count
wake a waiting thread if needed
```

---

## 8. Semaphore Example

```cpp
std::counting_semaphore<2> sem(2);
```

Suppose four threads call `acquire()`.

Execution:

```text
thread A acquires: count 2 -> 1
thread B acquires: count 1 -> 0
thread C blocks
thread D blocks
```

When A calls:

```cpp
sem.release();
```

one blocked thread may continue.

So at most two threads enter the limited region.

---

## 9. Semaphore Does Not Protect Arbitrary Shared Data

This is important.

A semaphore limits concurrency count.

It does not automatically make a data structure safe.

Example:

```cpp
std::vector<int> values;
std::counting_semaphore<3> sem(3);
```

If three threads concurrently call:

```cpp
values.push_back(...)
```

the vector still has a data race.

Why?

Because three threads are allowed inside.

For protecting vector mutation, use a mutex.

Semaphore use:

```text
limit number of simultaneous users
```

Mutex use:

```text
ensure exclusive access to shared state
```

Sometimes both are needed.

---

## 10. binary_semaphore

C++20 also provides:

```cpp
std::binary_semaphore
```

Conceptually count is only 0 or 1.

Example:

```cpp
std::binary_semaphore signal(0);
```

One thread:

```cpp
signal.acquire();
```

Another:

```cpp
signal.release();
```

This can be used as an event/signal.

But a binary semaphore is not exactly the same abstraction as mutex ownership.

---

## 11. Semaphore vs Mutex Ownership

Mutex has ownership semantics:

```text
the thread that locks it should unlock it
```

Semaphore does not require the same thread to release.

Example:

```cpp
thread A acquires
thread B may release
```

This is useful for signaling/resource counting.

Therefore semaphore is not simply a "mutex with count".

---

## 12. RAII for Semaphore

Manual:

```cpp
sem.acquire();

doWork();

sem.release();
```

Problem:

```cpp
doWork();
```

may throw.

Then `release()` is skipped.

Use an RAII guard.

Conceptually:

```cpp
class SemaphoreGuard {
    semaphore& sem;

public:
    SemaphoreGuard(semaphore& s) : sem(s) {
        sem.acquire();
    }

    ~SemaphoreGuard() {
        sem.release();
    }
};
```

Then:

```cpp
SemaphoreGuard guard(sem);
doWork();
```

Release happens automatically on scope exit.

---

## 13. try_acquire

A semaphore can be tested without blocking:

```cpp
if (sem.try_acquire()) {
    doWork();
    sem.release();
} else {
    // no permit available
}
```

Timed versions:

```cpp
try_acquire_for(...)
try_acquire_until(...)
```

Useful when work should not wait forever.

---

## 14. Limiting Concurrent Requests

Suppose service receives many requests but only wants 4 expensive model inferences at once.

```cpp
std::counting_semaphore<4> inferenceSlots(4);
```

Each request:

```cpp
inferenceSlots.acquire();

runInference();

inferenceSlots.release();
```

Even if there are 100 request threads:

```text
only 4 execute inference simultaneously
the rest block waiting for permits
```

---

## 15. Workers and Semaphore Together

Suppose thread pool has:

```text
8 worker threads
```

but database only supports:

```text
3 simultaneous connections
```

Then:

```cpp
std::counting_semaphore<3> dbSlots(3);
```

All 8 workers can process tasks.

But only 3 workers may enter database section at once.

This is a common use case.

---

## 16. Does One Worker Mean One Thread?

Usually yes in a standard thread pool:

```text
one worker = one worker thread
```

That worker executes one task at a time.

However, a task itself could create more threads or use async/GPU work.

But that is separate.

In the normal mental model:

```text
3 workers = 3 threads = up to 3 CPU tasks concurrently
```

---

## 17. Worker Loop

Typical worker:

```cpp
void workerLoop() {
    while (true) {
        std::function<void()> task;

        {
            std::unique_lock<std::mutex> lock(mutex);

            cv.wait(lock, [&] {
                return stopped || !tasks.empty();
            });

            if (stopped && tasks.empty()) {
                return;
            }

            task = std::move(tasks.front());
            tasks.pop();
        }

        task();
    }
}
```

Important:

```text
queue is accessed under mutex
task executes outside mutex
```

Why?

If task executes while holding queue mutex, only one worker could effectively run.

---

## 18. Critical Thread Pool Detail

Bad:

```cpp
std::unique_lock<std::mutex> lock(mutex);
task = popTask();
task(); // still holding queue mutex
```

Then other workers cannot pop tasks.

The pool becomes nearly serial.

Correct:

```cpp
{
    std::unique_lock<std::mutex> lock(mutex);
    task = popTask();
}

task(); // mutex already released
```

This allows workers to execute different tasks concurrently.

---

## 19. submit()

A thread pool submit function usually:

```text
1. lock queue mutex
2. push task
3. unlock
4. notify one worker
```

Example:

```cpp
void submit(std::function<void()> task) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        tasks.push(std::move(task));
    }

    cv.notify_one();
}
```

One new task usually needs one worker, so `notify_one()` is enough.

---

## 20. Graceful Shutdown

Shutdown usually means:

```text
stop accepting new tasks
finish queued tasks
wake sleeping workers
join worker threads
```

Worker exit condition:

```cpp
if (stopped && tasks.empty()) {
    return;
}
```

This drains existing tasks before exit.

Shutdown:

```cpp
{
    std::lock_guard<std::mutex> lock(mutex);
    stopped = true;
}

cv.notify_all();

for (auto& worker : workers) {
    worker.join();
}
```

---

## 21. Immediate vs Graceful Shutdown

### Graceful shutdown

```text
finish queued tasks
then exit
```

Condition:

```cpp
stopped && tasks.empty()
```

### Immediate shutdown

```text
discard pending tasks
exit as soon as possible
```

This may require clearing queue.

The API must define which behavior it provides.

---

## 22. Backpressure

If producers submit tasks faster than workers process them, queue grows.

This can cause:

```text
high memory usage
long latency
overload
```

Solutions:

```text
bounded queue
reject tasks
block submitter
drop low-priority tasks
scale workers
```

A semaphore can implement queue admission limits.

Example:

```cpp
std::counting_semaphore<100> queueSlots(100);
```

Submit acquires a slot.

Worker releases slot after removing/completing task.

---

## 23. Semaphore vs Condition Variable

### condition_variable

Waits for an arbitrary predicate:

```cpp
!queue.empty()
stopped || ready
```

Requires:

```text
mutex + shared state + predicate
```

### semaphore

Waits for available permits:

```text
count > 0
```

The count is stored inside the semaphore.

Use semaphore when the condition is naturally a resource count.

Use condition variable when waiting for complex shared state.

---

## 24. Semaphore vs Atomic Counter

This is not equivalent:

```cpp
std::atomic<int> slots = 3;
```

A plain atomic counter does not automatically block threads when zero.

You would need to implement waiting/retry logic correctly.

Semaphore already provides:

```text
atomic permit accounting
blocking
wakeup
```

So use semaphore for resource permits.

---

## 25. CPU Concurrency vs Parallelism

### concurrency

Multiple tasks make progress during overlapping time.

### parallelism

Multiple tasks literally execute at the same time on different CPU cores.

With 3 workers:

```text
on 1 core:
    concurrency through scheduling

on 4+ cores:
    possibly true parallel execution
```

Thread count does not guarantee physical parallelism.

It depends on hardware and scheduler.

---

## 26. Too Many Workers

More threads do not always mean faster.

Too many workers can cause:

```text
context switching
cache misses
memory pressure
lock contention
oversubscription
```

For CPU-bound work, a common starting point is around hardware concurrency:

```cpp
std::thread::hardware_concurrency()
```

For I/O-bound workloads, more workers may make sense.

This is workload-dependent.

---

## 27. CPU-Bound vs I/O-Bound

### CPU-bound task

Examples:

```text
matrix computation
compression
parsing
encryption
```

Too many threads can hurt due to oversubscription.

### I/O-bound task

Examples:

```text
network wait
disk wait
database query
```

More threads may help hide waiting time.

But asynchronous I/O may scale better than one thread per request.

---

## 28. Thread Pool and Future

A thread pool submit function can return a future.

Conceptually:

```cpp
template <typename F>
auto submit(F&& f) -> std::future<ReturnType>;
```

Implementation often uses:

```cpp
std::packaged_task<ReturnType()>
```

The packaged task is placed into queue.

A worker executes it.

The caller receives the future.

---

## 29. Common Wrong Patterns

### Wrong: semaphore as vector protection

```cpp
semaphore<3> sem(3);
values.push_back(...);
```

Three threads may still modify vector simultaneously.

### Wrong: execute task under queue mutex

```cpp
lock queue mutex
pop task
run task while still locked
```

This serializes workers.

### Wrong: no shutdown wakeup

```cpp
stopped = true;
// forgot notify_all
```

Workers remain asleep.

### Wrong: unbounded task queue

Heavy load may grow queue indefinitely.

### Wrong: acquire without exception-safe release

```cpp
sem.acquire();
mayThrow();
sem.release();
```

Use RAII.

---

## 30. Common Interview Questions

### Q1. What is a worker in a thread pool?

A worker is usually one reusable thread running a loop that waits for tasks, removes one task from the queue, and executes it.

### Q2. Can three workers execute concurrently?

Yes. Normally three workers mean three threads, so up to three tasks may execute concurrently, depending on CPU resources and blocking.

### Q3. Difference between mutex and semaphore?

A mutex gives exclusive access to one owner at a time. A semaphore maintains a number of permits and allows up to N concurrent users.

### Q4. When should semaphore be used?

When access must be limited to a fixed number of resources, such as database connections, network requests, GPU slots, or concurrent expensive operations.

### Q5. Why execute tasks outside the queue mutex?

So other workers can acquire the queue lock, pop their own tasks, and execute concurrently.

### Q6. Does a semaphore make a data structure thread-safe?

No. It only limits concurrency. If multiple permitted threads modify shared state concurrently, a mutex or another synchronization design may still be required.

---

## 31. Key Takeaways

- One worker usually means one thread.
- Multiple workers can execute tasks concurrently.
- Thread pools reuse worker threads.
- Mutex allows one thread at a time.
- Semaphore allows up to N threads at a time.
- Semaphore controls permits, not arbitrary shared-state safety.
- Use condition variable for task queue waiting.
- Run tasks outside the queue mutex.
- Use notify_one for a newly submitted task.
- Use notify_all during shutdown.
- Use RAII around semaphore acquire/release.
- Bounded queues provide backpressure.
