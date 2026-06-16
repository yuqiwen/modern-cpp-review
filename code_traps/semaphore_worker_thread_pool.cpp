#include <chrono>
#include <condition_variable>
#include <functional>
#include <iostream>
#include <mutex>
#include <queue>
#include <semaphore>
#include <stdexcept>
#include <thread>
#include <utility>
#include <vector>
using namespace std;

/*
 * Topic:
 * semaphore, workers, and thread pool basics.
 *
 * Compile:
 * g++ -std=c++20 -pthread semaphore_worker_thread_pool.cpp \
 *     -o semaphore_worker_thread_pool
 *
 * Run:
 * ./semaphore_worker_thread_pool
 */

template <ptrdiff_t Max>
class SemaphoreGuard {
private:
    counting_semaphore<Max>& semaphore_;

public:
    explicit SemaphoreGuard(
        counting_semaphore<Max>& semaphore
    )
        : semaphore_(semaphore) {
        semaphore_.acquire();
    }

    ~SemaphoreGuard() {
        semaphore_.release();
    }

    SemaphoreGuard(const SemaphoreGuard&) = delete;
    SemaphoreGuard& operator=(const SemaphoreGuard&) = delete;
};

void semaphoreLimitDemo() {
    cout << "\n=== semaphore concurrency limit demo ===" << endl;

    counting_semaphore<2> slots(2);

    mutex outputMutex;
    int active = 0;

    auto work = [&](int id) {
        SemaphoreGuard guard(slots);

        {
            lock_guard<mutex> lock(outputMutex);
            ++active;

            cout << "worker " << id
                 << " entered, active = "
                 << active << endl;
        }

        this_thread::sleep_for(
            chrono::milliseconds(200)
        );

        {
            lock_guard<mutex> lock(outputMutex);

            cout << "worker " << id
                 << " leaving, active = "
                 << active << endl;

            --active;
        }
    };

    vector<thread> threads;

    for (int i = 0; i < 6; ++i) {
        threads.emplace_back(work, i);
    }

    for (thread& t : threads) {
        t.join();
    }

    cout << "At most 2 workers entered simultaneously.\n";
}

class ThreadPool {
private:
    vector<thread> workers_;
    queue<function<void()>> tasks_;

    mutex mutex_;
    condition_variable cv_;

    bool stopped_ = false;

    void workerLoop(size_t workerId) {
        while (true) {
            function<void()> task;

            {
                unique_lock<mutex> lock(mutex_);

                cv_.wait(lock, [&] {
                    return stopped_ || !tasks_.empty();
                });

                if (stopped_ && tasks_.empty()) {
                    cout << "worker "
                         << workerId
                         << " exiting\n";

                    return;
                }

                task = std::move(tasks_.front());
                tasks_.pop();
            }

            // Important:
            // task executes after queue mutex is released.
            task();
        }
    }

public:
    explicit ThreadPool(size_t workerCount) {
        workers_.reserve(workerCount);

        for (size_t i = 0; i < workerCount; ++i) {
            workers_.emplace_back(
                [this, i] {
                    workerLoop(i);
                }
            );
        }
    }

    ~ThreadPool() {
        stop();
    }

    void submit(function<void()> task) {
        {
            lock_guard<mutex> lock(mutex_);

            if (stopped_) {
                throw runtime_error(
                    "submit on stopped thread pool"
                );
            }

            tasks_.push(std::move(task));
        }

        cv_.notify_one();
    }

    void stop() {
        {
            lock_guard<mutex> lock(mutex_);

            if (stopped_) {
                return;
            }

            stopped_ = true;
        }

        cv_.notify_all();

        for (thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(const ThreadPool&) = delete;
};

void threadPoolDemo() {
    cout << "\n=== thread pool demo ===" << endl;

    ThreadPool pool(3);

    mutex outputMutex;

    for (int taskId = 0; taskId < 8; ++taskId) {
        pool.submit([taskId, &outputMutex] {
            {
                lock_guard<mutex> lock(outputMutex);

                cout << "task "
                     << taskId
                     << " started on thread "
                     << this_thread::get_id()
                     << endl;
            }

            this_thread::sleep_for(
                chrono::milliseconds(150)
            );

            {
                lock_guard<mutex> lock(outputMutex);

                cout << "task "
                     << taskId
                     << " finished\n";
            }
        });
    }

    pool.stop();

    cout << "All queued tasks completed.\n";
}

void workerPlusResourceLimitDemo() {
    cout << "\n=== workers plus resource semaphore demo ===" << endl;

    ThreadPool pool(5);

    // Five worker threads exist, but only two may use
    // the expensive resource at the same time.
    counting_semaphore<2> resourceSlots(2);

    mutex outputMutex;
    int activeResourceUsers = 0;

    for (int taskId = 0; taskId < 8; ++taskId) {
        pool.submit([
            taskId,
            &resourceSlots,
            &outputMutex,
            &activeResourceUsers
        ] {
            {
                lock_guard<mutex> lock(outputMutex);
                cout << "task "
                     << taskId
                     << " waiting for resource\n";
            }

            SemaphoreGuard guard(resourceSlots);

            {
                lock_guard<mutex> lock(outputMutex);

                ++activeResourceUsers;

                cout << "task "
                     << taskId
                     << " acquired resource, active = "
                     << activeResourceUsers
                     << endl;
            }

            this_thread::sleep_for(
                chrono::milliseconds(120)
            );

            {
                lock_guard<mutex> lock(outputMutex);

                cout << "task "
                     << taskId
                     << " releasing resource\n";

                --activeResourceUsers;
            }
        });
    }

    pool.stop();

    cout << "Five workers existed, but only two used "
            "the resource concurrently.\n";
}

void semaphoreDoesNotProtectVectorDemo() {
    cout << "\n=== semaphore is not a mutex demo ===" << endl;

    cout << "A semaphore with count 3 allows three "
            "threads into the region.\n";

    cout << "Therefore it cannot by itself protect "
            "std::vector::push_back from concurrent calls.\n";

    cout << "Use a mutex for exclusive vector mutation.\n";
}

int main() {
    semaphoreLimitDemo();

    threadPoolDemo();

    workerPlusResourceLimitDemo();

    semaphoreDoesNotProtectVectorDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}