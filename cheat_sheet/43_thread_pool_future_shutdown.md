# 43. Thread Pool, Task Submission, Futures, and Shutdown

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. Why Use a Thread Pool?](#2-why-use-a-thread-pool)
- [3. Basic Components](#3-basic-components)
- [4. Worker Loop](#4-worker-loop)
- [5. Why Execute Outside the Lock?](#5-why-execute-outside-the-lock)
- [6. Condition Variable Predicate](#6-condition-variable-predicate)
- [7. Simple Void Task Submission](#7-simple-void-task-submission)
- [8. Returning Results](#8-returning-results)
- [9. Exception Propagation](#9-exception-propagation)
- [10. Generic submit](#10-generic-submit)
- [11. packaged_task Is Move-Only](#11-packaged_task-is-move-only)
- [12. Binding Callable and Arguments](#12-binding-callable-and-arguments)
- [13. Future Behavior](#13-future-behavior)
- [14. Shutdown State](#14-shutdown-state)
- [15. Graceful Shutdown](#15-graceful-shutdown)
- [16. Immediate Shutdown](#16-immediate-shutdown)
- [17. Destructor Sequence](#17-destructor-sequence)
- [18. Why notify_all During Shutdown?](#18-why-notify_all-during-shutdown)
- [19. Do Not Join Under the Pool Lock](#19-do-not-join-under-the-pool-lock)
- [20. Number of Workers](#20-number-of-workers)
- [21. Queue Capacity](#21-queue-capacity)
- [22. Rejection Policies](#22-rejection-policies)
- [23. Caller-Runs Policy](#23-caller-runs-policy)
- [24. Nested Submission Deadlock](#24-nested-submission-deadlock)
- [25. Avoiding Nested Wait Deadlock](#25-avoiding-nested-wait-deadlock)
- [26. Lambda Capture Lifetime](#26-lambda-capture-lifetime)
- [27. Capturing this](#27-capturing-this)
- [28. Task Exceptions](#28-task-exceptions)
- [29. Long-Running Tasks](#29-long-running-tasks)
- [30. Shared Queue Contention](#30-shared-queue-contention)
- [31. Work Stealing](#31-work-stealing)
- [32. Thread Pool in AI Infrastructure](#32-thread-pool-in-ai-infrastructure)
- [33. Common Wrong Patterns](#33-common-wrong-patterns)
- [34. Practical Checklist](#34-practical-checklist)
- [35. Common Interview Questions](#35-common-interview-questions)
- [36. Key Takeaways](#36-key-takeaways)

## Related Code Trap

- Demo file: [thread_pool_future_shutdown.cpp](../code_traps/thread_pool_future_shutdown.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

A thread pool creates a fixed group of worker threads.

Submitted tasks are placed into a queue.

Workers repeatedly:

```text
wait for a task
remove one task
execute it
repeat
```

This avoids creating and destroying a new operating-system thread for every task.

---

## 2. Why Use a Thread Pool?

Creating one thread per task can cause:

```text
thread creation overhead
thread destruction overhead
large stack memory usage
excessive context switching
unbounded concurrency
scheduler pressure
```

A thread pool limits the number of concurrently executing tasks.

---

## 3. Basic Components

A simple thread pool usually contains:

```cpp
std::vector<std::thread> workers_;
std::queue<std::function<void()>> tasks_;

std::mutex mutex_;
std::condition_variable condition_;

bool stopping_ = false;
```

The mutex protects:

```text
task queue
shutdown state
```

---

## 4. Worker Loop

A basic worker loop:

```cpp
while (true) {
    std::function<void()> task;

    {
        std::unique_lock lock(mutex_);

        condition_.wait(lock, [&] {
            return stopping_ ||
                   !tasks_.empty();
        });

        if (stopping_ && tasks_.empty()) {
            return;
        }

        task = std::move(tasks_.front());
        tasks_.pop();
    }

    task();
}
```

The task executes after the queue lock has been released.

---

## 5. Why Execute Outside the Lock?

Executing a task while holding the queue mutex would:

```text
prevent other workers from taking tasks
prevent producers from submitting tasks
serialize most of the pool
increase deadlock risk
```

A task might itself submit another task to the same pool.

If the queue mutex were still held, that submission could deadlock.

Correct lock scope:

```text
under lock:
    inspect state
    remove one task

outside lock:
    execute task
```

---

## 6. Condition Variable Predicate

Workers wait using:

```cpp
condition_.wait(lock, [&] {
    return stopping_ ||
           !tasks_.empty();
});
```

The worker should wake when:

```text
a task is available
or shutdown begins
```

The predicate handles spurious wakeups.

---

## 7. Simple Void Task Submission

A basic API can accept:

```cpp
void submit(std::function<void()> task);
```

Implementation:

```cpp
void submit(std::function<void()> task) {
    {
        std::lock_guard lock(mutex_);

        if (stopping_) {
            throw std::runtime_error(
                "submit on stopped pool"
            );
        }

        tasks_.push(std::move(task));
    }

    condition_.notify_one();
}
```

One worker is awakened because one new task became available.

---

## 8. Returning Results

To return a value from an asynchronous task, use:

```cpp
std::packaged_task
std::future
```

Example:

```cpp
std::packaged_task<int()> task([] {
    return 42;
});

std::future<int> result = task.get_future();

task();

std::cout << result.get();
```

The packaged task stores its result in a shared state observed by the future.

---

## 9. Exception Propagation

If the callable throws:

```cpp
std::packaged_task<int()> task([]() -> int {
    throw std::runtime_error("failure");
});
```

the exception is captured in the shared state.

It is rethrown when:

```cpp
future.get();
```

is called.

This transfers task failure to the future consumer.

---

## 10. Generic submit

A generic submission interface:

```cpp
template <typename F, typename... Args>
auto submit(F&& function, Args&&... args);
```

The result type can be calculated using:

```cpp
std::invoke_result_t<F, Args...>
```

The callable and its arguments are bound into a zero-argument task.

---

## 11. packaged_task Is Move-Only

`std::packaged_task` is move-only.

A common queue type is:

```cpp
std::queue<std::function<void()>>
```

The traditional `std::function` model expects its stored callable to be copyable.

A common solution is:

```cpp
auto task =
    std::make_shared<std::packaged_task<Result()>>(
        ...
    );

tasks_.push([task] {
    (*task)();
});
```

The lambda captures a copyable shared pointer.

---

## 12. Binding Callable and Arguments

A task can be created with a lambda:

```cpp
auto task = std::make_shared<
    std::packaged_task<Result()>
>(
    [
        function = std::forward<F>(function),
        ... arguments =
            std::forward<Args>(args)
    ]() mutable -> Result {
        return std::invoke(
            std::move(function),
            std::move(arguments)...
        );
    }
);
```

For broad compiler compatibility, `std::bind` is also commonly used.

Care must be taken with references and object lifetimes.

---

## 13. Future Behavior

`future.get()`:

```text
waits if the result is not ready
returns the result when ready
rethrows the stored exception on failure
generally consumes the result
```

A normal `std::future` is move-only.

For multiple readers of the same result, use:

```cpp
std::shared_future
```

---

## 14. Shutdown State

A pool usually rejects submissions after shutdown begins.

The shutdown state and task insertion must be checked under the same mutex.

Correct:

```cpp
{
    std::lock_guard lock(mutex_);

    if (stopping_) {
        throw ...;
    }

    tasks_.push(...);
}
```

This prevents a check-then-act race between `submit` and shutdown.

---

## 15. Graceful Shutdown

Graceful shutdown means:

```text
reject new tasks
execute already queued tasks
exit workers when queue becomes empty
join every worker
```

Worker exit condition:

```cpp
if (stopping_ && tasks_.empty()) {
    return;
}
```

If stopping is true but tasks remain, the worker continues processing.

---

## 16. Immediate Shutdown

Immediate shutdown may:

```text
reject new tasks
discard queued tasks
wake workers
exit as soon as possible
```

Discarding packaged tasks causes their futures to observe a broken promise or another defined cancellation policy, depending on implementation.

The shutdown policy should be explicit.

---

## 17. Destructor Sequence

A graceful destructor:

```cpp
~ThreadPool() {
    {
        std::lock_guard lock(mutex_);
        stopping_ = true;
    }

    condition_.notify_all();

    for (std::thread& worker : workers_) {
        worker.join();
    }
}
```

The mutex must be released before joining.

---

## 18. Why notify_all During Shutdown?

All workers may be blocked in `wait`.

After `stopping_` becomes true, every worker must wake to:

```text
drain tasks
or observe empty queue and exit
```

Using only `notify_one` may leave other workers asleep indefinitely, causing joins to block.

---

## 19. Do Not Join Under the Pool Lock

Incorrect:

```cpp
std::lock_guard lock(mutex_);

stopping_ = true;

for (auto& worker : workers_) {
    worker.join();
}
```

Workers need the same mutex to wake, inspect state, and exit.

The destructor would wait for workers while preventing them from finishing.

---

## 20. Number of Workers

For CPU-bound work, a starting point is:

```cpp
std::thread::hardware_concurrency()
```

This is only a hint and may return zero.

CPU-bound pools often use a count near the number of available cores.

I/O-bound pools may use more threads because workers spend time blocked.

The correct number depends on:

```text
task type
blocking behavior
latency goals
CPU architecture
memory pressure
contention
```

Measure realistic workloads.

---

## 21. Queue Capacity

A fixed worker count does not bound pending work.

An unbounded task queue can still cause:

```text
unbounded memory growth
large queue latency
overload collapse
```

Production pools often use:

```text
bounded queue
submission timeout
rejection policy
load shedding
```

---

## 22. Rejection Policies

When a bounded pool is full, possible policies include:

```text
block submitter
reject immediately
wait with timeout
run task in caller thread
drop low-priority task
drop oldest task
```

The correct policy depends on task semantics.

---

## 23. Caller-Runs Policy

A caller-runs policy executes the task in the submitting thread when the queue is full.

This creates natural backpressure:

```text
the producer becomes busy doing the work
and therefore submits more slowly
```

However, it changes execution context and latency behavior.

---

## 24. Nested Submission Deadlock

Suppose a pool has one worker.

Task A runs inside the worker:

```cpp
auto future = pool.submit(taskB);
future.get();
```

Task B is queued, but no worker is available because the only worker is blocked waiting for B.

This deadlocks.

The same can happen in a larger pool if every worker waits on child tasks submitted to the same saturated pool.

This is called thread-pool starvation deadlock.

---

## 25. Avoiding Nested Wait Deadlock

Possible approaches:

```text
do not synchronously wait for same-pool child tasks
use continuations
use separate pools for dependent stages
allow worker-assisted execution
use work stealing
ensure sufficient independent worker capacity
```

Simply increasing thread count does not prove the problem is solved.

---

## 26. Lambda Capture Lifetime

Dangerous:

```cpp
std::future<void> future;

{
    int value = 42;

    future = pool.submit([&] {
        use(value);
    });
}
```

The task may run after `value` is destroyed.

Capture by value when appropriate:

```cpp
pool.submit([value] {
    use(value);
});
```

Reference captures require an externally guaranteed lifetime.

---

## 27. Capturing this

Dangerous:

```cpp
pool.submit([this] {
    useMember();
});
```

If the object is destroyed before the task executes, `this` dangles.

Possible solutions include:

```text
join or cancel tasks before destruction
capture shared ownership
capture required data by value
use weak_ptr and validate lifetime
```

---

## 28. Task Exceptions

Tasks returning futures can propagate exceptions through packaged tasks.

Fire-and-forget tasks require another policy:

```text
catch and log
terminate process
send failure to error queue
invoke error callback
```

Allowing an exception to escape a raw worker function may call `std::terminate`.

---

## 29. Long-Running Tasks

One long task occupies one worker for its entire duration.

If all workers execute long blocking operations:

```text
short tasks may wait behind them
latency becomes unpredictable
```

Possible designs:

```text
separate CPU and I/O pools
priority queues
task deadlines
cooperative yielding
asynchronous I/O
```

---

## 30. Shared Queue Contention

A simple pool uses one shared queue and one mutex.

At high throughput:

```text
all producers and workers contend for the same lock
```

More advanced pools may use:

```text
multiple queues
per-worker queues
sharding
lock-free queues
work stealing
```

The simple design is still valuable because it is easier to reason about and often sufficient.

---

## 31. Work Stealing

In a work-stealing pool:

```text
each worker owns a local deque
worker pushes and pops local tasks
idle workers steal tasks from other workers
```

Benefits may include:

```text
lower central-queue contention
better locality
automatic load balancing
```

Costs include substantially greater implementation complexity.

---

## 32. Thread Pool in AI Infrastructure

Thread pools commonly support:

```text
input parsing
tokenization
image decoding
data augmentation
CPU preprocessing
request postprocessing
network completion work
storage operations
```

Different workloads may require separate pools to avoid interference.

Example:

```text
CPU preprocessing pool
blocking I/O pool
GPU submission thread
response serialization pool
```

---

## 33. Common Wrong Patterns

### Execute task while holding queue mutex

The pool becomes serialized and may deadlock.

### Submit after shutdown

A future may never complete unless submission is rejected.

### Set stopping without notify_all

Sleeping workers may never exit.

### Join while holding mutex

Workers cannot acquire the mutex to exit.

### Unbounded queue

Pending memory and latency can grow without limit.

### Capture local references without lifetime guarantee

Tasks may access destroyed objects.

### Wait for same-pool child tasks

The pool may suffer starvation deadlock.

---

## 34. Practical Checklist

When designing a thread pool, ask:

```text
How many workers are required?

Are tasks CPU-bound or I/O-bound?

Is the queue bounded?

What happens when the queue is full?

Are submissions allowed after shutdown begins?

Is shutdown graceful or immediate?

How are task exceptions reported?

Can tasks submit other tasks?

Can tasks wait on same-pool futures?

What lifetimes do task captures rely on?

Are multiple workload classes interfering?
```

---

## 35. Common Interview Questions

### Q1. Why use a thread pool?

A thread pool reuses a bounded set of worker threads, reducing thread creation overhead and controlling execution concurrency.

### Q2. Why execute tasks outside the queue lock?

Otherwise one long task prevents all other workers and producers from accessing the queue and may cause reentrant deadlocks.

### Q3. How does submit return a value?

Wrap the callable in a `std::packaged_task`, obtain its `std::future`, enqueue a void wrapper that executes the packaged task, and return the future.

### Q4. How are task exceptions propagated?

`packaged_task` stores the exception in the future's shared state, and `future.get()` rethrows it.

### Q5. What is graceful thread-pool shutdown?

The pool stops accepting new work, executes already queued tasks, wakes all workers, and joins them after the queue drains.

### Q6. What is thread-pool starvation deadlock?

All workers block waiting for child tasks submitted to the same pool, leaving no available worker to execute those child tasks.

---

## 36. Key Takeaways

- A thread pool reuses a fixed number of worker threads.
- Workers wait on a task queue.
- Queue state and shutdown state require synchronization.
- Pop tasks under the lock and execute them after unlocking.
- `packaged_task` and `future` carry results and exceptions.
- Submission must atomically check shutdown state and enqueue.
- Graceful shutdown drains queued work.
- Shutdown requires `notify_all`.
- Never join workers while holding the queue mutex.
- Bound the task queue when overload matters.
- Be careful with nested submissions and captured lifetimes.
