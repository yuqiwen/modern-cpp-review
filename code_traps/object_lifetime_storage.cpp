#include <iostream>
#include <memory>
#include <string>
using namespace std;

/*
 * Topic:
 * Object lifetime and storage duration.
 *
 * Compile:
 * g++ -std=c++17 object_lifetime_storage.cpp -o object_lifetime_storage
 *
 * Run:
 * ./object_lifetime_storage
 */

struct Tracer {
    string name;

    Tracer(const string& n) : name(n) {
        cout << "Constructor: " << name << endl;
    }

    ~Tracer() {
        cout << "Destructor: " << name << endl;
    }
};

void automaticObjectDemo() {
    cout << "\n=== automatic object demo ===" << endl;

    Tracer t("automatic local object");

    cout << "Inside function." << endl;
} // t destructor runs here

void blockScopeDemo() {
    cout << "\n=== block scope demo ===" << endl;

    cout << "Before block." << endl;

    {
        Tracer t("block object");
        cout << "Inside block." << endl;
    } // t destructor runs here

    cout << "After block." << endl;
}

void dynamicObjectRawDemo() {
    cout << "\n=== dynamic object with raw pointer demo ===" << endl;

    Tracer* p = new Tracer("dynamic raw object");

    cout << "Object created with new." << endl;

    delete p; // destructor runs here

    cout << "Object deleted manually." << endl;
}

void dynamicObjectSmartPointerDemo() {
    cout << "\n=== dynamic object with unique_ptr demo ===" << endl;

    auto p = make_unique<Tracer>("dynamic unique_ptr object");

    cout << "unique_ptr owns the object." << endl;
} // unique_ptr destructor deletes the object here

int goodReturnByValue() {
    int x = 10;
    return x; // OK: returns value, not address/reference
}

// Bad example: do not use.
// This returns a pointer to a local variable.
// int* badPointerReturn() {
//     int x = 10;
//     return &x;
// }

// Bad example: do not use.
// This returns a reference to a local variable.
// int& badReferenceReturn() {
//     int x = 10;
//     return x;
// }

int& staticLocalReference() {
    static int count = 0;
    return count;
}

void staticLocalDemo() {
    cout << "\n=== static local demo ===" << endl;

    int& c1 = staticLocalReference();
    c1++;

    int& c2 = staticLocalReference();
    c2++;

    cout << "static count = " << staticLocalReference() << endl;
}

int main() {
    automaticObjectDemo();

    blockScopeDemo();

    dynamicObjectRawDemo();

    dynamicObjectSmartPointerDemo();

    cout << "\n=== return by value demo ===" << endl;
    int value = goodReturnByValue();
    cout << "value = " << value << endl;

    staticLocalDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}