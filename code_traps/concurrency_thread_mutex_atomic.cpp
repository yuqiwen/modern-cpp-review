#include <atomic>
#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
using namespace std;

/*
 * Topic:
 * std::thread, data race, mutex, lock_guard, atomic.
 *
 * Compile:
 * g++ -std=c++20 concurrency_thread_mutex_atomic.cpp -o concurrency_thread_mutex_atomic
 *
 * Run:
 * ./concurrency_thread_mutex_atomic
 */

void basicThreadDemo() {
    cout << "\n=== basic thread demo ===" << endl;

    thread t([] {
        cout << "hello from worker thread\n";
    });

    cout << "hello from main thread\n";

    t.join();

    cout << "worker joined\n";
}

void argumentCopyDemo() {
    cout << "\n=== thread argument copy demo ===" << endl;

    int x = 10;

    thread t([](int value) {
        value += 100;
        cout << "inside thread copied value = " << value << endl;
    }, x);

    t.join();

    cout << "outside x = " << x << endl;
}

void argumentReferenceDemo() {
    cout << "\n=== thread argument reference demo ===" << endl;

    int x = 10;

    thread t([](int& value) {
        value += 100;
    }, ref(x));

    t.join();

    cout << "outside x after ref thread = " << x << endl;
}

void dataRaceWarningDemo() {
    cout << "\n=== data race warning demo ===" << endl;

    cout << "Bad example:" << endl;
    cout << "int counter = 0; multiple threads do ++counter without sync." << endl;
    cout << "That is a data race and undefined behavior." << endl;

    // Do not rely on this kind of code:
    //
    // int counter = 0;
    // auto work = [&] {
    //     for (int i = 0; i < 100000; ++i) {
    //         ++counter; // data race
    //     }
    // };
}

void mutexCounterDemo() {
    cout << "\n=== mutex counter demo ===" << endl;

    int counter = 0;
    mutex m;

    auto work = [&] {
        for (int i = 0; i < 100000; ++i) {
            lock_guard<mutex> lock(m);
            ++counter;
        }
    };

    thread t1(work);
    thread t2(work);

    t1.join();
    t2.join();

    cout << "counter with mutex = " << counter << endl;
}

void atomicCounterDemo() {
    cout << "\n=== atomic counter demo ===" << endl;

    atomic<int> counter = 0;

    auto work = [&] {
        for (int i = 0; i < 100000; ++i) {
            ++counter;
        }
    };

    thread t1(work);
    thread t2(work);

    t1.join();
    t2.join();

    cout << "counter with atomic = " << counter.load() << endl;
}

void lockGuardExceptionDemo() {
    cout << "\n=== lock_guard exception demo ===" << endl;

    mutex m;

    try {
        lock_guard<mutex> lock(m);

        cout << "mutex locked\n";

        throw runtime_error("error while mutex is locked");
    } catch (const exception& e) {
        cout << "caught: " << e.what() << endl;
    }

    cout << "lock_guard destructor unlocked the mutex during stack unwinding\n";
}

void uniqueLockDemo() {
    cout << "\n=== unique_lock demo ===" << endl;

    mutex m;

    unique_lock<mutex> lock(m);

    cout << "mutex locked by unique_lock\n";

    lock.unlock();

    cout << "mutex manually unlocked early\n";

    lock.lock();

    cout << "mutex locked again\n";
}

void scopedLockDemo() {
    cout << "\n=== scoped_lock demo ===" << endl;

    mutex m1;
    mutex m2;

    int a = 0;
    int b = 0;

    auto work1 = [&] {
        for (int i = 0; i < 1000; ++i) {
            scoped_lock lock(m1, m2);
            ++a;
            ++b;
        }
    };

    auto work2 = [&] {
        for (int i = 0; i < 1000; ++i) {
            scoped_lock lock(m1, m2);
            ++a;
            ++b;
        }
    };

    thread t1(work1);
    thread t2(work2);

    t1.join();
    t2.join();

    cout << "a = " << a << ", b = " << b << endl;
}

void atomicFlagDemo() {
    cout << "\n=== atomic flag demo ===" << endl;

    atomic<bool> stop = false;

    thread worker([&] {
        while (!stop.load()) {
            this_thread::sleep_for(chrono::milliseconds(10));
        }

        cout << "worker observed stop flag\n";
    });

    this_thread::sleep_for(chrono::milliseconds(50));

    stop.store(true);

    worker.join();

    cout << "worker stopped\n";
}

void vectorPushMutexDemo() {
    cout << "\n=== vector push with mutex demo ===" << endl;

    vector<int> values;
    mutex m;

    auto work = [&](int base) {
        for (int i = 0; i < 5; ++i) {
            lock_guard<mutex> lock(m);
            values.push_back(base + i);
        }
    };

    thread t1(work, 100);
    thread t2(work, 200);

    t1.join();
    t2.join();

    cout << "values size = " << values.size() << endl;

    cout << "values: ";
    for (int x : values) {
        cout << x << " ";
    }
    cout << endl;
}

void threadLifetimeWarningDemo() {
    cout << "\n=== thread lifetime warning demo ===" << endl;

    cout << "If a std::thread object is destroyed while still joinable," << endl;
    cout << "the program calls std::terminate()." << endl;

    cout << "Always join or detach. Prefer join." << endl;
}

int main() {
    basicThreadDemo();

    argumentCopyDemo();

    argumentReferenceDemo();

    dataRaceWarningDemo();

    mutexCounterDemo();

    atomicCounterDemo();

    lockGuardExceptionDemo();

    uniqueLockDemo();

    scopedLockDemo();

    atomicFlagDemo();

    vectorPushMutexDemo();

    threadLifetimeWarningDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}