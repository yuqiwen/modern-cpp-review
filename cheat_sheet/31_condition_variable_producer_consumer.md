# 31. condition_variable and Producer-Consumer Queue

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. The Problem: Busy Waiting](#2-the-problem-busy-waiting)
- [3. Basic Components](#3-basic-components)
- [4. Producer](#4-producer)
- [5. Consumer](#5-consumer)
- [6. Why unique_lock Instead of lock_guard?](#6-why-unique_lock-instead-of-lock_guard)
- [7. What wait Does](#7-what-wait-does)
- [8. Why Must wait Unlock the Mutex?](#8-why-must-wait-unlock-the-mutex)
- [9. Spurious Wakeup](#9-spurious-wakeup)
- [10. notify_one and notify_all](#10-notify_one-and-notify_all)
- [11. Notification Is Not Stored as a Task](#11-notification-is-not-stored-as-a-task)
- [12. Lost Wakeup](#12-lost-wakeup)
- [13. Why Protect Predicate State With the Same Mutex?](#13-why-protect-predicate-state-with-the-same-mutex)
- [14. Notify Before or After Unlock?](#14-notify-before-or-after-unlock)
- [15. Producer-Consumer Queue](#15-producer-consumer-queue)
- [16. Why pop Holds the Lock While Removing?](#16-why-pop-holds-the-lock-while-removing)
- [17. Shutdown Problem](#17-shutdown-problem)
- [18. Shutdown-Aware pop](#18-shutdown-aware-pop)
- [19. Draining Remaining Tasks](#19-draining-remaining-tasks)
- [20. Multiple Producers and Consumers](#20-multiple-producers-and-consumers)
- [21. Bounded Queue](#21-bounded-queue)
- [22. wait_for and wait_until](#22-wait_for-and-wait_until)
- [23. Common Wrong Patterns](#23-common-wrong-patterns)
- [24. condition_variable vs atomic](#24-condition_variable-vs-atomic)
- [25. Memory Visibility](#25-memory-visibility)
- [26. Common Interview Questions](#26-common-interview-questions)
- [27. Key Takeaways](#27-key-takeaways)

## Related Code Trap

- [condition_variable / producer-consumer Demo](../code_traps/condition_variable_producer_consumer.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

`std::condition_variable` lets one thread wait until another thread changes shared state.

Common use cases:

- producer-consumer queue
- thread pool task queue
- waiting for initialization
- waiting for work
- bounded buffer
- shutdown notification

Header:

```cpp
#include <condition_variable>
```

A condition variable is normally used together with:

```cpp
std::mutex
std::unique_lock<std::mutex>
shared state
predicate
```

---

## 2. The Problem: Busy Waiting

Bad:

```cpp
while (queue.empty()) {
    // keep checking
}
```

This is called:

```text
busy waiting
busy spinning
```

The thread repeatedly consumes CPU while doing no useful work.

Better:

```cpp
cv.wait(lock, [&] {
    return !queue.empty();
});
```

The waiting thread sleeps and is awakened when notified.

---

## 3. Basic Components

A typical producer-consumer queue has:

```cpp
std::queue<int> tasks;
std::mutex mutex;
std::condition_variable cv;
```

The queue is shared state.

The mutex protects the queue.

The condition variable lets consumers wait for:

```text
queue is not empty
```

---

## 4. Producer

```cpp
void produce(int value) {
    {
        std::lock_guard<std::mutex> lock(mutex);
        tasks.push(value);
    }

    cv.notify_one();
}
```

Steps:

```text
1. lock mutex
2. modify shared queue
3. unlock mutex
4. notify one waiting thread
```

The braces intentionally release the lock before notification.

---

## 5. Consumer

```cpp
int consume() {
    std::unique_lock<std::mutex> lock(mutex);

    cv.wait(lock, [&] {
        return !tasks.empty();
    });

    int value = tasks.front();
    tasks.pop();

    return value;
}
```

The consumer waits until the queue is not empty.

---

## 6. Why unique_lock Instead of lock_guard?

`condition_variable::wait` must temporarily unlock the mutex while the thread sleeps.

Conceptually:

```text
unlock mutex
sleep
wake up
lock mutex again
check condition
```

`std::lock_guard` cannot be manually unlocked and relocked.

`std::unique_lock` can.

Therefore:

```cpp
cv.wait(lock, predicate);
```

requires a `std::unique_lock<std::mutex>`.

---

## 7. What wait Does

This:

```cpp
cv.wait(lock, predicate);
```

is conceptually similar to:

```cpp
while (!predicate()) {
    cv.wait(lock);
}
```

The predicate is checked while the mutex is locked.

If false:

```text
wait atomically unlocks mutex and sleeps
```

When awakened:

```text
wait locks mutex again
then checks predicate again
```

When `wait` returns, the caller still owns the mutex.

---

## 8. Why Must wait Unlock the Mutex?

Suppose the consumer sleeps while still holding the mutex.

Then the producer tries:

```cpp
lock mutex
push data
```

But producer cannot acquire the mutex.

Consumer waits for data.

Producer waits for mutex.

That would deadlock.

So the waiting thread must release the mutex while asleep.

---

## 9. Spurious Wakeup

A waiting thread may wake up even if nobody provided the expected condition.

This is called:

```text
spurious wakeup
```

Therefore this is wrong:

```cpp
cv.wait(lock);

int value = tasks.front(); // queue might still be empty
```

Correct:

```cpp
cv.wait(lock, [&] {
    return !tasks.empty();
});
```

or manually:

```cpp
while (tasks.empty()) {
    cv.wait(lock);
}
```

Always wait in a loop or use the predicate overload.

---

## 10. notify_one and notify_all

### notify_one

```cpp
cv.notify_one();
```

Wakes one waiting thread.

Useful when one new task was added.

### notify_all

```cpp
cv.notify_all();
```

Wakes all waiting threads.

Useful for:

- shutdown
- global state transition
- many threads may now proceed

Waking all threads can be more expensive.

---

## 11. Notification Is Not Stored as a Task

A condition variable does not own the condition.

It does not store queue items.

It does not itself mean:

```text
one notification equals one task
```

The real truth is shared state:

```cpp
!tasks.empty()
```

Notification only tells waiting threads:

```text
something may have changed; check the predicate again
```

This is why the predicate is essential.

---

## 12. Lost Wakeup

A common concern:

```text
What if producer notifies before consumer starts waiting?
```

If the consumer uses shared state correctly, this is okay.

Example:

```cpp
tasks.push(value);
cv.notify_one();
```

Later consumer does:

```cpp
cv.wait(lock, [&] {
    return !tasks.empty();
});
```

The predicate is already true, so `wait` returns immediately without sleeping.

The notification itself may be lost, but the shared state is not lost.

Rule:

```text
The condition must be represented by shared state, not by notification alone.
```

---

## 13. Why Protect Predicate State With the Same Mutex?

The predicate usually examines shared state:

```cpp
!tasks.empty()
```

Both producer and consumer must access that state under the same mutex.

Otherwise there can be:

- data races
- missed state transitions
- inconsistent observations

Correct:

```cpp
{
    std::lock_guard<std::mutex> lock(mutex);
    tasks.push(value);
}
cv.notify_one();
```

and:

```cpp
std::unique_lock<std::mutex> lock(mutex);
cv.wait(lock, [&] {
    return !tasks.empty();
});
```

---

## 14. Notify Before or After Unlock?

Both can be correct, but notifying after unlocking is often preferred:

```cpp
{
    std::lock_guard<std::mutex> lock(mutex);
    tasks.push(value);
}

cv.notify_one();
```

Why?

If notified while mutex is still locked, the awakened thread may immediately block trying to acquire the same mutex.

Unlocking first may reduce unnecessary contention.

Correctness still depends on updating shared state before notification.

---

## 15. Producer-Consumer Queue

Basic design:

```cpp
class BlockingQueue {
private:
    std::queue<int> queue_;
    std::mutex mutex_;
    std::condition_variable cv_;

public:
    void push(int value) {
        {
            std::lock_guard<std::mutex> lock(mutex_);
            queue_.push(value);
        }

        cv_.notify_one();
    }

    int pop() {
        std::unique_lock<std::mutex> lock(mutex_);

        cv_.wait(lock, [&] {
            return !queue_.empty();
        });

        int value = queue_.front();
        queue_.pop();

        return value;
    }
};
```

---

## 16. Why pop Holds the Lock While Removing?

After `wait` returns, the queue is not empty while the consumer owns the mutex.

It must remove the item before releasing the mutex.

If it unlocked first:

```cpp
wait returns
unlock
read front
```

another consumer could remove the item first.

Therefore checking and consuming must be one synchronized operation.

---

## 17. Shutdown Problem

A blocking consumer may wait forever if no more work will arrive.

Add a shutdown flag:

```cpp
bool stopped = false;
```

Wait predicate:

```cpp
cv.wait(lock, [&] {
    return stopped || !queue.empty();
});
```

When shutting down:

```cpp
{
    std::lock_guard<std::mutex> lock(mutex);
    stopped = true;
}

cv.notify_all();
```

Every waiting consumer wakes up and checks `stopped`.

---

## 18. Shutdown-Aware pop

```cpp
std::optional<int> pop() {
    std::unique_lock<std::mutex> lock(mutex);

    cv.wait(lock, [&] {
        return stopped || !queue.empty();
    });

    if (queue.empty()) {
        return std::nullopt;
    }

    int value = queue.front();
    queue.pop();

    return value;
}
```

Interpretation:

```text
value returned -> got a task
nullopt returned -> queue stopped and no tasks remain
```

---

## 19. Draining Remaining Tasks

This predicate:

```cpp
stopped || !queue.empty()
```

allows a consumer to continue processing queued tasks after shutdown begins.

Only when:

```text
stopped == true
and queue.empty() == true
```

does `pop()` return `nullopt`.

This is called:

```text
graceful shutdown
draining the queue
```

---

## 20. Multiple Producers and Consumers

The same queue can support:

- multiple producer threads
- multiple consumer threads

As long as all queue operations use the same mutex.

Each push can call:

```cpp
notify_one()
```

Shutdown normally calls:

```cpp
notify_all()
```

because all consumers must be allowed to exit.

---

## 21. Bounded Queue

Sometimes the queue should have a maximum size.

Then producers also need to wait.

Conditions:

```text
consumer waits for not empty
producer waits for not full
```

Usually use two condition variables:

```cpp
std::condition_variable notEmpty;
std::condition_variable notFull;
```

Producer:

```cpp
notFull.wait(lock, [&] {
    return queue.size() < capacity;
});
```

Consumer:

```cpp
notEmpty.wait(lock, [&] {
    return !queue.empty();
});
```

After push:

```cpp
notEmpty.notify_one();
```

After pop:

```cpp
notFull.notify_one();
```

---

## 22. wait_for and wait_until

Timed waiting:

```cpp
cv.wait_for(lock, std::chrono::seconds(1), predicate);
```

Returns whether predicate became true before timeout.

Example:

```cpp
bool ready = cv.wait_for(
    lock,
    std::chrono::seconds(1),
    [&] { return !queue.empty(); }
);
```

Also:

```cpp
cv.wait_until(lock, deadline, predicate);
```

Use timed waits when a thread should not block forever.

---

## 23. Common Wrong Patterns

### Wrong: wait without predicate

```cpp
cv.wait(lock);
queue.front();
```

May fail due to spurious wakeup.

### Wrong: predicate state without mutex

```cpp
queue.push(value); // no lock
cv.notify_one();
```

Data race.

### Wrong: notification as the state

```text
Assume one notification permanently records one event
```

Condition variables do not work that way.

### Wrong: unlock before pop

```cpp
wait...
lock.unlock();
queue.pop();
```

Another consumer can race.

### Wrong: forgotten shutdown notification

```cpp
stopped = true;
// forgot notify_all()
```

Waiting threads may never wake.

---

## 24. condition_variable vs atomic

Use atomic for simple independent state:

```cpp
std::atomic<bool> stop;
std::atomic<int> counter;
```

Use condition variable when threads need to sleep until a condition is true:

```text
queue is non-empty
resource is available
initialization is complete
```

An atomic flag alone does not put a thread to sleep efficiently unless combined with newer atomic wait APIs.

---

## 25. Memory Visibility

The mutex does more than prevent simultaneous access.

It also establishes synchronization:

```text
producer writes shared data while holding mutex
consumer later locks the same mutex
consumer sees the synchronized writes
```

The mutex provides both:

- mutual exclusion
- memory ordering / visibility

---

## 26. Common Interview Questions

### Q1. Why does condition_variable use unique_lock?

Because `wait` must unlock the mutex while sleeping and lock it again after waking.

`unique_lock` supports unlock/relock; `lock_guard` does not.

### Q2. Why must wait use a predicate?

Because condition variables may have spurious wakeups, and notifications do not guarantee that the condition is still true.

### Q3. What does wait do to the mutex?

It atomically releases the mutex and sleeps. After waking, it reacquires the mutex before returning.

### Q4. What is the difference between notify_one and notify_all?

`notify_one` wakes one waiter. `notify_all` wakes all waiting threads.

### Q5. Can a notification be lost?

Yes, the notification itself is not stored. Correct code stores the condition in shared state, so a later waiter checks the state and does not need the old notification.

### Q6. Why update state before notify?

Because the awakened thread checks shared state. It must observe the new state associated with the notification.

---

## 27. Key Takeaways

- `condition_variable` lets threads sleep while waiting.
- The real condition lives in shared state.
- Protect shared state with a mutex.
- Use `unique_lock` with `wait`.
- `wait` unlocks while sleeping and relocks before returning.
- Always use a predicate or loop.
- Spurious wakeups are possible.
- `notify_one` wakes one waiter.
- `notify_all` wakes all waiters.
- Notification is not stored as work.
- Check and modify shared state while holding the mutex.
- Add a stop flag for graceful shutdown.
