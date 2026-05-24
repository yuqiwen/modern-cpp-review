#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>
using namespace std;

/*
 * Topic:
 * std::vector deep dive.
 *
 * Compile:
 * g++ -std=c++17 vector_deep_dive.cpp -o vector_deep_dive
 *
 * Run:
 * ./vector_deep_dive
 */

void printVectorInfo(const vector<int>& v, const string& label) {
    cout << label
         << " size = " << v.size()
         << ", capacity = " << v.capacity()
         << ", data = " << static_cast<const void*>(v.data())
         << endl;
}

void reserveVsResizeDemo() {
    cout << "\n=== reserve vs resize demo ===" << endl;

    vector<int> v;

    v.reserve(5);
    printVectorInfo(v, "after reserve(5)");

    cout << "reserve does not create elements, so v[0] is invalid here." << endl;

    v.push_back(10);
    printVectorInfo(v, "after push_back(10)");
    cout << "v[0] = " << v[0] << endl;

    v.resize(5);
    printVectorInfo(v, "after resize(5)");

    cout << "after resize, elements exist: ";
    for (int x : v) {
        cout << x << " ";
    }
    cout << endl;
}

void reallocationDemo() {
    cout << "\n=== reallocation demo ===" << endl;

    vector<int> v;
    v.reserve(3);

    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    printVectorInfo(v, "before push beyond capacity");

    int* oldPtr = &v[0];
    auto oldData = v.data();

    cout << "old &v[0] = " << static_cast<const void*>(oldPtr) << endl;

    v.push_back(4);

    printVectorInfo(v, "after push_back(4)");

    cout << "old data pointer = " << static_cast<const void*>(oldData) << endl;
    cout << "new data pointer = " << static_cast<const void*>(v.data()) << endl;

    if (oldData != v.data()) {
        cout << "Reallocation happened. oldPtr is now invalid and must not be used." << endl;
    } else {
        cout << "No reallocation happened. oldPtr still points to v[0]." << endl;
    }
}

void noReallocationDemo() {
    cout << "\n=== no reallocation demo ===" << endl;

    vector<int> v;
    v.reserve(10);

    v.push_back(1);
    v.push_back(2);

    int* p = &v[0];
    auto it = v.begin();

    printVectorInfo(v, "before push_back with enough capacity");

    v.push_back(3);

    printVectorInfo(v, "after push_back with enough capacity");

    cout << "*p = " << *p << endl;
    cout << "*it = " << *it << endl;
    cout << "Existing element pointer/iterator remained valid because no reallocation happened." << endl;
}

void eraseLoopDemo() {
    cout << "\n=== erase loop demo ===" << endl;

    vector<int> v = {1, 2, 3, 4, 5, 6};

    for (auto it = v.begin(); it != v.end(); ) {
        if (*it % 2 == 0) {
            it = v.erase(it); // returns next valid iterator
        } else {
            ++it;
        }
    }

    cout << "after removing even numbers: ";
    for (int x : v) {
        cout << x << " ";
    }
    cout << endl;
}

void eraseRemoveDemo() {
    cout << "\n=== erase-remove demo ===" << endl;

    vector<int> v = {1, 2, 3, 2, 4, 2, 5};

    v.erase(remove(v.begin(), v.end(), 2), v.end());

    cout << "after removing all 2s: ";
    for (int x : v) {
        cout << x << " ";
    }
    cout << endl;
}

class Tracker {
private:
    string name;

public:
    explicit Tracker(string n)
        : name(std::move(n)) {
        cout << "Constructor: " << name << endl;
    }

    ~Tracker() {
        cout << "Destructor: " << name << endl;
    }

    Tracker(const Tracker& other)
        : name(other.name + "_copy") {
        cout << "Copy constructor from " << other.name << endl;
    }

    Tracker(Tracker&& other) noexcept
        : name(std::move(other.name)) {
        cout << "Move constructor" << endl;
        other.name = "moved_from";
    }
};

void pushVsEmplaceDemo() {
    cout << "\n=== push_back vs emplace_back demo ===" << endl;

    vector<Tracker> v;
    v.reserve(2);

    cout << "\n--- push_back temporary ---" << endl;
    v.push_back(Tracker("push_temp"));

    cout << "\n--- emplace_back ---" << endl;
    v.emplace_back("emplace_direct");
}

void vectorUniquePtrDemo() {
    cout << "\n=== vector<unique_ptr<T>> demo ===" << endl;

    vector<unique_ptr<int>> v;

    auto p = make_unique<int>(42);

    // v.push_back(p); // error: unique_ptr is not copyable

    v.push_back(std::move(p));

    if (!p) {
        cout << "p is nullptr after moving into vector" << endl;
    }

    cout << "*v[0] = " << *v[0] << endl;

    v.push_back(make_unique<int>(100));

    cout << "vector contents: ";
    for (const auto& ptr : v) {
        cout << *ptr << " ";
    }
    cout << endl;
}

int main() {
    reserveVsResizeDemo();

    reallocationDemo();

    noReallocationDemo();

    eraseLoopDemo();

    eraseRemoveDemo();

    pushVsEmplaceDemo();

    vectorUniquePtrDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}