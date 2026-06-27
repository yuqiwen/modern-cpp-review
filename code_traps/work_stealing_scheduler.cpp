#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <deque>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <random>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>

using namespace std;

/*
 * Topic:
 * Work stealing and task scheduling.
 *
 * This is an educational mutex-based scheduler.
 * It is not a production lock-free work-stealing deque.
 *
 * Compile:
 * g++ -std=c++20 -O2 -pthread \
 *     work_stealing_scheduler.cpp \
 *     -o work_stealing_scheduler
 */

class WorkStealingPool {
private:
    using Task = function<void()>;

    struct WorkerQueue {
        mutex queueMutex;
        deque<Task> tasks;
    };

    vector<thread> workers_;
    vector<unique_ptr<WorkerQueue>> localQueues_;

    queue<Task> globalQueue_;
    mutex globalMutex_;

    condition_variable condition_;
    mutex waitMutex_;

    atomic<bool> stopping_{false};
    atomic<size_t> outstandingTasks_{0};

    static thread_local int currentWorkerId_;

    bool popLocal(size_t workerId, Task& task) {
        WorkerQueue& local =
            *localQueues_[workerId];

        lock_guard lock(local.queueMutex);

        if (local.tasks.empty()) {
            return false;
        }

        task = move(local.tasks.front());
        local.tasks.pop_front();

        return true;
    }

    bool popGlobal(Task& task) {
        lock_guard lock(globalMutex_);

        if (globalQueue_.empty()) {
            return false;
        }

        task = move(globalQueue_.front());
        globalQueue_.pop();

        return true;
    }

    bool stealTask(size_t thiefId, Task& task) {
        const size_t count = localQueues_.size();

        if (count <= 1) {
            return false;
        }

        static thread_local mt19937 generator{
            random_device{}()
        };

        uniform_int_distribution<size_t> distribution(
            0,
            count - 1
        );

        for (size_t attempt = 0;
             attempt < count;
             ++attempt) {
            size_t victimId = distribution(generator);

            if (victimId == thiefId) {
                continue;
            }

            WorkerQueue& victim =
                *localQueues_[victimId];

            unique_lock lock(
                victim.queueMutex,
                try_to_lock
            );

            if (
                !lock.owns_lock() ||
                victim.tasks.empty()
            ) {
                continue;
            }

            // Owner pops front.
            // Thief steals back.
            task = move(victim.tasks.back());
            victim.tasks.pop_back();

            return true;
        }

        return false;
    }

    bool findTask(size_t workerId, Task& task) {
        return popLocal(workerId, task) ||
               popGlobal(task) ||
               stealTask(workerId, task);
    }

    void runTask(Task& task) {
        struct CompletionGuard {
            atomic<size_t>& counter;

            ~CompletionGuard() {
                counter.fetch_sub(
                    1,
                    memory_order_release
                );
            }
        };

        CompletionGuard guard{
            outstandingTasks_
        };

        try {
            task();
        } catch (const exception& error) {
            cerr << "task exception: "
                 << error.what()
                 << '\n';
        } catch (...) {
            cerr << "unknown task exception\n";
        }
    }

    void workerLoop(size_t workerId) {
        currentWorkerId_ =
            static_cast<int>(workerId);

        while (true) {
            Task task;

            if (findTask(workerId, task)) {
                runTask(task);
                continue;
            }

            if (
                stopping_.load(
                    memory_order_acquire
                ) &&
                outstandingTasks_.load(
                    memory_order_acquire
                ) == 0
            ) {
                break;
            }

            unique_lock lock(waitMutex_);

            condition_.wait_for(
                lock,
                chrono::milliseconds(2),
                [&] {
                    return stopping_.load(
                               memory_order_acquire
                           ) ||
                           outstandingTasks_.load(
                               memory_order_acquire
                           ) > 0;
                }
            );
        }

        currentWorkerId_ = -1;

        cout << "worker "
             << workerId
             << " exiting\n";
    }

public:
    explicit WorkStealingPool(size_t workerCount) {
        if (workerCount == 0) {
            throw invalid_argument(
                "worker count must be positive"
            );
        }

        localQueues_.reserve(workerCount);

        for (size_t i = 0;
             i < workerCount;
             ++i) {
            localQueues_.push_back(
                make_unique<WorkerQueue>()
            );
        }

        workers_.reserve(workerCount);

        for (size_t id = 0;
             id < workerCount;
             ++id) {
            workers_.emplace_back(
                [this, id] {
                    workerLoop(id);
                }
            );
        }
    }

