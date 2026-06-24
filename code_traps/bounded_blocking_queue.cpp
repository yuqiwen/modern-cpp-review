#include <chrono>
#include <condition_variable>
#include <cstddef>
#include <iostream>
#include <mutex>
#include <optional>
#include <queue>
#include <string>
#include <thread>
#include <utility>
#include <vector>

using namespace std;

/*
 * Topic:
 * Bounded blocking queue, backpressure,
 * and graceful shutdown.
 *
 * Compile:
 * g++ -std=c++20 -O2 -pthread \
 *     bounded_blocking_queue.cpp \
 *     -o bounded_blocking_queue
 *
 * Run:
 * ./bounded_blocking_queue
 */

template <typename T>
class BoundedBlockingQueue {
private:
    queue<T> queue_;
    size_t capacity_;
    bool closed_ = false;

    mutable mutex mutex_;
    condition_variable notEmpty_;
    condition_variable notFull_;

public:
    explicit BoundedBlockingQueue(size_t capacity)
        : capacity_(capacity) {
        if (capacity_ == 0) {
            throw invalid_argument(
                "queue capacity must be positive"
            );
        }
    }

    BoundedBlockingQueue(
        const BoundedBlockingQueue&
    ) = delete;

    BoundedBlockingQueue& operator=(
        const BoundedBlockingQueue&
    ) = delete;

    bool push(T value) {
        unique_lock lock(mutex_);

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

    bool tryPush(T value) {
        {
            lock_guard lock(mutex_);

            if (
                closed_ ||
                queue_.size() >= capacity_
            ) {
                return false;
            }

            queue_.push(std::move(value));
        }

        notEmpty_.notify_one();
        return true;
    }

    template <typename Rep, typename Period>
    bool pushFor(
        T value,
        const chrono::duration<Rep, Period>& timeout
    ) {
        unique_lock lock(mutex_);

        bool ready = notFull_.wait_for(
            lock,
            timeout,
            [&] {
                return closed_ ||
                       queue_.size() < capacity_;
            }
        );

        if (!ready || closed_) {
            return false;
        }

        queue_.push(std::move(value));

        lock.unlock();
        notEmpty_.notify_one();

        return true;
    }

    bool pop(T& output) {
        unique_lock lock(mutex_);

        notEmpty_.wait(lock, [&] {
            return closed_ ||
                   !queue_.empty();
        });

        if (queue_.empty()) {
            // Queue is closed and fully drained.
            return false;
        }

        output = std::move(queue_.front());
        queue_.pop();

        lock.unlock();
        notFull_.notify_one();

        return true;
    }

    void close() {
        {
            lock_guard lock(mutex_);

            if (closed_) {
                return;
            }

            closed_ = true;
        }

        notEmpty_.notify_all();
        notFull_.notify_all();
    }

    bool isClosed() const {
        lock_guard lock(mutex_);
        return closed_;
    }

    size_t size() const {
        lock_guard lock(mutex_);
        return queue_.size();
    }

    size_t capacity() const {
        return capacity_;
    }
};

void basicProducerConsumerDemo() {
    cout << "\n=== bounded producer-consumer demo ===\n";

    BoundedBlockingQueue<int> queue(3);

    thread producer([&] {
        for (int value = 1; value <= 8; ++value) {
            cout << "producer wants to push "
                 << value << '\n';

            if (!queue.push(value)) {
                cout << "producer sees closed queue\n";
                return;
            }

            cout << "producer pushed "
                 << value << '\n';

            this_thread::sleep_for(
                chrono::milliseconds(30)
            );
        }

        cout << "producer finished, closing queue\n";
        queue.close();
    });

    thread consumer([&] {
        int value;

        while (queue.pop(value)) {
            cout << "consumer processing "
                 << value << '\n';

            this_thread::sleep_for(
                chrono::milliseconds(120)
            );
        }

        cout << "consumer observed closed and empty queue\n";
    });

    producer.join();
    consumer.join();
}

void multipleWorkersDemo() {
    cout << "\n=== multiple workers demo ===\n";

    BoundedBlockingQueue<string> queue(4);

    vector<thread> workers;

    for (int workerId = 0; workerId < 3; ++workerId) {
        workers.emplace_back([&, workerId] {
            string task;

            while (queue.pop(task)) {
                cout << "worker "
                     << workerId
                     << " processing "
                     << task
                     << '\n';

                this_thread::sleep_for(
                    chrono::milliseconds(80)
                );
            }

            cout << "worker "
                 << workerId
                 << " exiting\n";
        });
    }

    for (int i = 0; i < 10; ++i) {
        queue.push(
            "task-" + to_string(i)
        );
    }

    queue.close();

    for (thread& worker : workers) {
        worker.join();
    }
}

void tryPushDemo() {
    cout << "\n=== tryPush rejection demo ===\n";

    BoundedBlockingQueue<int> queue(2);

    cout << boolalpha;

    cout << "push 1: "
         << queue.tryPush(1)
         << '\n';

    cout << "push 2: "
         << queue.tryPush(2)
         << '\n';

    cout << "push 3 while full: "
         << queue.tryPush(3)
         << '\n';

    queue.close();

    int value;

    while (queue.pop(value)) {
        cout << "drained "
             << value << '\n';
    }
}

void timedPushDemo() {
    cout << "\n=== timed push demo ===\n";

    BoundedBlockingQueue<int> queue(1);

    queue.push(10);

    bool inserted = queue.pushFor(
        20,
        chrono::milliseconds(100)
    );

    cout << boolalpha;
    cout << "timed push succeeded: "
         << inserted
         << '\n';

    queue.close();

    int value;

    while (queue.pop(value)) {
        cout << "remaining item: "
             << value << '\n';
    }
}

void closeWakesWaitersDemo() {
    cout << "\n=== close wakes waiters demo ===\n";

    BoundedBlockingQueue<int> queue(1);

    thread consumer([&] {
        int value;

        cout << "consumer waiting on empty queue\n";

        bool success = queue.pop(value);

        cout << boolalpha;
        cout << "consumer pop result after close: "
             << success
             << '\n';
    });

    this_thread::sleep_for(
        chrono::milliseconds(100)
    );

    cout << "main closing queue\n";
    queue.close();

    consumer.join();
}

void gracefulDrainDemo() {
    cout << "\n=== graceful drain demo ===\n";

    BoundedBlockingQueue<int> queue(5);

    queue.push(1);
    queue.push(2);
    queue.push(3);

    queue.close();

    cout << "queue closed with items still pending\n";

    int value;

    while (queue.pop(value)) {
        cout << "drained task "
             << value << '\n';
    }

    cout << "queue now closed and empty\n";
}

void overloadPolicyExplanation() {
    cout << "\n=== overload policies ===\n";

    cout << "When full, a queue may:\n";
    cout << "- block producer\n";
    cout << "- reject immediately\n";
    cout << "- wait with timeout\n";
    cout << "- drop newest\n";
    cout << "- drop oldest\n";
    cout << "- shed low-priority work\n";
}

int main() {
    basicProducerConsumerDemo();

    multipleWorkersDemo();

    tryPushDemo();

    timedPushDemo();

    closeWakesWaitersDemo();

    gracefulDrainDemo();

    overloadPolicyExplanation();

    cout << "\n=== end of main ===\n";

    return 0;
}