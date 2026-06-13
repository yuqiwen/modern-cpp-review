#include <chrono>
#include <condition_variable>
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
 * std::condition_variable and producer-consumer queue.
 *
 * Compile:
 * g++ -std=c++20 -pthread condition_variable_producer_consumer.cpp \
 *     -o condition_variable_producer_consumer
 *
 * Run:
 * ./condition_variable_producer_consumer
 */

class BlockingQueue {
private:
    queue<int> queue_;
    mutex mutex_;
    condition_variable cv_;
    bool stopped_ = false;

public:
    void push(int value) {
        {
            lock_guard<mutex> lock(mutex_);

            if (stopped_) {
                throw runtime_error("push on stopped queue");
            }

            queue_.push(value);

            cout << "produced: " << value << endl;
        }

        cv_.notify_one();
    }

    optional<int> pop() {
        unique_lock<mutex> lock(mutex_);

        cv_.wait(lock, [&] {
            return stopped_ || !queue_.empty();
        });

        if (queue_.empty()) {
            return nullopt;
        }

        int value = queue_.front();
        queue_.pop();

        return value;
    }

    void stop() {
        {
            lock_guard<mutex> lock(mutex_);
            stopped_ = true;
        }

        cv_.notify_all();
    }
};

void basicWaitDemo() {
    cout << "\n=== basic condition_variable demo ===" << endl;

    mutex m;
    condition_variable cv;
    bool ready = false;

    thread worker([&] {
        unique_lock<mutex> lock(m);

        cv.wait(lock, [&] {
            return ready;
        });

        cout << "worker observed ready = true\n";
    });

    this_thread::sleep_for(chrono::milliseconds(100));

    {
        lock_guard<mutex> lock(m);
        ready = true;
    }

    cv.notify_one();

    worker.join();
}

void producerConsumerDemo() {
    cout << "\n=== producer-consumer demo ===" << endl;

    BlockingQueue queue;

    thread consumer1([&] {
        while (true) {
            optional<int> value = queue.pop();

            if (!value) {
                cout << "consumer 1 exiting\n";
                return;
            }

            cout << "consumer 1 consumed: " << *value << endl;
            this_thread::sleep_for(chrono::milliseconds(40));
        }
    });

    thread consumer2([&] {
        while (true) {
            optional<int> value = queue.pop();

            if (!value) {
                cout << "consumer 2 exiting\n";
                return;
            }

            cout << "consumer 2 consumed: " << *value << endl;
            this_thread::sleep_for(chrono::milliseconds(40));
        }
    });

    thread producer1([&] {
        for (int i = 1; i <= 5; ++i) {
            queue.push(i);
            this_thread::sleep_for(chrono::milliseconds(20));
        }
    });

    thread producer2([&] {
        for (int i = 101; i <= 105; ++i) {
            queue.push(i);
            this_thread::sleep_for(chrono::milliseconds(25));
        }
    });

    producer1.join();
    producer2.join();

    cout << "all producers finished\n";

    queue.stop();

    consumer1.join();
    consumer2.join();

    cout << "all consumers finished\n";
}

void lostNotificationExplanationDemo() {
    cout << "\n=== lost notification explanation ===" << endl;

    mutex m;
    condition_variable cv;
    bool ready = false;

    {
        lock_guard<mutex> lock(m);
        ready = true;
    }

    // Notification occurs before worker starts waiting.
    cv.notify_one();

    thread worker([&] {
        unique_lock<mutex> lock(m);

        cv.wait(lock, [&] {
            return ready;
        });

        cout << "worker did not block because predicate was already true\n";
    });

    worker.join();

    cout << "The notification was not stored, but shared state was stored.\n";
}

void timedWaitDemo() {
    cout << "\n=== timed wait demo ===" << endl;

    mutex m;
    condition_variable cv;
    bool ready = false;

    unique_lock<mutex> lock(m);

    bool success = cv.wait_for(
        lock,
        chrono::milliseconds(100),
        [&] {
            return ready;
        }
    );

    if (success) {
        cout << "condition became true\n";
    } else {
        cout << "wait timed out\n";
    }
}

class BoundedQueue {
private:
    queue<int> queue_;
    size_t capacity_;

    mutex mutex_;
    condition_variable notEmpty_;
    condition_variable notFull_;

    bool stopped_ = false;

public:
    explicit BoundedQueue(size_t capacity)
        : capacity_(capacity) {}

    bool push(int value) {
        unique_lock<mutex> lock(mutex_);

        notFull_.wait(lock, [&] {
            return stopped_ || queue_.size() < capacity_;
        });

        if (stopped_) {
            return false;
        }

        queue_.push(value);

        lock.unlock();
        notEmpty_.notify_one();

        return true;
    }

    optional<int> pop() {
        unique_lock<mutex> lock(mutex_);

        notEmpty_.wait(lock, [&] {
            return stopped_ || !queue_.empty();
        });

        if (queue_.empty()) {
            return nullopt;
        }

        int value = queue_.front();
        queue_.pop();

        lock.unlock();
        notFull_.notify_one();

        return value;
    }

    void stop() {
        {
            lock_guard<mutex> lock(mutex_);
            stopped_ = true;
        }

        notEmpty_.notify_all();
        notFull_.notify_all();
    }
};

void boundedQueueDemo() {
    cout << "\n=== bounded queue demo ===" << endl;

    BoundedQueue queue(2);

    thread producer([&] {
        for (int i = 0; i < 5; ++i) {
            if (!queue.push(i)) {
                return;
            }

            cout << "bounded producer pushed: " << i << endl;
        }
    });

    thread consumer([&] {
        for (int i = 0; i < 5; ++i) {
            optional<int> value = queue.pop();

            if (!value) {
                return;
            }

            cout << "bounded consumer popped: " << *value << endl;
            this_thread::sleep_for(chrono::milliseconds(80));
        }
    });

    producer.join();
    consumer.join();

    queue.stop();
}

int main() {
    basicWaitDemo();

    producerConsumerDemo();

    lostNotificationExplanationDemo();

    timedWaitDemo();

    boundedQueueDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}