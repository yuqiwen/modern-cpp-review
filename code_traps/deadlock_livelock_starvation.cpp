#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <utility>
using namespace std;

/*
 * Topic:
 * Deadlock, livelock, starvation, and lock ordering.
 *
 * Compile:
 * g++ -std=c++20 -O2 -pthread \
 *     deadlock_livelock_starvation.cpp \
 *     -o deadlock_livelock_starvation
 *
 * Run:
 * ./deadlock_livelock_starvation
 */

void oppositeLockOrderWarning() {
    cout << "\n=== opposite lock order warning ===\n";

    cout << "Thread A: lock m1, then m2\n";
    cout << "Thread B: lock m2, then m1\n";
    cout << "This can create circular wait and deadlock.\n";

    /*
    mutex m1;
    mutex m2;

    thread a([&] {
        lock_guard lock1(m1);
        this_thread::sleep_for(100ms);
        lock_guard lock2(m2);
    });

    thread b([&] {
        lock_guard lock2(m2);
        this_thread::sleep_for(100ms);
        lock_guard lock1(m1);
    });

    a.join();
    b.join();
    */
}

void scopedLockDemo() {
    cout << "\n=== scoped_lock demo ===\n";

    mutex first;
    mutex second;

    int firstValue = 0;
    int secondValue = 0;

    auto worker = [&] {
        for (int i = 0; i < 10000; ++i) {
            scoped_lock lock(first, second);

            ++firstValue;
            ++secondValue;
        }
    };

    thread a(worker);
    thread b(worker);

    a.join();
    b.join();

    cout << "firstValue = "
         << firstValue << '\n';

    cout << "secondValue = "
         << secondValue << '\n';
}

struct Account {
    int id;
    int balance;
    mutex accountMutex;
};

void transfer(
    Account& from,
    Account& to,
    int amount
) {
    if (&from == &to) {
        return;
    }

    scoped_lock lock(
        from.accountMutex,
        to.accountMutex
    );

    if (from.balance < amount) {
        return;
    }

    from.balance -= amount;
    to.balance += amount;
}

void bankTransferDemo() {
    cout << "\n=== bank transfer demo ===\n";

    Account first{
        1,
        1000
    };

    Account second{
        2,
        1000
    };

    thread a([&] {
        for (int i = 0; i < 1000; ++i) {
            transfer(first, second, 1);
        }
    });

    thread b([&] {
        for (int i = 0; i < 1000; ++i) {
            transfer(second, first, 1);
        }
    });

    a.join();
    b.join();

    cout << "first balance = "
         << first.balance << '\n';

    cout << "second balance = "
         << second.balance << '\n';

    cout << "total balance = "
         << first.balance + second.balance
         << '\n';
}

void joinUnderLockWarning() {
    cout << "\n=== join-under-lock warning ===\n";

    cout << "Do not hold a mutex while joining a worker\n";
    cout << "if the worker may need that mutex to exit.\n";

    /*
    mutex m;

    thread worker([&] {
        lock_guard lock(m);
        cout << "worker finishing\n";
    });

    {
        lock_guard lock(m);
        worker.join(); // deadlock
    }
    */
}

void callbackUnderLockWarning() {
    cout << "\n=== callback-under-lock warning ===\n";

    cout << "Do not invoke unknown callbacks while holding\n";
    cout << "an internal mutex. The callback may re-enter\n";
    cout << "the object or acquire another lock.\n";
}

void sameMutexRecursiveWarning() {
    cout << "\n=== same-mutex recursive warning ===\n";

    cout << "A normal std::mutex cannot be locked twice\n";
    cout << "by the same thread.\n";

    /*
    mutex m;

    m.lock();
    m.lock(); // may block forever
    */
}

void livelockExplanation() {
    cout << "\n=== livelock explanation ===\n";

    cout << "Deadlock: threads stop moving.\n";
    cout << "Livelock: threads keep retrying,\n";
    cout << "but no useful operation completes.\n";

    cout << "Consistent lock ordering or backoff\n";
    cout << "can prevent synchronized retries.\n";
}

void starvationExplanation() {
    cout << "\n=== starvation explanation ===\n";

    cout << "Starvation means the system progresses,\n";
    cout << "but one thread repeatedly fails to obtain\n";
    cout << "CPU time, a lock, or successful CAS.\n";
}

void tryLockBackoffDemo() {
    cout << "\n=== try_lock with backoff demo ===\n";

    mutex first;
    mutex second;

    auto worker = [&](int id) {
        for (int attempt = 0; attempt < 5; ++attempt) {
            unique_lock firstLock(
                first,
                defer_lock
            );

            unique_lock secondLock(
                second,
                defer_lock
            );

            if (
                try_lock(
                    firstLock,
                    secondLock
                ) == -1
            ) {
                cout << "worker "
                     << id
                     << " acquired both locks\n";

                return;
            }

            this_thread::sleep_for(
                chrono::milliseconds(
                    10 + id * 5
                )
            );
        }

        cout << "worker "
             << id
             << " gave up after retries\n";
    };

    thread a(worker, 1);
    thread b(worker, 2);

    a.join();
    b.join();
}

void priorityInversionExplanation() {
    cout << "\n=== priority inversion explanation ===\n";

    cout << "Low-priority thread owns a mutex.\n";
    cout << "High-priority thread waits for that mutex.\n";
    cout << "Medium-priority work prevents the low-priority\n";
    cout << "thread from running and releasing it.\n";
}

int main() {
    oppositeLockOrderWarning();

    scopedLockDemo();

    bankTransferDemo();

    joinUnderLockWarning();

    callbackUnderLockWarning();

    sameMutexRecursiveWarning();

    livelockExplanation();

    starvationExplanation();

    tryLockBackoffDemo();

    priorityInversionExplanation();

    cout << "\n=== end of main ===\n";

    return 0;
}