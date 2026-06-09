#include <array>
#include <iostream>
#include <span>
#include <vector>
using namespace std;

/*
 * Topic:
 * std::span, C array, std::array, std::vector.
 *
 * Compile:
 * g++ -std=c++20 span_array_vector_view.cpp -o span_array_vector_view
 *
 * Run:
 * ./span_array_vector_view
 */

void printSpan(span<const int> values) {
    cout << "values: ";
    for (int x : values) {
        cout << x << " ";
    }
    cout << ", size = " << values.size() << endl;
}

void incrementAll(span<int> values) {
    for (int& x : values) {
        ++x;
    }
}

void oldStylePrint(const int* data, size_t size) {
    cout << "old style: ";
    for (size_t i = 0; i < size; ++i) {
        cout << data[i] << " ";
    }
    cout << endl;
}

void cArrayDecayDemo() {
    cout << "\n=== C array decay demo ===" << endl;

    int arr[3] = {1, 2, 3};

    cout << "sizeof(arr) / sizeof(arr[0]) = "
         << sizeof(arr) / sizeof(arr[0]) << endl;

    oldStylePrint(arr, 3);

    cout << "When passed to a function, C array decays to int* and size is lost." << endl;
}

void spanAcceptsManyDemo() {
    cout << "\n=== span accepts vector/array/raw array demo ===" << endl;

    vector<int> v = {1, 2, 3};
    array<int, 3> a = {4, 5, 6};
    int raw[3] = {7, 8, 9};

    printSpan(v);
    printSpan(a);
    printSpan(raw);
}

void mutableSpanDemo() {
    cout << "\n=== mutable span demo ===" << endl;

    vector<int> v = {1, 2, 3};

    printSpan(v);

    incrementAll(v);

    printSpan(v);

    cout << "span<int> modifies the underlying vector elements." << endl;
}

void spanSubspanDemo() {
    cout << "\n=== subspan demo ===" << endl;

    vector<int> v = {10, 20, 30, 40, 50};

    span<int> s = v;

    auto mid = s.subspan(1, 3);

    printSpan(mid);

    mid[0] = 200;

    cout << "after modifying mid[0], original vector: ";
    printSpan(v);
}

void spanDanglingVectorReallocationDemo() {
    cout << "\n=== span dangling after vector reallocation demo ===" << endl;

    vector<int> v;
    v.reserve(3);
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    span<int> s = v;

    const int* oldData = s.data();

    cout << "old span data = " << static_cast<const void*>(oldData) << endl;

    v.push_back(4); // may reallocate

    cout << "new vector data = " << static_cast<const void*>(v.data()) << endl;

    if (oldData != v.data()) {
        cout << "vector reallocated; old span is dangling and must not be used." << endl;
    } else {
        cout << "no reallocation; span still valid." << endl;
    }
}

void constSpanDemo() {
    cout << "\n=== span<const T> vs const span<T> demo ===" << endl;

    vector<int> v = {1, 2, 3};

    span<const int> readOnlyView = v;
    cout << "readOnlyView[0] = " << readOnlyView[0] << endl;
    // readOnlyView[0] = 10; // error

    const span<int> constViewObject = v;
    constViewObject[0] = 10; // OK, span object is const but elements are mutable

    printSpan(v);

    cout << "span<const int> protects elements." << endl;
    cout << "const span<int> protects the span handle, not the elements." << endl;
}

void staticExtentDemo() {
    cout << "\n=== static extent span demo ===" << endl;

    array<int, 3> a = {1, 2, 3};

    span<int, 3> s = a;

    cout << "static extent size = " << s.size() << endl;

    printSpan(s);
}

int main() {
    cArrayDecayDemo();

    spanAcceptsManyDemo();

    mutableSpanDemo();

    spanSubspanDemo();

    spanDanglingVectorReallocationDemo();

    constSpanDemo();

    staticExtentDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}