#include <iostream>
#include <string>
#include <type_traits>
#include <vector>
using namespace std;

/*
 * Topic:
 * Template basics.
 *
 * Compile:
 * g++ -std=c++17 template_basics.cpp -o template_basics
 *
 * Run:
 * ./template_basics
 */

template <typename T>
T maxValue(T a, T b) {
    return a > b ? a : b;
}

template <typename T, typename U>
auto addTwoTypes(T a, U b) {
    return a + b;
}

template <typename T>
class Box {
private:
    T value;

public:
    explicit Box(const T& v)
        : value(v) {}

    const T& get() const {
        return value;
    }

    void set(const T& v) {
        value = v;
    }
};

template <typename T, size_t N>
class SimpleArray {
private:
    T data[N];

public:
    size_t size() const {
        return N;
    }

    T& operator[](size_t i) {
        return data[i];
    }

    const T& operator[](size_t i) const {
        return data[i];
    }
};

template <typename T>
class TypeName {
public:
    static string name() {
        return "unknown";
    }
};

template <>
class TypeName<int> {
public:
    static string name() {
        return "int";
    }
};

template <>
class TypeName<string> {
public:
    static string name() {
        return "std::string";
    }
};

template <typename T>
class IsPointer {
public:
    static constexpr bool value = false;
};

template <typename T>
class IsPointer<T*> {
public:
    static constexpr bool value = true;
};

template <typename Container>
void printFirst(const Container& c) {
    if (c.begin() == c.end()) {
        cout << "empty container" << endl;
        return;
    }

    typename Container::const_iterator it = c.begin();

    cout << "first element = " << *it << endl;
}

template <typename Container>
void printAll(const Container& c) {
    cout << "all elements: ";

    for (auto it = c.begin(); it != c.end(); ++it) {
        cout << *it << " ";
    }

    cout << endl;
}

template <typename T>
void printGeneric(const T& x) {
    cout << "generic print: " << x << endl;
}

void printGeneric(int x) {
    cout << "int overload print: " << x << endl;
}

struct NoCompare {
    int value;
};

void functionTemplateDemo() {
    cout << "\n=== function template demo ===" << endl;

    cout << "maxValue<int>(3, 5) = " << maxValue(3, 5) << endl;
    cout << "maxValue<double>(2.5, 1.5) = " << maxValue(2.5, 1.5) << endl;

    // maxValue(1, 2.5); // error: T cannot be both int and double

    cout << "addTwoTypes(1, 2.5) = " << addTwoTypes(1, 2.5) << endl;
}

void classTemplateDemo() {
    cout << "\n=== class template demo ===" << endl;

    Box<int> intBox(10);
    Box<string> stringBox("hello");

    cout << "intBox = " << intBox.get() << endl;
    cout << "stringBox = " << stringBox.get() << endl;
}

void nonTypeTemplateParamDemo() {
    cout << "\n=== non-type template parameter demo ===" << endl;

    SimpleArray<int, 3> arr;
    arr[0] = 10;
    arr[1] = 20;
    arr[2] = 30;

    cout << "arr size = " << arr.size() << endl;
    cout << "arr[1] = " << arr[1] << endl;
}

void specializationDemo() {
    cout << "\n=== specialization demo ===" << endl;

    cout << "TypeName<double> = " << TypeName<double>::name() << endl;
    cout << "TypeName<int> = " << TypeName<int>::name() << endl;
    cout << "TypeName<string> = " << TypeName<string>::name() << endl;

    cout << boolalpha;
    cout << "IsPointer<int>::value = " << IsPointer<int>::value << endl;
    cout << "IsPointer<int*>::value = " << IsPointer<int*>::value << endl;
}

void dependentNameDemo() {
    cout << "\n=== dependent name demo ===" << endl;

    vector<int> v = {1, 2, 3};

    printFirst(v);
    printAll(v);
}

void overloadDemo() {
    cout << "\n=== template overload demo ===" << endl;

    printGeneric(10);
    printGeneric(3.14);
    printGeneric(string("hello"));
}

void implicitRequirementDemo() {
    cout << "\n=== implicit template requirement demo ===" << endl;

    cout << "maxValue works only if T supports operator>" << endl;

    NoCompare a{1};
    NoCompare b{2};

    (void)a;
    (void)b;

    // maxValue(a, b); // error: NoCompare has no operator>
}

int main() {
    functionTemplateDemo();

    classTemplateDemo();

    nonTypeTemplateParamDemo();

    specializationDemo();

    dependentNameDemo();

    overloadDemo();

    implicitRequirementDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}