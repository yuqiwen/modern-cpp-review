#include <iostream>
#include <string>
#include <vector>
using namespace std;

/*
 * Topic:
 * Pointer, reference, parameter passing, and lifetime.
 *
 * Compile:
 * g++ -std=c++17 pointer_reference_parameter.cpp -o pointer_reference_parameter
 *
 * Run:
 * ./pointer_reference_parameter
 */

void changeByValue(int x) {
    x = 100;
}

void changeByRef(int& x) {
    x = 200;
}

void changeByPtr(int* x) {
    if (x) {
        *x = 300;
    }
}

void printVector(const vector<int>& nums) {
    for (int x : nums) {
        cout << x << " ";
    }
    cout << endl;
}

void normalize(vector<int>& nums) {
    for (int& x : nums) {
        if (x < 0) {
            x = -x;
        }
    }
}

// Bad example: do not use.
// This returns the address of a local variable.
// int* danglingPointer() {
//     int x = 10;
//     return &x;
// }

// Bad example: do not use.
// This returns a reference to a local variable.
// int& danglingReference() {
//     int x = 10;
//     return x;
// }

// Bad example: do not use.
// This returns a const reference to a temporary string.
// const string& danglingConstReference() {
//     return string("hello");
// }

int main() {
    cout << "=== Reference assignment vs pointer reseating ===" << endl;

    int a = 10;
    int b = 20;

    int& r = a;
    int* p = &a;

    r = b;   // This means a = b, not r rebinding to b.
    p = &b;  // This reseats p to point to b.

    changeByValue(a); // a is unchanged.
    changeByRef(a);   // a becomes 200.
    changeByPtr(p);   // p points to b, so b becomes 300.

    cout << "a = " << a << ", b = " << b << endl;
    cout << "Expected: a = 200, b = 300" << endl;

    cout << endl;

    cout << "=== const reference input ===" << endl;

    vector<int> nums = {1, -2, 3, -4};

    cout << "Before normalize: ";
    printVector(nums);

    normalize(nums);

    cout << "After normalize: ";
    printVector(nums);

    cout << endl;

    cout << "=== pointer can be nullptr ===" << endl;

    int* nullPtr = nullptr;
    changeByPtr(nullPtr); // Safe because changeByPtr checks nullptr.

    cout << "Calling changeByPtr(nullptr) is safe in this implementation." << endl;

    return 0;
}