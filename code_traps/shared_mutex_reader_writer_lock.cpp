#include <chrono>
#include <iostream>
#include <optional>
#include <shared_mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <utility>
#include <vector>
using namespace std;

/*
 * Topic:
 * std::shared_mutex and reader-writer locking.
 *
 * Compile:
 * g++ -std=c++20 -pthread shared_mutex_reader_writer_lock.cpp \
 *     -o shared_mutex_reader_writer_lock
 *
 * Run:
 * ./shared_mutex_reader_writer_lock
 */

class ThreadSafeCache {
private:
    mutable shared_mutex mutex_;
    unordered_map<int, string> data_;

public:
    optional<string> get(int key) const {
        shared_lock lock(mutex_);

        auto it = data_.find(key);

        if (it == data_.end()) {
            return nullopt;
        }

        // Return a copy while the lock is held.
        return it->second;
    }

    void put(int key, string value) {
        unique_lock lock(mutex_);

        data_.insert_or_assign(
            key,
            std::move(value)
        );
    }

    bool erase(int key) {
        unique_lock lock(mutex_);
        return data_.erase(key) > 0;
    }

    size_t size() const {
        shared_lock lock(mutex_);
        return data_.size();
    }
};

void basicReaderWriterDemo() {
    cout << "\n=== basic reader-writer demo ===" << endl;

    shared_mutex mutex;
    int value = 10;

    auto reader = [&](int id) {
        shared_lock lock(mutex);

        cout << "reader " << id
             << " sees value = "
             << value << endl;

        this_thread::sleep_for(
            chrono::milliseconds(150)
        );

        cout << "reader " << id
             << " leaving\n";
    };

    auto writer = [&] {
        this_thread::sleep_for(
            chrono::milliseconds(30)
        );

        unique_lock lock(mutex);

        cout << "writer acquired exclusive lock\n";

        value = 20;

        this_thread::sleep_for(
            chrono::milliseconds(100)
        );

        cout << "writer updated value\n";
    };

    thread r1(reader, 1);
    thread r2(reader, 2);
    thread r3(reader, 3);
    thread w(writer);

    r1.join();
    r2.join();
    r3.join();
    w.join();

    cout << "final value = "
         << value << endl;
}

void cacheDemo() {
    cout << "\n=== thread-safe cache demo ===" << endl;

    ThreadSafeCache cache;

    cache.put(1, "one");
    cache.put(2, "two");

    vector<thread> readers;

    for (int i = 0; i < 4; ++i) {
        readers.emplace_back([&, i] {
            for (int key : {1, 2, 3}) {
                optional<string> value =
                    cache.get(key);

                if (value) {
                    cout << "reader " << i
                         << " key " << key
                         << " = " << *value
                         << endl;
                } else {
                    cout << "reader " << i
                         << " key " << key
                         << " missing\n";
                }
            }
        });
    }

    thread writer([&] {
        cache.put(3, "three");
        cache.erase(1);
    });

    for (thread& reader : readers) {
        reader.join();
    }

    writer.join();

    cout << "cache size = "
         << cache.size()
         << endl;
}

void hiddenMutationWarningDemo() {
    cout << "\n=== hidden mutation warning ===" << endl;

    cout << "unordered_map::operator[] may insert a missing key.\n";
    cout << "Therefore it must not be used under only a shared lock.\n";
    cout << "Use find() for a true read-only lookup.\n";
}

class UnsafeReferenceCache {
private:
    mutable shared_mutex mutex_;
    unordered_map<int, string> data_{
        {1, "one"}
    };

public:
    const string& unsafeGet(int key) const {
        shared_lock lock(mutex_);

        // The returned reference outlives the lock.
        return data_.at(key);
    }

    void erase(int key) {
        unique_lock lock(mutex_);
        data_.erase(key);
    }
};

void returningReferenceWarningDemo() {
    cout << "\n=== returning reference warning ===" << endl;

    cout << "Returning a reference to internal protected data is unsafe\n";
    cout << "because the lock is released before the caller finishes using it.\n";

    cout << "Another writer may modify or erase the element.\n";
}

class LazyCache {
private:
    mutable shared_mutex mutex_;
    unordered_map<int, string> data_;

    static string expensiveCompute(int key) {
        this_thread::sleep_for(
            chrono::milliseconds(50)
        );

        return "value-" + to_string(key);
    }

public:
    string getOrCompute(int key) {
        {
            shared_lock readLock(mutex_);

            auto it = data_.find(key);

            if (it != data_.end()) {
                return it->second;
            }
        }

        // Compute outside lock.
        string computed = expensiveCompute(key);

        {
            unique_lock writeLock(mutex_);

            // Must check again because state may have changed
            // after releasing the shared lock.
            auto [it, inserted] = data_.try_emplace(
                key,
                std::move(computed)
            );

            return it->second;
        }
    }
};

void lockUpgradeDemo() {
    cout << "\n=== read-to-write recheck demo ===" << endl;

    LazyCache cache;

    thread t1([&] {
        cout << "t1 result = "
             << cache.getOrCompute(5)
             << endl;
    });

    thread t2([&] {
        cout << "t2 result = "
             << cache.getOrCompute(5)
             << endl;
    });

    t1.join();
    t2.join();

    cout << "Both threads may compute, but only one value is inserted.\n";
}

void trySharedLockDemo() {
    cout << "\n=== try shared lock demo ===" << endl;

    shared_mutex mutex;
    int value = 42;

    unique_lock writerLock(mutex);

    thread reader([&] {
        shared_lock readLock(
            mutex,
            try_to_lock
        );

        if (!readLock.owns_lock()) {
            cout << "reader could not acquire shared lock\n";
            return;
        }

        cout << "reader sees "
             << value << endl;
    });

    this_thread::sleep_for(
        chrono::milliseconds(50)
    );

    writerLock.unlock();

    reader.join();
}

void performanceWarningDemo() {
    cout << "\n=== performance warning ===" << endl;

    cout << "shared_mutex is not automatically faster than mutex.\n";
    cout << "Use it for genuinely read-heavy contended workloads,\n";
    cout << "and validate the benefit with measurement.\n";
}

int main() {
    basicReaderWriterDemo();

    cacheDemo();

    hiddenMutationWarningDemo();

    returningReferenceWarningDemo();

    lockUpgradeDemo();

    trySharedLockDemo();

    performanceWarningDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}