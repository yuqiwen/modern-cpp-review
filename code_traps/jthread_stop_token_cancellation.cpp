#include <chrono>
#include <condition_variable>
#include <iostream>
#include <jthread>
#include <mutex>
#include <queue>
#include <stop_token>
#include <string>
#include <thread>
using namespace std;

/*
 * Topic:
 * std::jthread, stop_token, and cooperative cancellation.
 *
 * Compile:
 * g++ -std=c++20 -pthread jthread_stop_token_cancellation.cpp \
 *     -o jthread_stop_token_cancellation
 *
 * Run:
 * ./jthread_stop_token_cancellation
 */

void basicJthreadDemo() {
    cout << "\n=== basic jthread demo ===" << endl;

    {
        jthread worker([] {
            cout << "worker running\n";
        });

        cout << "main continues\n";
    }

    cout << "jthread was automatically joined\n";
}

void stopTokenLoopDemo() {
    cout << "\n=== stop_token loop demo ===" << endl;

    jthread worker([](stop_token token) {
        int iteration = 0;

        while (!token.stop_requested()) {
            cout << "worker iteration "
                 << iteration++
                 << endl;

            this_thread::sleep_for(
                chrono::milliseconds(100)
            );
        }

        cout << "worker observed stop request\n";
    });

    this_thread::sleep_for(
        chrono::milliseconds(350)
    );

    cout << "main requesting stop\n";

    worker.request_stop();

    // Explicit join is optional here,
    // but makes the waiting point visible.
    worker.join();

    cout << "worker joined\n";
}

void automaticDestructorStopDemo() {
    cout << "\n=== destructor request_stop demo ===" << endl;

    {
        jthread worker([](stop_token token) {
            while (!token.stop_requested()) {
                cout << "working...\n";

                this_thread::sleep_for(
                    chrono::milliseconds(100)
                );
            }

            cout << "destructor requested stop\n";
        });

        this_thread::sleep_for(
            chrono::milliseconds(250)
        );

        cout << "leaving scope\n";
    }

    cout << "jthread destructor requested stop and joined\n";
}

void stopSourceDemo() {
    cout << "\n=== stop_source demo ===" << endl;

    stop_source source;
    stop_token token = source.get_token();

    jthread worker1([token] {
        while (!token.stop_requested()) {
            this_thread::sleep_for(
                chrono::milliseconds(50)
            );
        }

        cout << "worker 1 stopped\n";
    });

    jthread worker2([token] {
        while (!token.stop_requested()) {
            this_thread::sleep_for(
                chrono::milliseconds(50)
            );
        }

        cout << "worker 2 stopped\n";
    });

    this_thread::sleep_for(
        chrono::milliseconds(150)
    );

    bool firstRequest = source.request_stop();

    cout << boolalpha;
    cout << "first request_stop returned "
         << firstRequest
         << endl;

    bool secondRequest = source.request_stop();

    cout << "second request_stop returned "
         << secondRequest
         << endl;
}

void stopCallbackDemo() {
    cout << "\n=== stop_callback demo ===" << endl;

    jthread worker([](stop_token token) {
        stop_callback callback(token, [] {
            cout << "stop callback executed on thread "
                 << this_thread::get_id()
                 << endl;
        });

        while (!token.stop_requested()) {
            this_thread::sleep_for(
                chrono::milliseconds(50)
            );
        }

        cout << "worker exiting\n";
    });

    this_thread::sleep_for(
        chrono::milliseconds(100)
    );

    cout << "requester thread id = "
         << this_thread::get_id()
         << endl;

    worker.request_stop();
}

class CancellableQueueWorker {
private:
    queue<int> tasks_;
    mutex mutex_;
    condition_variable cv_;
    jthread worker_;

    void run(stop_token token) {
        stop_callback callback(token, [&] {
            cv_.notify_all();
        });

        while (true) {
            int task = 0;

            {
                unique_lock<mutex> lock(mutex_);

                cv_.wait(lock, [&] {
                    return token.stop_requested()
                        || !tasks_.empty();
                });

                if (
                    token.stop_requested()
                    && tasks_.empty()
                ) {
                    cout << "queue worker stopping\n";
                    return;
                }

                task = tasks_.front();
                tasks_.pop();
            }

            cout << "processing task "
                 << task
                 << endl;

            this_thread::sleep_for(
                chrono::milliseconds(100)
            );
        }
    }

public:
    CancellableQueueWorker()
        : worker_([this](stop_token token) {
            run(token);
        }) {}

    ~CancellableQueueWorker() {
        worker_.request_stop();
        cv_.notify_all();
    }

    void submit(int task) {
        {
            lock_guard<mutex> lock(mutex_);
            tasks_.push(task);
        }

        cv_.notify_one();
    }

    void requestStop() {
        worker_.request_stop();
        cv_.notify_all();
    }
};

void cancellableQueueDemo() {
    cout << "\n=== cancellable queue worker demo ===" << endl;

    CancellableQueueWorker worker;

    worker.submit(1);
    worker.submit(2);
    worker.submit(3);

    this_thread::sleep_for(
        chrono::milliseconds(180)
    );

    cout << "main requests queue worker stop\n";

    worker.requestStop();
}

void cancellationGranularityDemo() {
    cout << "\n=== cancellation granularity demo ===" << endl;

    jthread worker([](stop_token token) {
        for (int chunk = 0; chunk < 100; ++chunk) {
            if (token.stop_requested()) {
                cout << "stopped before chunk "
                     << chunk
                     << endl;
                return;
            }

            this_thread::sleep_for(
                chrono::milliseconds(20)
            );
        }

        cout << "all chunks completed\n";
    });

    this_thread::sleep_for(
        chrono::milliseconds(130)
    );

    worker.request_stop();
}

void ignoredStopWarningDemo() {
    cout << "\n=== ignored stop warning ===" << endl;

    cout << "If a jthread worker never checks its stop_token,\n";
    cout << "request_stop does not force it to terminate.\n";

    cout << "The jthread destructor may block forever waiting "
            "for that worker to return.\n";
}

int main() {
    basicJthreadDemo();

    stopTokenLoopDemo();

    automaticDestructorStopDemo();

    stopSourceDemo();

    stopCallbackDemo();

    cancellableQueueDemo();

    cancellationGranularityDemo();

    ignoredStopWarningDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}