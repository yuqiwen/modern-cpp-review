# 42. Bounded Blocking Queue, Backpressure, and Graceful Shutdown

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. Why an Unbounded Queue Is Dangerous](#2-why-an-unbounded-queue-is-dangerous)
- [3. Backpressure](#3-backpressure)
- [4. Queue State](#4-queue-state)
- [5. Two Condition Variables](#5-two-condition-variables)
- [6. Blocking Push](#6-blocking-push)
- [7. Blocking Pop](#7-blocking-pop)
- [8. Why Use Predicates?](#8-why-use-predicates)
- [9. Wait Releases the Mutex](#9-wait-releases-the-mutex)
- [10. Push Notification](#10-push-notification)
- [11. Pop Notification](#11-pop-notification)
- [12. Closing the Queue](#12-closing-the-queue)
- [13. Why close Uses notify_all](#13-why-close-uses-notify_all)
- [14. Graceful Shutdown](#14-graceful-shutdown)
- [15. Immediate Shutdown](#15-immediate-shutdown)
- [16. Idempotent Close](#16-idempotent-close)
- [17. Push After Close](#17-push-after-close)
- [18. Pop After Close](#18-pop-after-close)
- [19. Notification Outside the Lock](#19-notification-outside-the-lock)
- [20. Lost Wakeups](#20-lost-wakeups)
- [21. Blocking Backpressure](#21-blocking-backpressure)
- [22. Rejection Backpressure](#22-rejection-backpressure)
- [23. Timed Push](#23-timed-push)
- [24. Drop Policies](#24-drop-policies)
- [25. Queue Capacity and Latency](#25-queue-capacity-and-latency)
- [26. Little's Law Intuition](#26-littles-law-intuition)
- [27. Worker Pool Shutdown](#27-worker-pool-shutdown)
- [28. Join Outside the Queue Lock](#28-join-outside-the-queue-lock)
- [29. Exceptions in Tasks](#29-exceptions-in-tasks)
- [30. Move-Only Tasks](#30-move-only-tasks)
- [31. Semaphore-Based Bounded Queue](#31-semaphore-based-bounded-queue)
- [32. Condition Variables vs Semaphores](#32-condition-variables-vs-semaphores)
- [33. AI Serving Application](#33-ai-serving-application)
- [34. Dynamic Batching](#34-dynamic-batching)
- [35. Common Wrong Patterns](#35-common-wrong-patterns)
- [36. Practical Checklist](#36-practical-checklist)
- [37. Common Interview Questions](#37-common-interview-questions)
- [38. Key Takeaways](#38-key-takeaways)

## Related Code Trap

- Demo file: [bounded_blocking_queue.cpp](../code_traps/bounded_blocking_queue.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

A bounded blocking queue has a fixed maximum capacity.

It coordinates:

```text
producers that insert tasks
consumers that remove tasks
```

When the queue is empty:

```text
consumers wait
```

When the queue is full:

```text
producers wait or apply another overload policy
```

A bounded queue prevents unlimited memory growth and provides backpressure.

---

## 2. Why an Unbounded Queue Is Dangerous

Suppose producers create tasks faster than consumers process them.

```text
production rate: 1000 tasks/second
consumption rate: 500 tasks/second
```

The backlog grows by approximately:

```text
500 tasks/second
```

An unbounded queue can lead to:

```text
increasing memory consumption
increasing latency
out-of-memory termination
poor overload behavior
```

The queue hides overload temporarily instead of controlling it.

---

## 3. Backpressure

Backpressure means that downstream saturation affects upstream behavior.

When a bounded queue is full, the producer may:

```text
block
fail immediately
wait with a timeout
drop a task
replace an older task
reduce production rate
```

The important property is:

```text
the system does not accept unlimited work without accounting for capacity
```

---

## 4. Queue State

A bounded blocking queue commonly stores:

```cpp
std::queue<T> queue_;
std::size_t capacity_;
bool closed_ = false;

std::mutex mutex_;
std::condition_variable notEmpty_;
std::condition_variable notFull_;
```

Important invariant:

```text
0 <= queue_.size() <= capacity_
```

---

## 5. Two Condition Variables

### notEmpty

Consumers wait when the queue is empty.

```cpp
notEmpty_.wait(lock, [&] {
    return closed_ || !queue_.empty();
});
```

### notFull

Producers wait when the queue is full.

```cpp
notFull_.wait(lock, [&] {
    return closed_ ||
           queue_.size() < capacity_;
});
```

Separate condition variables make the intended wakeup conditions clear.

---

## 6. Blocking Push

A blocking push operation:

```cpp
bool push(T value) {
    std::unique_lock lock(mutex_);

    notFull_.wait(lock, [&] {
        return closed_ ||
               queue_.size() < capacity_;
    });

    if (closed_) {
        return false;
    }

    queue_.push(std::move(value));

    lock.unlock();
    notEmpty_.notify_one();

    return true;
}
```

Execution:

```text
acquire mutex
wait until space exists or queue closes
reject if closed
insert item
unlock
notify one consumer
```

---

## 7. Blocking Pop

A blocking pop operation:

```cpp
bool pop(T& output) {
    std::unique_lock lock(mutex_);

    notEmpty_.wait(lock, [&] {
        return closed_ ||
               !queue_.empty();
    });

    if (queue_.empty()) {
        return false;
    }

    output = std::move(queue_.front());
    queue_.pop();

    lock.unlock();
    notFull_.notify_one();

    return true;
}
```

If the queue is closed but still contains items, consumers continue draining it.

If the queue is both closed and empty, `pop` returns false.

---

## 8. Why Use Predicates?

Condition variables can wake spuriously.

Incorrect:

```cpp
notEmpty_.wait(lock);

output = queue_.front();
```

The queue may still be empty.

Correct:

```cpp
notEmpty_.wait(lock, [&] {
    return closed_ ||
           !queue_.empty();
});
```

The predicate is always rechecked while holding the mutex.

---

## 9. Wait Releases the Mutex

When a thread calls:

```cpp
condition.wait(lock, predicate);
```

and the predicate is false:

```text
the mutex is atomically released
the thread blocks
the thread later wakes
the mutex is reacquired
the predicate is checked again
```

The consumer must release the mutex while waiting so producers can insert.

The producer must release the mutex while waiting so consumers can remove.

---

## 10. Push Notification

After a producer inserts an item:

```cpp
notEmpty_.notify_one();
```

One waiting consumer may now make progress.

The queue changed from possibly empty to non-empty.

---

## 11. Pop Notification

After a consumer removes an item:

```cpp
notFull_.notify_one();
```

One waiting producer may now make progress.

The queue changed from possibly full to having available capacity.

---

## 12. Closing the Queue

A queue needs an explicit shutdown state.

```cpp
void close() {
    {
        std::lock_guard lock(mutex_);

        if (closed_) {
            return;
        }

        closed_ = true;
    }

    notEmpty_.notify_all();
    notFull_.notify_all();
}
```

Closing means:

```text
no new items are accepted
waiting threads are awakened
existing items may still be consumed
```

---

## 13. Why close Uses notify_all

At shutdown, several threads may be waiting:

```text
multiple consumers on notEmpty
multiple producers on notFull
```

If only one thread is notified, the others may remain asleep forever because no future queue operations may occur.

Therefore shutdown normally uses:

```cpp
notify_all();
```

---

## 14. Graceful Shutdown

A graceful drain policy means:

```text
reject new pushes after close
continue popping queued items
exit consumers when closed and empty
```

State progression:

```text
open + non-empty
closed + non-empty
closed + empty
```

Consumers stop only in the final state.

---

## 15. Immediate Shutdown

Some systems need immediate cancellation.

That policy may:

```text
close the queue
discard pending items
wake all workers
exit without draining
```

This is different from graceful shutdown.

The API should make the policy explicit.

---

## 16. Idempotent Close

`close()` should usually be safe to call more than once.

```cpp
if (closed_) {
    return;
}
```

This helps with:

```text
normal shutdown
exception paths
destructors
multiple owners requesting termination
```

---

## 17. Push After Close

A push after close should not block forever.

Possible API choices:

```text
return false
throw an exception
return a status enum
```

A boolean result is often simple:

```cpp
if (!queue.push(task)) {
    // queue closed
}
```

---

## 18. Pop After Close

A pop operation should distinguish:

```text
closed but data remains
closed and empty
```

Typical behavior:

```text
closed + non-empty -> return next item
closed + empty     -> return false
```

This enables graceful draining.

---

## 19. Notification Outside the Lock

Common pattern:

```cpp
{
    std::lock_guard lock(mutex_);
    queue_.push(...);
}

notEmpty_.notify_one();
```

The shared state is modified under the lock.

The lock is then released before notification.

This avoids waking a thread only for it to immediately block on the same mutex.

Notifying while holding the lock can still be correct, but is often less efficient.

---

## 20. Lost Wakeups

Condition variables do not store notifications as queue entries.

Correctness must rely on:

```text
shared state protected by mutex
predicate checked under mutex
```

A notification only informs waiters that state may have changed.

The predicate is the source of truth.

---

## 21. Blocking Backpressure

Blocking push is appropriate when:

```text
tasks must not be lost
producer is allowed to wait
bounded memory is required
```

Risks:

```text
producer threads may all block
upstream latency propagates
deadlock is possible if producer also owns resources consumers need
```

Blocking behavior must be designed carefully.

---

## 22. Rejection Backpressure

A nonblocking push may return immediately:

```cpp
bool tryPush(T value);
```

If full:

```text
return false
```

The caller may:

```text
retry
return an overload error
shed load
route elsewhere
```

This is common in latency-sensitive services.

---

## 23. Timed Push

A timed push waits only for a bounded duration.

```cpp
bool pushFor(
    T value,
    std::chrono::milliseconds timeout
);
```

Possible result:

```text
success
timeout
closed
```

A status enum may be more expressive than bool.

---

## 24. Drop Policies

Overload strategies may include:

### Drop newest

Reject the arriving item.

### Drop oldest

Remove the oldest queued item and insert the new one.

### Keep latest

Useful for sensor data, video frames, or monitoring updates where stale data has little value.

### Priority shedding

Drop low-priority work first.

The correct policy depends on application semantics.

---

## 25. Queue Capacity and Latency

A large queue can absorb bursts, but it also permits large waiting times.

If each task takes 10 milliseconds and 1000 tasks are queued, a new task may wait a long time.

A queue capacity therefore controls both:

```text
memory
maximum backlog latency
```

An oversized queue can convert overload into unacceptable latency.

---

## 26. Little's Law Intuition

A common queueing relationship is:

```text
average items in system
≈ arrival rate × average time in system
```

For example:

```text
100 requests/second
2 seconds average system time
```

implies approximately:

```text
200 requests in the system
```

This illustrates the relationship between throughput, concurrency, and latency.

---

## 27. Worker Pool Shutdown

A graceful worker-pool shutdown often follows:

```text
stop accepting submissions
close the task queue
wake blocked producers and consumers
allow workers to drain remaining tasks
workers exit when pop returns false
join all worker threads
```

Do not destroy the queue before workers stop accessing it.

---

## 28. Join Outside the Queue Lock

Dangerous:

```cpp
std::lock_guard lock(queueMutex);
worker.join();
```

The worker may need `queueMutex` to finish.

Safer:

```text
set closed state under lock
release lock
notify workers
join workers
```

---

## 29. Exceptions in Tasks

If a worker executes arbitrary tasks:

```cpp
task();
```

an uncaught exception may terminate the worker thread or process.

Worker loops should define an exception policy:

```cpp
try {
    task();
} catch (...) {
    // log, record failure, or propagate through promise
}
```

Queue correctness alone does not solve task failure handling.

---

## 30. Move-Only Tasks

Tasks may be move-only:

```cpp
std::packaged_task<void()>
std::unique_ptr<Job>
```

The queue API should support moves:

```cpp
bool push(T value);
```

and:

```cpp
output = std::move(queue_.front());
```

Avoid unnecessary copies.

---

## 31. Semaphore-Based Bounded Queue

A bounded queue can also use semaphores:

```text
availableSlots initialized to capacity
availableItems initialized to zero
```

Producer:

```text
acquire slot
lock queue
push
unlock queue
release item
```

Consumer:

```text
acquire item
lock queue
pop
unlock queue
release slot
```

The mutex still protects the underlying container.

Semaphores represent counts; they do not replace container mutual exclusion.

---

## 32. Condition Variables vs Semaphores

Condition-variable design:

```text
state stored in queue size and closed flag
waiters check predicates
```

Semaphore design:

```text
permit counts represent available items and slots
```

Condition variables are often easier when shutdown and multiple predicates are involved.

Semaphores can express resource counts naturally.

---

## 33. AI Serving Application

In an inference server:

```text
incoming request
request queue
batcher
GPU execution
response
```

A bounded request queue can prevent:

```text
unlimited request accumulation
unbounded memory use
extreme tail latency
```

When full, the service may return:

```text
overloaded
retry later
deadline exceeded
```

Rejecting early can be better than accepting work that cannot complete before its deadline.

---

## 34. Dynamic Batching

A batching worker may wait until:

```text
batch reaches maximum size
or timeout expires
```

Important controls include:

```text
maximum queue size
maximum batch size
maximum batching delay
request deadline
```

A request that has already expired should not necessarily remain in the batch queue.

---

## 35. Common Wrong Patterns

### Unbounded queue under overload

```text
memory and latency grow indefinitely
```

### Wait without predicate

```cpp
condition.wait(lock);
```

May continue after spurious wakeup.

### Close without notification

Waiting threads may never wake.

### notify_one during global shutdown

Other waiters may remain blocked forever.

### Push succeeds after close

Shutdown state becomes inconsistent.

### Join while holding queue mutex

Workers may need the mutex to exit.

### Destroy queue before workers stop

Workers may access destroyed state.

---

## 36. Practical Checklist

When designing a work queue, ask:

```text
What is the maximum capacity?

What happens when the queue is full?

Can tasks be dropped?

Can producers block?

Is there a timeout?

What does close mean?

Are queued tasks drained during shutdown?

How are waiting producers awakened?

How are waiting consumers awakened?

How are task exceptions handled?

What is the maximum acceptable queueing latency?
```

---

## 37. Common Interview Questions

### Q1. What is backpressure?

Backpressure is a mechanism that propagates downstream saturation to upstream producers so the system does not accept unlimited work.

### Q2. Why use a bounded queue?

A bounded queue limits memory growth, controls backlog, and makes overload behavior explicit.

### Q3. Why are two condition variables useful?

Consumers wait for `notEmpty`, while producers wait for `notFull`. The conditions represent different state transitions.

### Q4. Why must close notify all waiters?

After shutdown there may be no more queue operations to generate notifications, so every blocked producer and consumer must be awakened to observe the closed state.

### Q5. What is graceful shutdown?

Graceful shutdown stops accepting new work but allows already queued work to finish before workers exit.

### Q6. Why does the predicate include closed?

A thread waiting on an empty or full queue must also wake and exit when shutdown is requested.

---

## 38. Key Takeaways

- Bounded queues prevent unlimited backlog growth.
- Backpressure communicates downstream saturation upstream.
- Consumers wait for non-empty state.
- Producers wait for non-full state.
- Condition-variable predicates must include shutdown state.
- Close should reject new pushes.
- Graceful shutdown drains existing items.
- Shutdown should notify all waiting threads.
- Queue capacity affects both memory and latency.
- Join workers only after releasing queue locks.
- Overload policy must be explicit.
