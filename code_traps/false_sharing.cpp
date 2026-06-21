#include <atomic>
#include <chrono>
#include <iostream>
#include <thread>
using namespace std;

constexpr long long Iterations = 50'000'000;

struct BadCounters {
    atomic<long long> first{0};
    atomic<long long> second{0};
};

struct alignas(64) PaddedCounter {
    atomic<long long> value{0};
};

struct GoodCounters {
    PaddedCounter first;
    PaddedCounter second;
};

template <typename F>
long long measureMilliseconds(F&& function) {
    auto start = chrono::steady_clock::now();

    function();

    auto end = chrono::steady_clock::now();

    return chrono::duration_cast<chrono::milliseconds>(
        end - start
    ).count();
}

void falseSharingDemo() {
    cout << "\n=== possible false sharing ===\n";

    BadCounters counters;

    long long elapsed = measureMilliseconds([&] {
        thread firstThread([&] {
            for (long long i = 0; i < Iterations; ++i) {
                counters.first.fetch_add(
                    1,
                    memory_order_relaxed
                );
            }
        });

        thread secondThread([&] {
            for (long long i = 0; i < Iterations; ++i) {
                counters.second.fetch_add(
                    1,
                    memory_order_relaxed
                );
            }
        });

        firstThread.join();
        secondThread.join();
    });

    cout << "first = " << counters.first.load() << '\n';
    cout << "second = " << counters.second.load() << '\n';
    cout << "elapsed = " << elapsed << " ms\n";
}

void paddedCounterDemo() {
    cout << "\n=== padded counters ===\n";

    GoodCounters counters;

    long long elapsed = measureMilliseconds([&] {
        thread firstThread([&] {
            for (long long i = 0; i < Iterations; ++i) {
                counters.first.value.fetch_add(
                    1,
                    memory_order_relaxed
                );
            }
        });

        thread secondThread([&] {
            for (long long i = 0; i < Iterations; ++i) {
                counters.second.value.fetch_add(
                    1,
                    memory_order_relaxed
                );
            }
        });

        firstThread.join();
        secondThread.join();
    });

    cout << "first = "
         << counters.first.value.load()
         << '\n';

    cout << "second = "
         << counters.second.value.load()
         << '\n';

    cout << "elapsed = " << elapsed << " ms\n";
}

int main() {
    cout << "BadCounters size = "
         << sizeof(BadCounters)
         << '\n';

    cout << "PaddedCounter size = "
         << sizeof(PaddedCounter)
         << '\n';

    falseSharingDemo();
    paddedCounterDemo();

    return 0;
}