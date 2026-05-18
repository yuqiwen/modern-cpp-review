#include <iostream>
#include <string>
#include <utility>
using namespace std;

/*
 * Topic:
 * Copy constructor, copy assignment, move constructor, and move assignment call timing.
 *
 * Compile:
 * g++ -std=c++17 copy_move_call_timing.cpp -o copy_move_call_timing
 *
 * Run:
 * ./copy_move_call_timing
 */

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
        cout << "Copy constructor from " << other.name << " to " << name << endl;
    }

    Tracker& operator=(const Tracker& other) {
        cout << "Copy assignment: " << name << " = " << other.name << endl;

        if (this == &other) {
            cout << "Self copy assignment detected" << endl;
            return *this;
        }

        name = other.name + "_copy_assigned";
        return *this;
    }

    Tracker(Tracker&& other) noexcept
        : name(std::move(other.name)) {
        cout << "Move constructor, new object took name: " << name << endl;
        other.name = "moved_from";
    }

    Tracker& operator=(Tracker&& other) noexcept {
        cout << "Move assignment: " << name << " = move(" << other.name << ")" << endl;

        if (this == &other) {
            cout << "Self move assignment detected" << endl;
            return *this;
        }

        name = std::move(other.name);
        other.name = "moved_from";

        return *this;
    }

    const string& getName() const {
        return name;
    }
};

void passByValue(Tracker t) {
    cout << "Inside passByValue, object name = " << t.getName() << endl;
}

Tracker makeTrackerNRVO() {
    Tracker temp("temp_NRVO");
    return temp; // may use NRVO
}

Tracker makeTrackerMandatoryElision() {
    return Tracker("temporary_return"); // C++17 mandatory copy elision
}

int main() {
    cout << "\n=== Basic construction ===" << endl;
    Tracker a("a");

    cout << "\n=== Copy construction ===" << endl;
    Tracker b = a; // copy constructor

    cout << "\n=== Copy assignment ===" << endl;
    Tracker c("c");
    c = a; // copy assignment

    cout << "\n=== Move construction ===" << endl;
    Tracker d = std::move(a); // move constructor
    cout << "After move, a = " << a.getName() << endl;

    cout << "\n=== Move assignment ===" << endl;
    Tracker e("e");
    e = std::move(b); // move assignment
    cout << "After move, b = " << b.getName() << endl;

    cout << "\n=== Pass by value with lvalue ===" << endl;
    passByValue(c); // copy constructor

    cout << "\n=== Pass by value with std::move ===" << endl;
    passByValue(std::move(c)); // move constructor
    cout << "After move, c = " << c.getName() << endl;

    cout << "\n=== Return by value: NRVO candidate ===" << endl;
    Tracker f = makeTrackerNRVO();

    cout << "\n=== Return by value: mandatory copy elision candidate ===" << endl;
    Tracker g = makeTrackerMandatoryElision();

    cout << "\n=== std::move on const object ===" << endl;
    const Tracker h("const_h");
    Tracker i = std::move(h); // usually copy constructor, because h is const

    cout << "\n=== End of main ===" << endl;

    return 0;
}