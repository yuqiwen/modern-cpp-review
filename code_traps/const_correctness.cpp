#include <iostream>
#include <mutex>
#include <string>
#include <utility>
#include <vector>
using namespace std;

/*
 * Topic:
 * Const correctness.
 *
 * Compile:
 * g++ -std=c++17 const_correctness.cpp -o const_correctness
 *
 * Run:
 * ./const_correctness
 */

class User {
private:
    string name;
    mutable int accessCount = 0;

public:
    explicit User(string n)
        : name(std::move(n)) {}

    // Const member function.
    // It can be called on const User objects.
    const string& getName() const {
        accessCount++;
        return name;
    }

    // Non-const member function.
    // It cannot be called on const User objects.
    void setName(const string& newName) {
        name = newName;
    }

    int getAccessCount() const {
        return accessCount;
    }
};

class VectorLike {
private:
    vector<int> data;

public:
    explicit VectorLike(size_t n)
        : data(n, 0) {}

    // Non-const version returns mutable reference.
    int& at(size_t i) {
        cout << "non-const at()\n";
        return data[i];
    }

    // Const version returns const reference.
    const int& at(size_t i) const {
        cout << "const at()\n";
        return data[i];
    }
};

class PointerWrapper {
private:
    int* ptr;

public:
    explicit PointerWrapper(int* p)
        : ptr(p) {}

    void modifyPointeeInConstFunction(int value) const {
        // This is allowed because constness of this object makes ptr itself const,
        // but it does not make the pointed-to int const.
        *ptr = value;
    }

    // This would be illegal in a const member function:
    // void reseatPointer(int* p) const {
    //     ptr = p; // error
    // }
};

class ThreadSafeCounter {
private:
    int value = 0;
    mutable mutex m;

public:
    void increment() {
        lock_guard<mutex> lock(m);
        value++;
    }

    int get() const {
        // get() is logically const, but locking a mutex modifies mutex state.
        // Therefore the mutex member must be mutable.
        lock_guard<mutex> lock(m);
        return value;
    }
};

class Tracker {
private:
    string name;

public:
    explicit Tracker(string n)
        : name(std::move(n)) {
        cout << "Constructor: " << name << endl;
    }

    Tracker(const Tracker& other)
        : name(other.name + "_copy") {
        cout << "Copy constructor from " << other.name << endl;
    }

    Tracker(Tracker&& other) noexcept
        : name(std::move(other.name)) {
        cout << "Move constructor\n";
        other.name = "moved_from";
    }

    const string& getName() const {
        return name;
    }
};

void pointerConstDemo() {
    cout << "\n=== pointer constness demo ===" << endl;

    int a = 10;
    int b = 20;

    const int* pointerToConst = &a;
    // *pointerToConst = 30; // error
    pointerToConst = &b;     // OK

    int* const constPointer = &a;
    *constPointer = 30;      // OK
    // constPointer = &b;    // error

    const int* const constPointerToConst = &a;
    // *constPointerToConst = 40; // error
    // constPointerToConst = &b;  // error

    cout << "a = " << a << ", b = " << b << endl;
}

void constMemberFunctionDemo() {
    cout << "\n=== const member function demo ===" << endl;

    User u("Yuqi");
    cout << u.getName() << endl;
    u.setName("NewName");
    cout << u.getName() << endl;

    const User constUser("ConstUser");
    cout << constUser.getName() << endl;
    // constUser.setName("Fail"); // error

    cout << "constUser access count = " << constUser.getAccessCount() << endl;
}

void constOverloadDemo() {
    cout << "\n=== const overload demo ===" << endl;

    VectorLike v(3);
    v.at(0) = 42; // calls non-const version

    const VectorLike cv(3);
    cout << cv.at(0) << endl; // calls const version
}

void shallowConstDemo() {
    cout << "\n=== shallow const demo ===" << endl;

    int x = 10;
    const PointerWrapper wrapper(&x);

    wrapper.modifyPointeeInConstFunction(99);

    cout << "x = " << x << endl;
    cout << "A const object with pointer member may still modify the pointee." << endl;
}

void mutableMutexDemo() {
    cout << "\n=== mutable mutex demo ===" << endl;

    ThreadSafeCounter counter;
    counter.increment();

    cout << "counter = " << counter.get() << endl;
}

void moveConstDemo() {
    cout << "\n=== std::move on const object demo ===" << endl;

    const Tracker a("const_tracker");

    Tracker b = std::move(a); // calls copy constructor, not move constructor

    cout << "b name = " << b.getName() << endl;
}

int main() {
    pointerConstDemo();

    constMemberFunctionDemo();

    constOverloadDemo();

    shallowConstDemo();

    mutableMutexDemo();

    moveConstDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}