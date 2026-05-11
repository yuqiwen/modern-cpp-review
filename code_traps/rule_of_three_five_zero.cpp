#include <algorithm>
#include <iostream>
#include <utility>
#include <vector>
using namespace std;

/*
 * Topic:
 * Rule of Three, Rule of Five, and Rule of Zero.
 *
 * Compile:
 * g++ -std=c++17 rule_of_three_five_zero.cpp -o rule_of_three_five_zero
 *
 * Run:
 * ./rule_of_three_five_zero
 */

// Bad example. Do not use.
// This class owns a raw pointer but does not define copy behavior.
// Copying it would cause shallow copy and double delete.
class BadBuffer {
private:
    int* data;
    int size;

public:
    explicit BadBuffer(int n)
        : data(new int[n]), size(n) {
        cout << "BadBuffer constructor\n";
    }

    ~BadBuffer() {
        cout << "BadBuffer destructor\n";
        delete[] data;
    }
};

// Rule of Five version.
class Buffer {
private:
    int* data;
    int size;

public:
    explicit Buffer(int n)
        : data(new int[n]), size(n) {
        cout << "Buffer constructor, size = " << size << "\n";

        for (int i = 0; i < size; ++i) {
            data[i] = i;
        }
    }

    ~Buffer() {
        cout << "Buffer destructor, size = " << size << "\n";
        delete[] data;
    }

    // Copy constructor: deep copy.
    Buffer(const Buffer& other)
        : data(new int[other.size]), size(other.size) {
        cout << "Buffer copy constructor\n";
        copy(other.data, other.data + size, data);
    }

    // Copy assignment: copy-and-swap.
    Buffer& operator=(const Buffer& other) {
        cout << "Buffer copy assignment\n";

        if (this == &other) {
            return *this;
        }

        Buffer temp(other);
        swap(temp);

        return *this;
    }

    // Move constructor: steal resource.
    Buffer(Buffer&& other) noexcept
        : data(other.data), size(other.size) {
        cout << "Buffer move constructor\n";

        other.data = nullptr;
        other.size = 0;
    }

    // Move assignment: release current, steal resource.
    Buffer& operator=(Buffer&& other) noexcept {
        cout << "Buffer move assignment\n";

        if (this == &other) {
            return *this;
        }

        delete[] data;

        data = other.data;
        size = other.size;

        other.data = nullptr;
        other.size = 0;

        return *this;
    }

    void swap(Buffer& other) noexcept {
        std::swap(data, other.data);
        std::swap(size, other.size);
    }

    int getSize() const {
        return size;
    }

    int get(int index) const {
        return data[index];
    }

    void set(int index, int value) {
        data[index] = value;
    }

    void print(const string& label) const {
        cout << label << ": ";

        if (!data) {
            cout << "[moved-from empty buffer]\n";
            return;
        }

        for (int i = 0; i < size; ++i) {
            cout << data[i] << " ";
        }

        cout << "\n";
    }
};

// Rule of Zero version.
class VectorBuffer {
private:
    vector<int> data;

public:
    explicit VectorBuffer(int n)
        : data(n) {
        for (int i = 0; i < n; ++i) {
            data[i] = i;
        }
    }

    void print(const string& label) const {
        cout << label << ": ";

        for (int x : data) {
            cout << x << " ";
        }

        cout << "\n";
    }
};

Buffer makeBuffer() {
    Buffer temp(3);
    return temp; // may use NRVO or move
}

int main() {
    cout << "=== Rule of Five demo ===\n";

    Buffer a(5);
    a.print("a");

    cout << "\n--- copy construction ---\n";
    Buffer b = a;
    b.set(0, 100);

    a.print("a after b modification");
    b.print("b");

    cout << "\n--- copy assignment ---\n";
    Buffer c(2);
    c = a;
    c.print("c after copy assignment");

    cout << "\n--- move construction ---\n";
    Buffer d = std::move(c);
    d.print("d");
    c.print("c after move");

    cout << "\n--- move assignment ---\n";
    Buffer e(1);
    e = std::move(d);
    e.print("e");
    d.print("d after move");

    cout << "\n--- return by value ---\n";
    Buffer f = makeBuffer();
    f.print("f");

    cout << "\n=== Rule of Zero demo ===\n";

    VectorBuffer v1(4);
    VectorBuffer v2 = v1; // vector handles deep copy automatically

    v1.print("v1");
    v2.print("v2");

    cout << "\n=== end of main ===\n";

    return 0;
}