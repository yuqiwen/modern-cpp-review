#include <chrono>
#include <future>
#include <iostream>
#include <stdexcept>
#include <string>
#include <thread>
#include <utility>
#include <vector>
using namespace std;

/*
 * Topic:
 * std::future, std::promise, std::async, std::packaged_task.
 *
 * Compile:
 * g++ -std=c++20 -pthread future_promise_async.cpp \
 *     -o future_promise_async
 *
 * Run:
 * ./future_promise_async
 */

void promiseWorker(promise<int> resultPromise, int x) {
    this_thread::sleep_for(chrono::milliseconds(100));

    resultPromise.set_value(x * 2);
}

void basicPromiseFutureDemo() {
    cout << "\n=== basic promise/future demo ===" << endl;

    promise<int> resultPromise;
    future<int> resultFuture = resultPromise.get_future();

    thread worker(
        promiseWorker,
        std::move(resultPromise),
        21
    );

    cout << "main waiting for result\n";

    int result = resultFuture.get();

    cout << "result = " << result << endl;

    worker.join();
}

void promiseExceptionWorker(promise<int> resultPromise) {
    try {
        throw runtime_error("worker failed");
    } catch (...) {
        resultPromise.set_exception(current_exception());
    }
}

void promiseExceptionDemo() {
    cout << "\n=== promise exception demo ===" << endl;

    promise<int> resultPromise;
    future<int> resultFuture = resultPromise.get_future();

    thread worker(
        promiseExceptionWorker,
        std::move(resultPromise)
    );

    try {
        int result = resultFuture.get();
        cout << result << endl;
    } catch (const exception& e) {
        cout << "main caught worker exception: "
             << e.what() << endl;
    }

    worker.join();
}

void brokenPromiseDemo() {
    cout << "\n=== broken promise demo ===" << endl;

    future<int> resultFuture;

    {
        promise<int> resultPromise;
        resultFuture = resultPromise.get_future();

        // promise is destroyed without set_value or set_exception
    }

    try {
        cout << resultFuture.get() << endl;
    } catch (const future_error& e) {
        cout << "future_error from broken promise: "
             << e.what() << endl;
    }
}

int calculate(int x) {
    this_thread::sleep_for(chrono::milliseconds(100));
    return x * x;
}

void asyncDemo() {
    cout << "\n=== async demo ===" << endl;

    future<int> resultFuture = async(
        launch::async,
        calculate,
        12
    );

    cout << "main doing other work\n";

    cout << "async result = "
         << resultFuture.get()
         << endl;
}

void deferredDemo() {
    cout << "\n=== deferred demo ===" << endl;

    cout << "main thread id = "
         << this_thread::get_id()
         << endl;

    future<thread::id> resultFuture = async(
        launch::deferred,
        [] {
            cout << "deferred callable running now\n";
            return this_thread::get_id();
        }
    );

    cout << "callable has not necessarily run yet\n";

    thread::id executionThread = resultFuture.get();

    cout << "deferred callable thread id = "
         << executionThread
         << endl;
}

void asyncExceptionDemo() {
    cout << "\n=== async exception demo ===" << endl;

    future<int> resultFuture = async(
        launch::async,
        []() -> int {
            throw runtime_error("async task failed");
        }
    );

    try {
        cout << resultFuture.get() << endl;
    } catch (const exception& e) {
        cout << "caught from future.get(): "
             << e.what() << endl;
    }
}

void futureValidDemo() {
    cout << "\n=== future valid/get once demo ===" << endl;

    future<int> resultFuture = async(
        launch::async,
        [] {
            return 42;
        }
    );

    cout << "valid before get = "
         << resultFuture.valid()
         << endl;

    int result = resultFuture.get();

    cout << "result = " << result << endl;

    cout << "valid after get = "
         << resultFuture.valid()
         << endl;

    // resultFuture.get(); // wrong: no longer valid
}

void sharedFutureDemo() {
    cout << "\n=== shared_future demo ===" << endl;

    promise<int> resultPromise;

    shared_future<int> resultFuture =
        resultPromise.get_future().share();

    thread consumer1([resultFuture] {
        cout << "consumer 1 result = "
             << resultFuture.get()
             << endl;
    });

    thread consumer2([resultFuture] {
        cout << "consumer 2 result = "
             << resultFuture.get()
             << endl;
    });

    this_thread::sleep_for(chrono::milliseconds(100));

    resultPromise.set_value(99);

    consumer1.join();
    consumer2.join();
}

void packagedTaskDemo() {
    cout << "\n=== packaged_task demo ===" << endl;

    packaged_task<int(int, int)> task(
        [](int a, int b) {
            return a + b;
        }
    );

    future<int> resultFuture = task.get_future();

    thread worker(
        std::move(task),
        10,
        20
    );

    cout << "packaged_task result = "
         << resultFuture.get()
         << endl;

    worker.join();
}

void waitForDemo() {
    cout << "\n=== wait_for demo ===" << endl;

    future<int> resultFuture = async(
        launch::async,
        [] {
            this_thread::sleep_for(
                chrono::milliseconds(200)
            );

            return 7;
        }
    );

    future_status status =
        resultFuture.wait_for(
            chrono::milliseconds(50)
        );

    if (status == future_status::ready) {
        cout << "result ready\n";
    } else if (status == future_status::timeout) {
        cout << "timeout: result not ready yet\n";
    } else {
        cout << "deferred\n";
    }

    cout << "eventual result = "
         << resultFuture.get()
         << endl;
}

void asyncTemporaryWarningDemo() {
    cout << "\n=== async temporary warning demo ===" << endl;

    auto start = chrono::steady_clock::now();

    {
        future<void> first = async(
            launch::async,
            [] {
                this_thread::sleep_for(
                    chrono::milliseconds(100)
                );
            }
        );

        future<void> second = async(
            launch::async,
            [] {
                this_thread::sleep_for(
                    chrono::milliseconds(100)
                );
            }
        );

        first.get();
        second.get();
    }

    auto elapsed = chrono::duration_cast<chrono::milliseconds>(
        chrono::steady_clock::now() - start
    );

    cout << "two futures retained; elapsed about "
         << elapsed.count()
         << " ms\n";

    cout << "Ignoring async futures can accidentally block "
            "when temporary futures are destroyed.\n";
}

int main() {
    basicPromiseFutureDemo();

    promiseExceptionDemo();

    brokenPromiseDemo();

    asyncDemo();

    deferredDemo();

    asyncExceptionDemo();

    futureValidDemo();

    sharedFutureDemo();

    packagedTaskDemo();

    waitForDemo();

    asyncTemporaryWarningDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}