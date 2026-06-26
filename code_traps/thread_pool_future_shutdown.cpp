#include <condition_variable>
#include <cstddef>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

using namespace std;

/*
 * Topic:
 * Thread pool, task submission, futures,
 * exception propagation, and graceful shutdown.
 *
 * Compile:
 * g++ -std=c++20 -O2 -pthread \
 *     thread_pool_future_shutdown.cpp \
 *     -o thread_pool_future_shutdown
 *
 * Run:
 * ./thread_pool_future_shutdown
 */

class ThreadPool {
private:
    vector<thread> workers_;
    queue<function<void()>> tasks_;

    mutex mutex_;
    condition_variable condition_;

    bool stopping_ = false;

    void workerLoop(size_t workerId) {
        while (true) {
            function<void()> task;

            {
                unique_lock lock(mutex_);

                condition_.wait(lock, [&] {
                    return stopping_ ||
                           !tasks_.empty();
                });

                if (
                    stopping_ &&
                    tasks_.empty()
                ) {
                    cout << "worker "
                         << workerId
                         << " exiting\n";

                    return;
                }

                task = move(tasks_.front());
                tasks_.pop();
            }

            // Never execute arbitrary work while
            // holding the task-queue mutex.
            task();
        }
    }

public:
    explicit ThreadPool(size_t workerCount) {
        if (workerCount == 0) {
            throw invalid_argument(
                "worker count must be positive"
            );
        }

        workers_.reserve(workerCount);

        for (
            size_t id = 0;
            id < workerCount;
            ++id
        ) {
            workers_.emplace_back(
                [this, id] {
                    workerLoop(id);
                }
            );
        }
    }

    ThreadPool(const ThreadPool&) = delete;
    ThreadPool& operator=(
        const ThreadPool&
    ) = delete;

    ~ThreadPool() {
        shutdown();
    }

    template <typename F, typename... Args>
    auto submit(F&& function, Args&&... args)
        -> future<
            invoke_result_t<F, Args...>
        > {
        using Result =
            invoke_result_t<F, Args...>;

        auto task = make_shared<
            packaged_task<Result()>
        >(
            bind(
                forward<F>(function),
                forward<Args>(args)...
            )
        );

        future<Result> result =
            task->get_future();

        {
            lock_guard lock(mutex_);

            if (stopping_) {
                throw runtime_error(
                    "submit on stopped ThreadPool"
                );
            }

            tasks_.push([task] {
                (*task)();
            });
        }

        condition_.notify_one();

        return result;
    }

    void shutdown() {
        {
            lock_guard lock(mutex_);

            if (stopping_) {
                return;
            }

            stopping_ = true;
        }

        condition_.notify_all();

        for (thread& worker : workers_) {
            if (worker.joinable()) {
                worker.join();
            }
        }
    }

    size_t workerCount() const {
        return workers_.size();
    }
};

void returnValueDemo() {
    cout << "\n=== return value demo ===\n";

    ThreadPool pool(2);

    future<int> result = pool.submit([] {
        return 6 * 7;
    });

    cout << "result = "
         << result.get()
         << '\n';
}

void argumentsDemo() {
    cout << "\n=== callable arguments demo ===\n";

    ThreadPool pool(2);

    auto add = [](int first, int second) {
        return first + second;
    };

    future<int> result =
        pool.submit(add, 10, 20);

    cout << "10 + 20 = "
         << result.get()
         << '\n';
}

void exceptionPropagationDemo() {
    cout << "\n=== exception propagation demo ===\n";

    ThreadPool pool(1);

    future<int> result =
        pool.submit([]() -> int {
            throw runtime_error(
                "task failed"
            );
        });

    try {
        cout << result.get() << '\n';
    } catch (const exception& error) {
        cout << "future rethrew: "
             << error.what()
             << '\n';
    }
}

void concurrentTasksDemo() {
    cout << "\n=== concurrent tasks demo ===\n";

    ThreadPool pool(3);

    vector<future<int>> results;

    for (int i = 0; i < 8; ++i) {
        results.push_back(
            pool.submit([i] {
                this_thread::sleep_for(
                    chrono::milliseconds(50)
                );

                cout << "task "
                     << i
                     << " ran on thread "
                     << this_thread::get_id()
                     << '\n';

                return i * i;
            })
        );
    }

    for (future<int>& result : results) {
        cout << "square = "
             << result.get()
             << '\n';
    }
}

void gracefulShutdownDemo() {
    cout << "\n=== graceful shutdown demo ===\n";

    vector<future<void>> results;

    {
        ThreadPool pool(2);

        for (int i = 0; i < 5; ++i) {
            results.push_back(
                pool.submit([i] {
                    this_thread::sleep_for(
                        chrono::milliseconds(40)
                    );

                    cout << "completed task "
                         << i
                         << '\n';
                })
            );
        }

        cout << "leaving pool scope\n";
    }

    cout << "pool destructor drained queued tasks\n";

    for (future<void>& result : results) {
        result.get();
    }
}

void submitAfterShutdownDemo() {
    cout << "\n=== submit after shutdown demo ===\n";

    ThreadPool pool(1);

    pool.shutdown();

    try {
        auto result = pool.submit([] {
            return 1;
        });

        cout << result.get() << '\n';
    } catch (const exception& error) {
        cout << "submission rejected: "
             << error.what()
             << '\n';
    }
}

void captureLifetimeWarningDemo() {
    cout << "\n=== capture lifetime warning ===\n";

    cout << "Asynchronous tasks may outlive local variables.\n";
    cout << "Prefer value capture unless reference lifetime\n";
    cout << "is externally guaranteed.\n";
}

void nestedSubmissionWarningDemo() {
    cout << "\n=== nested submission warning ===\n";

    cout << "A worker that submits a child task to the same\n";
    cout << "pool and waits on its future can deadlock if no\n";
    cout << "other worker is available to run the child.\n";

    /*
    ThreadPool pool(1);

    auto outer = pool.submit([&pool] {
        auto inner = pool.submit([] {
            return 42;
        });

        return inner.get(); // deadlock with one worker
    });

    cout << outer.get();
    */
}

void unboundedQueueWarningDemo() {
    cout << "\n=== unbounded queue warning ===\n";

    cout << "A fixed worker count limits execution concurrency,\n";
    cout << "but an unbounded task queue can still accumulate\n";
    cout << "unlimited pending work and latency.\n";
}

int main() {
    returnValueDemo();

    argumentsDemo();

    exceptionPropagationDemo();

    concurrentTasksDemo();

    gracefulShutdownDemo();

    submitAfterShutdownDemo();

    captureLifetimeWarningDemo();

    nestedSubmissionWarningDemo();

    unboundedQueueWarningDemo();

    cout << "\n=== end of main ===\n";

    return 0;
}