    WorkStealingPool(
        const WorkStealingPool&
    ) = delete;

    WorkStealingPool& operator=(
        const WorkStealingPool&
    ) = delete;

    ~WorkStealingPool() {
        shutdown();
    }

    template <typename F>
    bool submit(F&& function) {
        if (
            stopping_.load(
                memory_order_acquire
            )
        ) {
            return false;
        }

        Task task(
            forward<F>(function)
        );

        outstandingTasks_.fetch_add(
            1,
            memory_order_release
        );

        // Worker-generated task:
        // push into its own local queue.
        if (currentWorkerId_ >= 0) {
            size_t workerId =
                static_cast<size_t>(
                    currentWorkerId_
                );

            {
                lock_guard lock(
                    localQueues_[workerId]
                        ->queueMutex
                );

                localQueues_[workerId]
                    ->tasks
                    .push_front(
                        move(task)
                    );
            }
        } else {
            // External submission:
            // push into global queue.
            {
                lock_guard lock(globalMutex_);

                globalQueue_.push(
                    move(task)
                );
            }
        }

        condition_.notify_one();

        return true;
    }

    void waitUntilIdle() {
        while (
            outstandingTasks_.load(
                memory_order_acquire
            ) != 0
        ) {
            this_thread::sleep_for(
                chrono::milliseconds(1)
            );
        }
    }

    void shutdown() {
        bool expected = false;

        if (!stopping_.compare_exchange_strong(
                expected,
                true,
                memory_order_acq_rel
            )) {
            return;
        }

        condition_.notify_all();

        for (thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }
};

thread_local int
    WorkStealingPool::currentWorkerId_ = -1;

void basicDemo() {
    cout << "\n=== basic work stealing demo ===\n";

    WorkStealingPool pool(4);

    for (int i = 0; i < 12; ++i) {
        pool.submit([i] {
            cout << "task "
                 << i
                 << " executed by thread "
                 << this_thread::get_id()
                 << '\n';

            this_thread::sleep_for(
                chrono::milliseconds(
                    20 + (i % 4) * 15
                )
            );
        });
    }

    pool.waitUntilIdle();
}

void nestedTaskDemo() {
    cout << "\n=== nested task demo ===\n";

    WorkStealingPool pool(4);

    pool.submit([&pool] {
        cout << "parent task on "
             << this_thread::get_id()
             << '\n';

        for (int child = 0;
             child < 8;
             ++child) {
            pool.submit([child] {
                cout << "child "
                     << child
                     << " on "
                     << this_thread::get_id()
                     << '\n';

                this_thread::sleep_for(
                    chrono::milliseconds(30)
                );
            });
        }
    });

    pool.waitUntilIdle();
}

void imbalanceDemo() {
    cout << "\n=== irregular workload demo ===\n";

    WorkStealingPool pool(3);

    for (int i = 0; i < 9; ++i) {
        pool.submit([i] {
            int duration =
                (i == 0)
                    ? 300
                    : 30;

            cout << "task "
                 << i
                 << " duration "
                 << duration
                 << " ms\n";

            this_thread::sleep_for(
                chrono::milliseconds(
                    duration
                )
            );
        });
    }

    pool.waitUntilIdle();
}

void warningDemo() {
    cout << "\n=== design warnings ===\n";

    cout << "This implementation uses mutexes.\n";
    cout << "Work stealing does not imply lock-free.\n";

    cout << "Production schedulers must handle:\n";
    cout << "- stronger shutdown semantics\n";
    cout << "- efficient waiting\n";
    cout << "- task futures\n";
    cout << "- cancellation\n";
    cout << "- nested joins\n";
    cout << "- NUMA locality\n";
    cout << "- false sharing\n";
}

int main() {
    basicDemo();
    nestedTaskDemo();
    imbalanceDemo();
    warningDemo();

    cout << "\n=== end of main ===\n";

    return 0;
}