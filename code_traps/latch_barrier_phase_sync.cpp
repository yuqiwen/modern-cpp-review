#include <barrier>
#include <chrono>
#include <iostream>
#include <latch>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
using namespace std;

/*
 * Topic:
 * std::latch, std::barrier, and phase synchronization.
 *
 * Compile:
 * g++ -std=c++20 -pthread latch_barrier_phase_sync.cpp \
 *     -o latch_barrier_phase_sync
 *
 * Run:
 * ./latch_barrier_phase_sync
 */

void latchCompletionDemo() {
    cout << "\n=== latch completion demo ===" << endl;

    constexpr int workerCount = 3;

    latch done(workerCount);
    mutex outputMutex;

    vector<thread> workers;

    for (int id = 0; id < workerCount; ++id) {
        workers.emplace_back([&, id] {
            {
                lock_guard<mutex> lock(outputMutex);
                cout << "worker " << id << " started\n";
            }

            this_thread::sleep_for(
                chrono::milliseconds(100 + id * 50)
            );

            {
                lock_guard<mutex> lock(outputMutex);
                cout << "worker " << id << " finished\n";
            }

            done.count_down();
        });
    }

    cout << "main waiting for all workers\n";

    done.wait();

    cout << "all workers reached latch\n";

    for (thread& worker : workers) {
        worker.join();
    }
}

void latchStartLineDemo() {
    cout << "\n=== latch start-line demo ===" << endl;

    constexpr int participantCount = 4;

    latch startLine(participantCount);
    mutex outputMutex;

    vector<thread> threads;

    for (int id = 0; id < participantCount; ++id) {
        threads.emplace_back([&, id] {
            {
                lock_guard<mutex> lock(outputMutex);
                cout << "thread " << id
                     << " preparing\n";
            }

            this_thread::sleep_for(
                chrono::milliseconds(id * 40)
            );

            {
                lock_guard<mutex> lock(outputMutex);
                cout << "thread " << id
                     << " reached start line\n";
            }

            startLine.arrive_and_wait();

            {
                lock_guard<mutex> lock(outputMutex);
                cout << "thread " << id
                     << " started together\n";
            }
        });
    }

    for (thread& t : threads) {
        t.join();
    }
}

void barrierPhaseDemo() {
    cout << "\n=== reusable barrier phase demo ===" << endl;

    constexpr int workerCount = 3;
    constexpr int phaseCount = 3;

    mutex outputMutex;
    int completedPhase = 0;

    barrier phaseBarrier(
        workerCount,
        [&] noexcept {
            lock_guard<mutex> lock(outputMutex);

            cout << "--- all workers completed phase "
                 << completedPhase
                 << " ---\n";

            ++completedPhase;
        }
    );

    vector<thread> workers;

    for (int id = 0; id < workerCount; ++id) {
        workers.emplace_back([&, id] {
            for (int phase = 0; phase < phaseCount; ++phase) {
                {
                    lock_guard<mutex> lock(outputMutex);

                    cout << "worker " << id
                         << " working on phase "
                         << phase << endl;
                }

                this_thread::sleep_for(
                    chrono::milliseconds(
                        50 + id * 30
                    )
                );

                {
                    lock_guard<mutex> lock(outputMutex);

                    cout << "worker " << id
                         << " arrived at barrier for phase "
                         << phase << endl;
                }

                phaseBarrier.arrive_and_wait();
            }
        });
    }

    for (thread& worker : workers) {
        worker.join();
    }
}

void arriveAndDropDemo() {
    cout << "\n=== arrive_and_drop demo ===" << endl;

    barrier phaseBarrier(3);
    mutex outputMutex;

    thread worker1([&] {
        for (int phase = 0; phase < 3; ++phase) {
            {
                lock_guard<mutex> lock(outputMutex);
                cout << "worker 1 phase "
                     << phase << endl;
            }

            phaseBarrier.arrive_and_wait();
        }
    });

    thread worker2([&] {
        for (int phase = 0; phase < 3; ++phase) {
            {
                lock_guard<mutex> lock(outputMutex);
                cout << "worker 2 phase "
                     << phase << endl;
            }

            phaseBarrier.arrive_and_wait();
        }
    });

    thread temporaryWorker([&] {
        {
            lock_guard<mutex> lock(outputMutex);
            cout << "temporary worker participates "
                    "only in phase 0\n";
        }

        phaseBarrier.arrive_and_drop();

        cout << "temporary worker permanently left barrier\n";
    });

    worker1.join();
    worker2.join();
    temporaryWorker.join();
}

void latchMemoryVisibilityDemo() {
    cout << "\n=== latch result visibility demo ===" << endl;

    constexpr int workerCount = 4;

    vector<int> results(workerCount, 0);
    latch done(workerCount);

    vector<thread> workers;

    for (int id = 0; id < workerCount; ++id) {
        workers.emplace_back([&, id] {
            // Each worker writes a separate element.
            results[id] = id * id;

            done.count_down();
        });
    }

    done.wait();

    cout << "results after latch: ";

    for (int value : results) {
        cout << value << " ";
    }

    cout << endl;

    for (thread& worker : workers) {
        worker.join();
    }
}

void incorrectCountWarningDemo() {
    cout << "\n=== incorrect participant count warning ===" << endl;

    cout << "If barrier expects 4 participants but only 3 arrive,\n";
    cout << "the three threads may wait forever.\n";

    cout << "If latch starts at 3 but only 2 count_down calls occur,\n";
    cout << "wait may block forever.\n";
}

int main() {
    latchCompletionDemo();

    latchStartLineDemo();

    barrierPhaseDemo();

    arriveAndDropDemo();

    latchMemoryVisibilityDemo();

    incorrectCountWarningDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}