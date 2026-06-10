#include <cstdio>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
#include <utility>
using namespace std;

/*
 * Topic:
 * RAII, exception safety, scope guard.
 *
 * Compile:
 * g++ -std=c++17 raii_exception_safety.cpp -o raii_exception_safety
 *
 * Run:
 * ./raii_exception_safety
 */

class Tracker {
private:
    string name;

public:
    explicit Tracker(string n)
        : name(std::move(n)) {
        cout << "Tracker constructor: " << name << endl;
    }

    ~Tracker() {
        cout << "Tracker destructor: " << name << endl;
    }
};

void stackUnwindingDemo() {
    cout << "\n=== stack unwinding demo ===" << endl;

    try {
        Tracker a("A");
        Tracker b("B");

        cout << "throwing exception now\n";
        throw runtime_error("error");
    } catch (const exception& e) {
        cout << "caught exception: " << e.what() << endl;
    }
}

class File {
private:
    FILE* fp = nullptr;

public:
    explicit File(const char* path)
        : fp(fopen(path, "w")) {
        if (!fp) {
            throw runtime_error("failed to open file");
        }

        cout << "File opened\n";
    }

    ~File() {
        if (fp) {
            cout << "File closed\n";
            fclose(fp);
        }
    }

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    File(File&& other) noexcept
        : fp(other.fp) {
        other.fp = nullptr;
        cout << "File move constructor\n";
    }

    File& operator=(File&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        if (fp) {
            fclose(fp);
        }

        fp = other.fp;
        other.fp = nullptr;

        cout << "File move assignment\n";

        return *this;
    }

    void write(const char* text) {
        fputs(text, fp);
    }
};

void fileRaiiDemo() {
    cout << "\n=== file RAII demo ===" << endl;

    try {
        File f("raii_demo_output.txt");

        f.write("hello RAII\n");

        throw runtime_error("something failed after opening file");
    } catch (const exception& e) {
        cout << "caught exception: " << e.what() << endl;
    }

    cout << "File was still closed automatically." << endl;
}

void uniquePtrRaiiDemo() {
    cout << "\n=== unique_ptr RAII demo ===" << endl;

    try {
        auto p = make_unique<Tracker>("heap_tracker");

        cout << "about to throw\n";
        throw runtime_error("error after allocation");
    } catch (const exception& e) {
        cout << "caught exception: " << e.what() << endl;
    }

    cout << "unique_ptr cleaned up automatically." << endl;
}

void lockGuardDemo() {
    cout << "\n=== lock_guard RAII demo ===" << endl;

    mutex m;

    try {
        lock_guard<mutex> lock(m);

        cout << "mutex locked inside scope\n";

        throw runtime_error("error while holding mutex");
    } catch (const exception& e) {
        cout << "caught exception: " << e.what() << endl;
    }

    cout << "mutex was unlocked when lock_guard was destroyed." << endl;
}

template <typename F>
class ScopeGuard {
private:
    F func_;
    bool active_ = true;

public:
    explicit ScopeGuard(F func)
        : func_(std::move(func)) {}

    ~ScopeGuard() {
        if (active_) {
            func_();
        }
    }

    void dismiss() {
        active_ = false;
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    ScopeGuard(ScopeGuard&& other) noexcept
        : func_(std::move(other.func_)),
          active_(other.active_) {
        other.active_ = false;
    }

    ScopeGuard& operator=(ScopeGuard&&) = delete;
};

template <typename F>
ScopeGuard<F> makeScopeGuard(F f) {
    return ScopeGuard<F>(std::move(f));
}

void scopeGuardSuccessDemo() {
    cout << "\n=== scope guard success demo ===" << endl;

    bool committed = false;

    auto guard = makeScopeGuard([&] {
        if (!committed) {
            cout << "rollback\n";
        }
    });

    cout << "doing work\n";

    committed = true;
    cout << "commit\n";

    guard.dismiss();
}

void scopeGuardExceptionDemo() {
    cout << "\n=== scope guard exception demo ===" << endl;

    try {
        bool committed = false;

        auto guard = makeScopeGuard([&] {
            if (!committed) {
                cout << "rollback due to exception\n";
            }
        });

        cout << "doing work\n";

        throw runtime_error("work failed");

        committed = true;
        guard.dismiss();
    } catch (const exception& e) {
        cout << "caught exception: " << e.what() << endl;
    }
}

class BadManualResource {
private:
    Tracker* ptr = nullptr;

public:
    BadManualResource()
        : ptr(new Tracker("manual_resource")) {}

    ~BadManualResource() {
        delete ptr;
    }

    // This class is intentionally incomplete:
    // compiler-generated copy would shallow-copy ptr and cause double delete.
};

void manualResourceWarningDemo() {
    cout << "\n=== manual resource warning demo ===" << endl;

    cout << "If a class owns raw pointer, it must define copy/move/destructor correctly." << endl;
    cout << "Prefer unique_ptr member to get Rule of Zero." << endl;
}

class GoodResourceOwner {
private:
    unique_ptr<Tracker> ptr;

public:
    GoodResourceOwner()
        : ptr(make_unique<Tracker>("rule_of_zero_resource")) {}

    // No destructor/copy/move needed manually.
    // unique_ptr makes this move-only automatically.
};

void ruleOfZeroDemo() {
    cout << "\n=== rule of zero demo ===" << endl;

    GoodResourceOwner owner;

    cout << "GoodResourceOwner relies on unique_ptr cleanup." << endl;
}

int main() {
    stackUnwindingDemo();

    fileRaiiDemo();

    uniquePtrRaiiDemo();

    lockGuardDemo();

    scopeGuardSuccessDemo();

    scopeGuardExceptionDemo();

    manualResourceWarningDemo();

    ruleOfZeroDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}