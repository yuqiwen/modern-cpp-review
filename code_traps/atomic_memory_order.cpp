#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
using namespace std;

/*
 * Topic:
 * std::atomic memory ordering.
 *
 * Compile:
 * g++ -std=c++20 -O2 -pthread atomic_memory_order.cpp \
 *     -o atomic_memory_order
 *
 * Run:
 * ./atomic_memory_order
 */

void relaxedCounterDemo() {
    cout << "\n=== relaxed counter demo ===" << endl;

    atomic<int> counter = 0;

    auto worker = [&] {
        for (int i = 0; i < 100000; ++i) {
            counter.fetch_add(
                1,
                memory_order_relaxed
            );
        }
    };

    thread t1(worker);
    thread t2(worker);

    t1.join();
    t2.join();

    cout << "counter = "
         << counter.load(memory_order_relaxed)
         << endl;
}

struct Payload {
    int id = 0;
    string message;
};

void releaseAcquireDemo() {
    cout << "\n=== release/acquire publication demo ===" << endl;

    Payload payload;
    atomic<bool> ready = false;

    thread producer([&] {
        payload.id = 42;
        payload.message = "published data";

        ready.store(
            true,
            memory_order_release
        );
    });

    thread consumer([&] {
        while (!ready.load(memory_order_acquire)) {
            this_thread::yield();
        }

        cout << "payload.id = "
             << payload.id
             << endl;

        cout << "payload.message = "
             << payload.message
             << endl;
    });

    producer.join();
    consumer.join();
}

void sequentialConsistencyDemo() {
    cout << "\n=== sequential consistency demo ===" << endl;

    atomic<int> value = 0;

    thread writer([&] {
        value.store(10); // seq_cst by default
    });

    thread reader([&] {
        int observed = value.load(); // seq_cst by default
        cout << "observed value = "
             << observed
             << endl;
    });

    writer.join();
    reader.join();

    cout << "final value = "
         << value.load()
         << endl;
}

void compareExchangeDemo() {
    cout << "\n=== compare_exchange demo ===" << endl;

    atomic<int> state = 0;

    int expected = 0;

    bool success = state.compare_exchange_strong(
        expected,
        1
    );

    cout << boolalpha;
    cout << "success = " << success << endl;
    cout << "state = " << state.load() << endl;
    cout << "expected = " << expected << endl;

    expected = 0;

    success = state.compare_exchange_strong(
        expected,
        2
    );

    cout << "second success = " << success << endl;
    cout << "state = " << state.load() << endl;
    cout << "expected after failure = "
         << expected
         << endl;
}

void updateMax(atomic<int>& maxValue, int candidate) {
    int current = maxValue.load(memory_order_relaxed);

    while (
        candidate > current &&
        !maxValue.compare_exchange_weak(
            current,
            candidate,
            memory_order_relaxed,
            memory_order_relaxed
        )
    ) {
        // On failure, current is updated with actual maxValue.
    }
}

void compareExchangeLoopDemo() {
    cout << "\n=== compare_exchange loop demo ===" << endl;

    atomic<int> maximum = 0;

    vector<thread> threads;

    for (int value : {5, 20, 3, 100, 45, 80}) {
        threads.emplace_back([&, value] {
            updateMax(maximum, value);
        });
    }

    for (thread& t : threads) {
        t.join();
    }

    cout << "maximum = "
         << maximum.load(memory_order_relaxed)
         << endl;
}

void atomicWaitDemo() {
    cout << "\n=== atomic wait/notify demo ===" << endl;

    atomic<bool> ready = false;

    int data = 0;

    thread consumer([&] {
        ready.wait(
            false,
            memory_order_acquire
        );

        cout << "consumer sees data = "
             << data
             << endl;
    });

    thread producer([&] {
        data = 123;

        ready.store(
            true,
            memory_order_release
        );

        ready.notify_one();
    });

    producer.join();
    consumer.join();
}

void compoundInvariantWarningDemo() {
    cout << "\n=== compound invariant warning ===" << endl;

    atomic<int> accountBalance = 100;
    atomic<int> transactionCount = 0;

    cout << "Separate atomics do not make this pair "
            "one atomic transaction."
         << endl;

    cout << "balance = "
         << accountBalance.load()
         << ", count = "
         << transactionCount.load()
         << endl;

    cout << "Use a mutex if both values must always "
            "change and be observed together."
         << endl;
}

int main() {
    relaxedCounterDemo();

    releaseAcquireDemo();

    sequentialConsistencyDemo();

    compareExchangeDemo();

    compareExchangeLoopDemo();

    atomicWaitDemo();

    compoundInvariantWarningDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}