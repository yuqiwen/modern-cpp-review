#include <cstdio>
#include <exception>
#include <iostream>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string>
using namespace std;

/*
 * Topic:
 * Constructor, destructor, and RAII.
 *
 * Compile:
 * g++ -std=c++17 constructor_destructor_raii.cpp -o constructor_destructor_raii
 *
 * Run:
 * ./constructor_destructor_raii
 */

struct Tracer {
    string name;

    explicit Tracer(const string& n) : name(n) {
        cout << "Constructor: " << name << endl;
    }

    ~Tracer() {
        cout << "Destructor: " << name << endl;
    }
};

void basicConstructorDestructorDemo() {
    cout << "\n=== basic constructor/destructor demo ===" << endl;

    Tracer t("local object");

    cout << "Inside function." << endl;
} // t destructor runs here

void earlyReturnWithoutLeakDemo(bool shouldReturnEarly) {
    cout << "\n=== RAII with early return demo ===" << endl;

    auto ptr = make_unique<Tracer>("unique_ptr managed object");

    if (shouldReturnEarly) {
        cout << "Returning early." << endl;
        return;
    }

    cout << "Normal return path." << endl;
} // ptr destructor runs here and deletes managed Tracer

void exceptionSafetyDemo() {
    cout << "\n=== RAII with exception demo ===" << endl;

    Tracer t("object before exception");

    cout << "About to throw exception." << endl;

    throw runtime_error("something went wrong");
} // t destructor still runs during stack unwinding

class SimpleFile {
private:
    FILE* file;

public:
    SimpleFile(const char* path, const char* mode) : file(nullptr) {
        file = fopen(path, mode);
        if (!file) {
            throw runtime_error("failed to open file");
        }

        cout << "File opened." << endl;
    }

    ~SimpleFile() {
        if (file) {
            fclose(file);
            cout << "File closed." << endl;
        }
    }

    void writeLine(const string& line) {
        if (file) {
            fprintf(file, "%s\n", line.c_str());
        }
    }

    // For now, disable copy to avoid double fclose.
    SimpleFile(const SimpleFile&) = delete;
    SimpleFile& operator=(const SimpleFile&) = delete;
};

void fileRaiiDemo() {
    cout << "\n=== file RAII demo ===" << endl;

    SimpleFile file("raii_demo_output.txt", "w");
    file.writeLine("Hello RAII.");
    file.writeLine("The file will be closed automatically.");
} // file destructor closes the file

class Member {
public:
    Member() {
        cout << "Member constructor" << endl;
    }

    ~Member() {
        cout << "Member destructor" << endl;
    }
};

class ConstructorThrows {
private:
    Member member;

public:
    ConstructorThrows() {
        cout << "ConstructorThrows constructor body" << endl;
        throw runtime_error("ConstructorThrows failed");
    }

    ~ConstructorThrows() {
        cout << "ConstructorThrows destructor" << endl;
    }
};

void constructorThrowDemo() {
    cout << "\n=== constructor throws demo ===" << endl;

    try {
        ConstructorThrows obj;
    } catch (const exception& e) {
        cout << "Caught exception: " << e.what() << endl;
    }

    cout << "Notice: Member destructor ran, but ConstructorThrows destructor did not." << endl;
}

class InitOrderTrap {
private:
    int a;
    int b;

public:
    // Members are initialized in declaration order: a first, then b.
    // This version is safe because a does not depend on b.
    InitOrderTrap() : a(1), b(2) {}

    void print() const {
        cout << "a = " << a << ", b = " << b << endl;
    }
};

void initOrderDemo() {
    cout << "\n=== initialization order demo ===" << endl;

    InitOrderTrap obj;
    obj.print();

    cout << "Reminder: member initialization order follows declaration order." << endl;
}

void mutexRaiiDemo() {
    cout << "\n=== mutex RAII demo ===" << endl;

    mutex m;

    {
        lock_guard<mutex> lock(m);
        cout << "Inside critical section." << endl;
    } // lock_guard destructor unlocks mutex here

    cout << "Mutex was unlocked automatically." << endl;
}

int main() {
    basicConstructorDestructorDemo();

    earlyReturnWithoutLeakDemo(true);

    try {
        exceptionSafetyDemo();
    } catch (const exception& e) {
        cout << "Caught exception in main: " << e.what() << endl;
    }

    fileRaiiDemo();

    constructorThrowDemo();

    initOrderDemo();

    mutexRaiiDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}