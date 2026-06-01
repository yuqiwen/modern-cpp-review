#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>
using namespace std;

/*
 * Topic:
 * Advanced templates lite:
 * variadic templates, fold expressions, type traits, enable_if.
 *
 * Compile:
 * g++ -std=c++17 advanced_templates_lite.cpp -o advanced_templates_lite
 *
 * Run:
 * ./advanced_templates_lite
 */

template <typename... Args>
void printAll(const Args&... args) {
    ((cout << args << " "), ...);
    cout << endl;
}

template <typename... Args>
auto sum(Args... args) {
    return (args + ...);
}

template <typename... Args>
auto sumSafe(Args... args) {
    return (0 + ... + args);
}

template <typename... Args>
bool allTrue(Args... args) {
    return (args && ...);
}

template <typename... Args>
bool anyTrue(Args... args) {
    return (args || ...);
}

class User {
private:
    string name;
    int age;

public:
    User(string n, int a)
        : name(std::move(n)), age(a) {
        cout << "User constructor: " << name << ", " << age << endl;
    }

    void print() const {
        cout << "User{name=" << name << ", age=" << age << "}" << endl;
    }
};

template <typename T, typename... Args>
unique_ptr<T> makeUniqueLike(Args&&... args) {
    return unique_ptr<T>(new T(std::forward<Args>(args)...));
}

template <typename T>
void inspectType() {
    if constexpr (is_integral_v<T>) {
        cout << "integral type" << endl;
    } else if constexpr (is_floating_point_v<T>) {
        cout << "floating point type" << endl;
    } else if constexpr (is_pointer_v<T>) {
        cout << "pointer type" << endl;
    } else {
        cout << "other type" << endl;
    }
}

template <typename T>
void printValueOrPointer(T value) {
    if constexpr (is_pointer_v<T>) {
        if (value) {
            cout << "pointer points to: " << *value << endl;
        } else {
            cout << "null pointer" << endl;
        }
    } else {
        cout << "value: " << value << endl;
    }
}

template <typename T,
          typename = enable_if_t<is_integral_v<T>>>
void onlyIntegral(T value) {
    cout << "onlyIntegral: " << value << endl;
}

template <typename T>
enable_if_t<is_floating_point_v<T>, void>
onlyFloating(T value) {
    cout << "onlyFloating: " << value << endl;
}

template <typename T>
void moveOrCopyDecision() {
    cout << "Type traits for T:" << endl;
    cout << boolalpha;
    cout << "  is_copy_constructible = " << is_copy_constructible_v<T> << endl;
    cout << "  is_move_constructible = " << is_move_constructible_v<T> << endl;
    cout << "  is_nothrow_move_constructible = "
         << is_nothrow_move_constructible_v<T> << endl;

    if constexpr (is_nothrow_move_constructible_v<T> ||
                  !is_copy_constructible_v<T>) {
        cout << "  generic container would prefer move" << endl;
    } else {
        cout << "  generic container may prefer copy for exception safety" << endl;
    }
}

class ThrowingMove {
public:
    ThrowingMove() = default;
    ThrowingMove(const ThrowingMove&) {}
    ThrowingMove(ThrowingMove&&) {
        // not noexcept
    }
};

class NoCopyNoexceptMove {
public:
    NoCopyNoexceptMove() = default;
    NoCopyNoexceptMove(const NoCopyNoexceptMove&) = delete;
    NoCopyNoexceptMove(NoCopyNoexceptMove&&) noexcept {}
};

void variadicDemo() {
    cout << "\n=== variadic template demo ===" << endl;

    printAll(1, "hello", 3.14, string("world"));

    cout << "sum(1, 2, 3) = " << sum(1, 2, 3) << endl;
    cout << "sumSafe() = " << sumSafe() << endl;

    cout << boolalpha;
    cout << "allTrue(true, true, false) = "
         << allTrue(true, true, false) << endl;
    cout << "anyTrue(false, false, true) = "
         << anyTrue(false, false, true) << endl;
}

void perfectForwardingFactoryDemo() {
    cout << "\n=== perfect forwarding factory demo ===" << endl;

    auto user = makeUniqueLike<User>("Yuqi", 25);
    user->print();
}

void typeTraitsDemo() {
    cout << "\n=== type traits demo ===" << endl;

    inspectType<int>();
    inspectType<double>();
    inspectType<int*>();
    inspectType<string>();
}

void ifConstexprDemo() {
    cout << "\n=== if constexpr demo ===" << endl;

    int x = 42;
    int* p = &x;

    printValueOrPointer(x);
    printValueOrPointer(p);
}

void enableIfDemo() {
    cout << "\n=== enable_if demo ===" << endl;

    onlyIntegral(10);
    // onlyIntegral(3.14); // error: disabled for double

    onlyFloating(3.14);
    // onlyFloating(10); // error: disabled for int
}

void moveOrCopyDecisionDemo() {
    cout << "\n=== move/copy decision demo ===" << endl;

    cout << "\nint:" << endl;
    moveOrCopyDecision<int>();

    cout << "\nstring:" << endl;
    moveOrCopyDecision<string>();

    cout << "\nThrowingMove:" << endl;
    moveOrCopyDecision<ThrowingMove>();

    cout << "\nNoCopyNoexceptMove:" << endl;
    moveOrCopyDecision<NoCopyNoexceptMove>();
}

int main() {
    variadicDemo();

    perfectForwardingFactoryDemo();

    typeTraitsDemo();

    ifConstexprDemo();

    enableIfDemo();

    moveOrCopyDecisionDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}