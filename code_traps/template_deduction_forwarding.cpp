#include <iostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
using namespace std;

/*
 * Topic:
 * Template type deduction, forwarding references, and std::forward.
 *
 * Compile:
 * g++ -std=c++17 template_deduction_forwarding.cpp -o template_deduction_forwarding
 *
 * Run:
 * ./template_deduction_forwarding
 */

void process(const string& s) {
    cout << "process(const string&): lvalue/read-only, value = " << s << endl;
}

void process(string&& s) {
    cout << "process(string&&): rvalue/movable, value = " << s << endl;
}

template <typename T>
void badWrapperNoForward(T&& x) {
    cout << "badWrapperNoForward -> ";
    process(x); // x is named, so x is an lvalue expression
}

template <typename T>
void badWrapperMove(T&& x) {
    cout << "badWrapperMove -> ";
    process(std::move(x)); // always rvalue, even if caller passed lvalue
}

template <typename T>
void goodWrapperForward(T&& x) {
    cout << "goodWrapperForward -> ";
    process(std::forward<T>(x)); // preserves original value category
}

void normalRvalueReference(string&& s) {
    cout << "normalRvalueReference received: " << s << endl;
}

template <typename T>
void inspectForwardingReference(T&& x) {
    cout << "inspectForwardingReference: ";

    if constexpr (is_lvalue_reference_v<T>) {
        cout << "T is lvalue reference; ";
    } else {
        cout << "T is non-reference type; ";
    }

    if constexpr (is_lvalue_reference_v<decltype(x)>) {
        cout << "decltype(x) is lvalue reference";
    } else if constexpr (is_rvalue_reference_v<decltype(x)>) {
        cout << "decltype(x) is rvalue reference";
    }

    cout << endl;
}

class Tracker {
private:
    string name;

public:
    explicit Tracker(string n)
        : name(std::move(n)) {
        cout << "Tracker constructor: " << name << endl;
    }

    Tracker(const Tracker& other)
        : name(other.name + "_copy") {
        cout << "Tracker copy constructor from " << other.name << endl;
    }

    Tracker(Tracker&& other) noexcept
        : name(std::move(other.name)) {
        cout << "Tracker move constructor" << endl;
        other.name = "moved_from";
    }
};

template <typename T, typename... Args>
T makeObject(Args&&... args) {
    return T(std::forward<Args>(args)...);
}

template <typename T>
class Box {
private:
    T value;

public:
    explicit Box(T v)
        : value(std::move(v)) {}

    // This is NOT a forwarding reference.
    // T is already fixed by Box<T>.
    void setRvalueOnly(T&& x) {
        value = std::move(x);
    }

    // This is a forwarding reference because U is deduced by this function.
    template <typename U>
    void setForwarding(U&& x) {
        value = std::forward<U>(x);
    }

    const T& get() const {
        return value;
    }
};

void wrapperDemo() {
    cout << "\n=== wrapper forwarding demo ===" << endl;

    string s = "hello";

    badWrapperNoForward(s);
    badWrapperNoForward(string("temporary"));

    goodWrapperForward(s);
    goodWrapperForward(string("temporary"));

    cout << "\nNow badWrapperMove with lvalue:" << endl;
    string name = "Yuqi";
    badWrapperMove(name);
    cout << "After badWrapperMove(name), name may be moved-from: " << name << endl;
}

void inspectDemo() {
    cout << "\n=== inspect forwarding reference demo ===" << endl;

    string s = "hello";

    inspectForwardingReference(s);
    inspectForwardingReference(string("temporary"));
}

void normalRvalueReferenceDemo() {
    cout << "\n=== normal rvalue reference demo ===" << endl;

    string s = "hello";

    // normalRvalueReference(s); // error: s is lvalue

    normalRvalueReference(std::move(s));
}

void emplaceLikeDemo() {
    cout << "\n=== perfect forwarding factory demo ===" << endl;

    Tracker t = makeObject<Tracker>("from factory");
}

void boxMemberFunctionDemo() {
    cout << "\n=== class template member function demo ===" << endl;

    Box<string> box("initial");

    string s = "lvalue string";

    // box.setRvalueOnly(s); // error: setRvalueOnly expects string&&

    box.setRvalueOnly(string("rvalue string"));
    cout << "box = " << box.get() << endl;

    box.setForwarding(s);
    cout << "box = " << box.get() << endl;

    box.setForwarding(string("temporary forwarded"));
    cout << "box = " << box.get() << endl;
}

void vectorEmplaceDemo() {
    cout << "\n=== vector emplace_back demo ===" << endl;

    vector<Tracker> v;
    v.reserve(2);

    v.emplace_back("direct construction");
    v.push_back(Tracker("temporary then move"));
}

int main() {
    wrapperDemo();

    inspectDemo();

    normalRvalueReferenceDemo();

    emplaceLikeDemo();

    boxMemberFunctionDemo();

    vectorEmplaceDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}