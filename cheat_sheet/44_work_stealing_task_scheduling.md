# 44. Work Stealing and Task Scheduling

## Table of Contents

- [1. Core Idea](#1-core-idea)
- [2. Why a Shared Queue Can Become a Bottleneck](#2-why-a-shared-queue-can-become-a-bottleneck)
- [3. Local Worker Queues](#3-local-worker-queues)
- [4. Why Use a Deque?](#4-why-use-a-deque)
- [5. Owner LIFO Behavior](#5-owner-lifo-behavior)
- [6. Thief FIFO-Like Behavior](#6-thief-fifo-like-behavior)
- [7. Dynamic Load Balancing](#7-dynamic-load-balancing)
- [8. Local and Global Queues](#8-local-and-global-queues)
- [9. Worker Search Order](#9-worker-search-order)
- [10. Worker Identification](#10-worker-identification)
- [11. Work Stealing Is Not Necessarily Lock-Free](#11-work-stealing-is-not-necessarily-lock-free)
- [12. Victim Selection](#12-victim-selection)
- [13. Stealing One Task](#13-stealing-one-task)
- [14. Stealing Multiple Tasks](#14-stealing-multiple-tasks)
- [15. Divide-and-Conquer Workloads](#15-divide-and-conquer-workloads)
- [16. Fork-Join Work](#16-fork-join-work)
- [17. Helping](#17-helping)
- [18. Queue Synchronization](#18-queue-synchronization)
- [19. Reduced Contention](#19-reduced-contention)
- [20. Cache Locality](#20-cache-locality)
- [21. Task Completion Detection](#21-task-completion-detection)
- [22. RAII Completion Guard](#22-raii-completion-guard)
- [23. Shutdown Complexity](#23-shutdown-complexity)
- [24. External vs Internal Submission During Shutdown](#24-external-vs-internal-submission-during-shutdown)
- [25. Sleeping Workers](#25-sleeping-workers)
- [26. Notification](#26-notification)
- [27. Lost-Wakeup Considerations](#27-lost-wakeup-considerations)
- [28. Busy Waiting](#28-busy-waiting)
- [29. Yielding](#29-yielding)
- [30. Fairness](#30-fairness)
- [31. Priority Scheduling](#31-priority-scheduling)
- [32. NUMA and Topology](#32-numa-and-topology)
- [33. False Sharing in Scheduler Metadata](#33-false-sharing-in-scheduler-metadata)
- [34. Work Stealing in AI Infrastructure](#34-work-stealing-in-ai-infrastructure)
- [35. When Work Stealing Helps](#35-when-work-stealing-helps)
- [36. When a Shared Queue Is Better](#36-when-a-shared-queue-is-better)
- [37. Common Wrong Assumptions](#37-common-wrong-assumptions)
- [38. Practical Checklist](#38-practical-checklist)
- [39. Common Interview Questions](#39-common-interview-questions)
- [40. Key Takeaways](#40-key-takeaways)

## Related Code Trap

- Demo file: [work_stealing_scheduler.cpp](../code_traps/work_stealing_scheduler.cpp)
- Index: [Code Traps README](../code_traps/README.md)

## 1. Core Idea

A basic thread pool uses one shared task queue.

```text
producers -> shared queue -> all workers
```

This design is simple and often correct.

However, all producers and workers contend for:

```text
the same queue
the same queue mutex
the same head and tail metadata
```

A work-stealing scheduler instead gives each worker a local queue.

```text
worker 0 -> local deque 0
worker 1 -> local deque 1
worker 2 -> local deque 2
```

A worker processes local work first.

When it becomes idle, it steals work from another worker.

---

## 2. Why a Shared Queue Can Become a Bottleneck

With one shared queue, every operation may require the same mutex.

```cpp
std::mutex mutex;
std::queue<Task> tasks;
```

Submission:

```cpp
{
    std::lock_guard lock(mutex);
    tasks.push(std::move(task));
}
```

Worker pop:

```cpp
{
    std::lock_guard lock(mutex);
    task = std::move(tasks.front());
    tasks.pop();
}
```

Under many short tasks, workers may spend substantial time:

```text
waiting for the queue lock
invalidating shared cache lines
updating shared queue metadata
```

The scheduler itself becomes a bottleneck.

---

## 3. Local Worker Queues

A work-stealing pool commonly stores:

```cpp
struct WorkerQueue {
    std::mutex mutex;
    std::deque<Task> tasks;
};
```

Each worker has one queue.

The owner usually accesses its own queue frequently.

Other workers access it only when stealing.

This distributes contention across several queues.

---

## 4. Why Use a Deque?

A deque supports operations at both ends.

Typical strategy:

```text
owner pushes and pops at one end
thieves steal from the other end
```

Example:

```text
owner:
    push_front
    pop_front

thief:
    pop_back
```

This reduces direct contention between the owner and stealing threads.

---

## 5. Owner LIFO Behavior

The owner often processes the most recently generated task first.

This produces LIFO-like behavior.

Benefits:

```text
better cache locality
depth-first execution
recent parent-task data may remain hot
smaller active working set
```

This is useful in recursive divide-and-conquer workloads.

---

## 6. Thief FIFO-Like Behavior

A thief often steals from the opposite end.

It tends to receive older tasks.

Older tasks may represent larger independent branches of work.

This can improve:

```text
parallelism
fairness
load balancing
```

The combination is often described as:

```text
owner LIFO
thief FIFO
```

---

## 7. Dynamic Load Balancing

Static task assignment can become unbalanced when task durations differ.

Example:

```text
Task A: 1 ms
Task B: 1 ms
Task C: 1 ms
Task D: 1000 ms
```

Equal task counts do not imply equal work.

With work stealing, a worker that finishes early can take work from a busy worker.

This provides dynamic load balancing.

---

## 8. Local and Global Queues

Many schedulers use both:

```text
one global queue
one local queue per worker
```

External submissions usually enter the global queue.

Worker-generated child tasks may enter the current worker's local queue.

This preserves locality for nested tasks while still accepting work from non-worker threads.

---

## 9. Worker Search Order

A worker may search in this order:

```text
local queue
global queue
steal from other worker
wait
```

Conceptual loop:

```cpp
if (popLocal(task)) {
    run(task);
} else if (popGlobal(task)) {
    run(task);
} else if (steal(task)) {
    run(task);
} else {
    wait();
}
```

Different systems may choose different search priorities.

---

## 10. Worker Identification

A worker needs to know which local queue belongs to it.

A common approach is:

```cpp
thread_local int workerId = -1;
```

Worker startup:

```cpp
workerId = id;
```

External threads retain:

```text
workerId == -1
```

Submission can then choose:

```text
worker thread -> local queue
external thread -> global queue
```

Thread-local state must be managed carefully in long-lived worker threads.

---

## 11. Work Stealing Is Not Necessarily Lock-Free

Work stealing describes scheduling behavior.

It does not specify synchronization implementation.

A work-stealing deque may use:

```text
mutexes
spinlocks
atomic indices
lock-free algorithms
```

Therefore:

```text
work stealing != lock-free
```

They are independent concepts.

---

## 12. Victim Selection

An idle worker needs to choose another worker to steal from.

Possible strategies:

```text
round-robin
random selection
scan all workers
topology-aware selection
```

Random victim selection can reduce synchronized contention where many thieves target the same queue.

The thief should not choose itself.

---

## 13. Stealing One Task

The simplest policy steals one task.

Advantages:

```text
simple implementation
small lock hold time
victim retains most of its local work
```

Disadvantages:

```text
frequent stealing under severe imbalance
higher scheduling overhead
```

---

## 14. Stealing Multiple Tasks

Another policy steals several tasks or half the victim queue.

Advantages:

```text
faster load balancing
fewer steal operations
```

Disadvantages:

```text
more task movement
larger critical section
possible locality loss
possible over-stealing
```

The correct strategy depends on workload.

---

## 15. Divide-and-Conquer Workloads

Work stealing is especially useful for recursive algorithms.

Examples:

```text
parallel quicksort
parallel merge sort
tree traversal
graph exploration
recursive numerical algorithms
```

A task generates subproblems.

The owner continues depth-first work.

Idle workers steal other branches.

---

## 16. Fork-Join Work

Fork-join workloads have two phases.

Fork:

```text
create child tasks
```

Join:

```text
wait for child tasks to complete
```

Example:

```text
sort left half
sort right half
merge
```

A naive fixed thread pool can deadlock if all workers block waiting for child tasks submitted to the same pool.

Advanced schedulers may use helping or work stealing during waits.

---

## 17. Helping

Helping means a worker waiting for one task may execute other available tasks.

Conceptually:

```cpp
while (!targetReady()) {
    if (findTask(task)) {
        task();
    } else {
        std::this_thread::yield();
    }
}
```

Helping can prevent worker starvation.

However, it introduces complexity:

```text
reentrancy
nested execution
stack growth
exception handling
task ownership
cancellation
```

---

## 18. Queue Synchronization

A simple local queue can use a mutex.

```cpp
bool popLocal(std::size_t id, Task& task) {
    auto& queue = localQueues[id];

    std::lock_guard lock(queue.mutex);

    if (queue.tasks.empty()) {
        return false;
    }

    task = std::move(queue.tasks.front());
    queue.tasks.pop_front();

    return true;
}
```

Steal:

```cpp
bool stealFrom(std::size_t victim, Task& task) {
    auto& queue = localQueues[victim];

    std::lock_guard lock(queue.mutex);

    if (queue.tasks.empty()) {
        return false;
    }

    task = std::move(queue.tasks.back());
    queue.tasks.pop_back();

    return true;
}
```

This is mutex-based work stealing.

---

## 19. Reduced Contention

With local queues:

```text
worker 0 mostly locks queue 0
worker 1 mostly locks queue 1
worker 2 mostly locks queue 2
```

Only idle workers contend with a victim queue.

This often reduces contention compared with one global mutex.

---

## 20. Cache Locality

A worker often executes tasks it recently created.

The task's data may still be present in:

```text
registers
L1 cache
L2 cache
```

Local LIFO scheduling may preserve this locality.

Stealing usually occurs only when another worker would otherwise be idle.

---

## 21. Task Completion Detection

Empty queues do not necessarily mean all work is finished.

A worker may currently execute a task that will create more child tasks.

Completion often requires tracking:

```text
queued tasks
currently running tasks
potential future submissions
```

A common technique is an outstanding-task counter.

```cpp
std::atomic<std::size_t> outstanding;
```

On submit:

```cpp
outstanding.fetch_add(1);
```

On task completion:

```cpp
outstanding.fetch_sub(1);
```

When the counter reaches zero, all submitted work has completed.

The counter must be updated correctly on exceptions and rejected submissions.

---

## 22. RAII Completion Guard

A task must decrement the outstanding count even if it throws.

Conceptually:

```cpp
struct CompletionGuard {
    std::atomic<std::size_t>& counter;

    ~CompletionGuard() {
        counter.fetch_sub(1);
    }
};
```

Task wrapper:

```cpp
[task, &counter] {
    CompletionGuard guard{counter};
    task();
}
```

RAII prevents task-count leaks during exceptions.

---

## 23. Shutdown Complexity

A work-stealing pool cannot only inspect the global queue.

Local queues may still contain tasks.

Graceful shutdown may require:

```text
stop accepting external tasks
allow worker-generated tasks according to policy
drain global and local queues
wait for outstanding count to become zero
wake all workers
join workers
```

Shutdown semantics should be explicitly defined.

---

## 24. External vs Internal Submission During Shutdown

Possible policy:

```text
external submissions rejected after shutdown starts
internal child submissions allowed while draining
```

This can support recursive completion.

But it complicates the state machine.

Simpler pools may reject all submissions after shutdown begins.

The choice must match workload semantics.

---

## 25. Sleeping Workers

If no task is found, workers should not scan forever.

Possible strategy:

```text
spin briefly
yield
wait on condition variable
```

Spinning provides low latency but consumes CPU.

Sleeping reduces CPU use but adds wakeup latency.

Many runtimes combine both.

---

## 26. Notification

When a task is submitted:

```cpp
condition.notify_one();
```

An idle worker may wake and:

```text
check local queue
check global queue
steal work
```

During shutdown:

```cpp
condition.notify_all();
```

All workers must wake and reevaluate exit conditions.

---

## 27. Lost-Wakeup Considerations

The condition-variable predicate must correspond to shared state.

A notification by itself is not stored.

Workers should wait based on conditions such as:

```text
shutdown requested
global queue non-empty
outstanding task count non-zero
```

Checking every local queue in a predicate may be expensive.

Production schedulers often use additional atomic task counters or wakeup flags.

---

## 28. Busy Waiting

A worker can repeatedly attempt:

```text
pop local
pop global
steal
```

This minimizes wakeup latency.

But an idle pool may consume an entire CPU core per worker.

Busy waiting is appropriate only when latency requirements justify the CPU cost.

---

## 29. Yielding

```cpp
std::this_thread::yield();
```

suggests that the scheduler run another ready thread.

Yield does not guarantee sleep or fairness.

The same worker may be scheduled again immediately.

It is commonly used as one stage of a backoff strategy.

---

## 30. Fairness

Local LIFO scheduling may repeatedly prioritize newly created tasks.

Older local tasks could wait.

Stealing older tasks from the opposite end helps reduce starvation.

Strict fairness is usually not guaranteed.

Work stealing optimizes throughput and locality more than FIFO ordering.

---

## 31. Priority Scheduling

Work stealing becomes more complicated with task priorities.

A worker may need:

```text
one deque per priority
priority-aware stealing
deadline tracking
starvation prevention
```

A basic deque does not provide strict priority scheduling.

---

## 32. NUMA and Topology

On multi-socket systems, stealing from a nearby core may be cheaper than stealing across sockets.

Topology-aware schedulers may prefer:

```text
same core group
same NUMA node
same socket
remote socket only when necessary
```

This improves memory locality and reduces remote-access cost.

---

## 33. False Sharing in Scheduler Metadata

Per-worker fields may include:

```text
queue size
sleep state
task counter
random generator state
```

If adjacent workers frequently update fields in the same cache line, false sharing may occur.

Hot per-worker metadata may need padding or alignment.

---

## 34. Work Stealing in AI Infrastructure

Possible uses include:

```text
CPU preprocessing
tokenization
image decoding
data augmentation
graph optimization
model compilation
parallel file processing
distributed runtime scheduling
```

These workloads often have unpredictable task duration and recursive task generation.

---

## 35. When Work Stealing Helps

Work stealing is most useful when:

```text
there are many tasks
task durations are irregular
tasks generate child tasks
load imbalance is significant
central queue contention is measurable
```

---

## 36. When a Shared Queue Is Better

A single shared queue may be preferable when:

```text
the pool is small
tasks are long-running
submission rate is low
implementation simplicity matters
queue contention is not a bottleneck
strict FIFO behavior is desired
```

Use the simplest correct design that satisfies measured requirements.

---

## 37. Common Wrong Assumptions

### Wrong: work stealing is lock-free

It may still use mutexes.

### Wrong: empty global queue means no work

Local queues or running tasks may still exist.

### Wrong: equal task counts mean equal work

Task durations may differ greatly.

### Wrong: more stealing is always better

Stealing has synchronization and locality costs.

### Wrong: no task found means safe to exit

Another running task may soon create child work.

---

## 38. Practical Checklist

When designing a work-stealing scheduler, ask:

```text
Does each worker have a local queue?

Which end does the owner use?

Which end does the thief use?

Where do external submissions go?

How is a victim selected?

How many tasks are stolen?

How is total outstanding work tracked?

What happens during nested waits?

What are the shutdown semantics?

When do idle workers spin, yield, or sleep?

Is false sharing present in per-worker metadata?
```

---

## 39. Common Interview Questions

### Q1. What is work stealing?

Work stealing is a scheduling strategy where each worker maintains local tasks and idle workers take tasks from busy workers.

### Q2. Why use a deque?

The owner and thieves can operate on opposite ends, reducing contention and enabling owner-LIFO and thief-FIFO behavior.

### Q3. Why does the owner often use LIFO?

Recently created tasks may share data with the current task, so LIFO improves cache locality and depth-first execution.

### Q4. Is work stealing lock-free?

Not necessarily. Work stealing is a scheduling strategy and may be implemented with mutexes or lock-free data structures.

### Q5. Why is shutdown harder?

Tasks may exist in local queues or be generated by currently running tasks, so an empty global queue does not mean the system is idle.

### Q6. When is work stealing useful?

It is useful for many short, irregular, dynamically generated tasks where load imbalance or central queue contention is significant.

---

## 40. Key Takeaways

- A shared queue is simple but may become contended.
- Work stealing gives each worker a local deque.
- Workers process local tasks before stealing.
- Owners and thieves usually access opposite deque ends.
- Owner-LIFO improves locality.
- Thief-FIFO helps parallelism and fairness.
- External tasks often enter a global queue.
- Worker-generated tasks often enter local queues.
- Work stealing is not inherently lock-free.
- Completion detection requires more than checking queue emptiness.
- Shutdown must account for local queues and running tasks.
- Use work stealing only when workload complexity justifies it.
