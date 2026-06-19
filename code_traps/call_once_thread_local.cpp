#include <chrono>
#include <iostream>
#include <mutex>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
using namespace std;

/*
 * Topic:
 * std::once_flag, std::call_once, and thread_local.
 *
 * Compile:
 * g++ -std=c++20 -pthread call_once_thread_local.cpp \
 *     -o call_once_thread_local
 *
 * Run:
 * ./call_once_thread_local
 */

once_flag initializationFlag;

void initializeService() {
    call_once(initializationFlag, [] {
        cout << "service initialization executed by thread "
             << this_thread::get_id()
             << endl;

        this_thread::sleep_for(
            chrono::milliseconds(100)
        );
    });
}

void callOnceBasicDemo() {
    cout << "\n=== call_once basic demo ===" << endl;

    vector<thread> threads;

    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([] {
            initializeService();

            cout << "thread "
                 << this_thread::get_id()
                 << " returned from initializeService\n";
        });
    }

    for (thread& t : threads) {
        t.join();
    }
}

void callOnceExceptionDemo() {
    cout << "\n=== call_once exception retry demo ===" << endl;

    once_flag flag;
    mutex outputMutex;
    int attempts = 0;

    auto initialize = [&] {
        try {
            call_once(flag, [&] {
                int currentAttempt;

                {
                    lock_guard<mutex> lock(outputMutex);
                    currentAttempt = ++attempts;

                    cout << "initialization attempt "
                         << currentAttempt
                         << endl;
                }

                if (currentAttempt == 1) {
                    throw runtime_error(
                        "first initialization failed"
                    );
                }

                cout << "initialization succeeded\n";
            });
        } catch (const exception& e) {
            lock_guard<mutex> lock(outputMutex);

            cout << "thread caught: "
                 << e.what()
                 << endl;
        }
    };

    thread t1(initialize);
    t1.join();

    thread t2(initialize);
    t2.join();

    thread t3(initialize);
    t3.join();

    cout << "total attempts = "
         << attempts
         << endl;
}

class Config {
public:
    Config() {
        cout << "Config constructed once by thread "
             << this_thread::get_id()
             << endl;
    }

    int value = 42;
};

Config& getConfig() {
    static Config config;
    return config;
}

void localStaticDemo() {
    cout << "\n=== function-local static demo ===" << endl;

    vector<thread> threads;

    for (int i = 0; i < 4; ++i) {
        threads.emplace_back([] {
            Config& config = getConfig();

            cout << "thread "
                 << this_thread::get_id()
                 << " sees config value "
                 << config.value
                 << endl;
        });
    }

    for (thread& t : threads) {
        t.join();
    }
}

thread_local int perThreadCounter = 0;

void incrementPerThreadCounter(int threadNumber) {
    for (int i = 0; i < 3; ++i) {
        ++perThreadCounter;

        cout << "logical thread "
             << threadNumber
             << ", system thread "
             << this_thread::get_id()
             << ", local counter = "
             << perThreadCounter
             << endl;
    }
}

void threadLocalCounterDemo() {
    cout << "\n=== thread_local counter demo ===" << endl;

    thread t1(incrementPerThreadCounter, 1);
    thread t2(incrementPerThreadCounter, 2);

    t1.join();
    t2.join();

    cout << "main thread counter = "
         << perThreadCounter
         << endl;
}

void functionLocalThreadLocalDemo() {
    cout << "\n=== function-local thread_local demo ===" << endl;

    auto function = [] {
        thread_local int callCount = 0;

        ++callCount;

        cout << "thread "
             << this_thread::get_id()
             << ", function call count = "
             << callCount
             << endl;
    };

    thread t1([&] {
        function();
        function();
        function();
    });

    thread t2([&] {
        function();
        function();
    });

    t1.join();
    t2.join();
}

thread_local string requestId;

class RequestContextGuard {
private:
    string previous_;

public:
    explicit RequestContextGuard(string newRequestId)
        : previous_(std::move(requestId)) {
        requestId = std::move(newRequestId);
    }

    ~RequestContextGuard() {
        requestId = std::move(previous_);
    }

    RequestContextGuard(const RequestContextGuard&) = delete;
    RequestContextGuard& operator=(
        const RequestContextGuard&
    ) = delete;
};

void processRequest(const string& id) {
    RequestContextGuard guard(id);

    cout << "processing request "
         << requestId
         << " on thread "
         << this_thread::get_id()
         << endl;

    this_thread::sleep_for(
        chrono::milliseconds(50)
    );
}

void threadLocalContextDemo() {
    cout << "\n=== thread-local request context demo ===" << endl;

    thread worker([] {
        processRequest("request-A");

        cout << "after request A, context = ["
             << requestId
             << "]\n";

        processRequest("request-B");

        cout << "after request B, context = ["
             << requestId
             << "]\n";
    });

    worker.join();
}

void perThreadBufferDemo() {
    cout << "\n=== per-thread scratch buffer demo ===" << endl;

    auto format = [](int value) -> const string& {
        thread_local string buffer;

        buffer = "formatted-" + to_string(value);

        return buffer;
    };

    thread t1([&] {
        cout << "thread 1: "
             << format(100)
             << endl;
    });

    thread t2([&] {
        cout << "thread 2: "
             << format(200)
             << endl;
    });

    t1.join();
    t2.join();

    cout << "Each thread used its own string buffer.\n";
}

void threadPoolWarningDemo() {
    cout << "\n=== thread-pool thread_local warning ===" << endl;

    cout << "A worker thread may execute many unrelated tasks.\n";
    cout << "Its thread_local state survives between those tasks.\n";
    cout << "Treat thread_local as per-thread, not per-request.\n";
}

int main() {
    callOnceBasicDemo();

    callOnceExceptionDemo();

    localStaticDemo();

    threadLocalCounterDemo();

    functionLocalThreadLocalDemo();

    threadLocalContextDemo();

    perThreadBufferDemo();

    threadPoolWarningDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}