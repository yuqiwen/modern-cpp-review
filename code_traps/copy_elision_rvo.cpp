#include <iostream>
#include <memory>
#include <string>
#include <vector>
using namespace std;

/*
 * Topic:
 * Copy elision, RVO, NRVO, return by value.
 *
 * Compile:
 * g++ -std=c++17 copy_elision_rvo.cpp -o copy_elision_rvo
 *
 * Run:
 * ./copy_elision_rvo
 *
 * Optional:
 * g++ -std=c++17 -fno-elide-constructors copy_elision_rvo.cpp -o copy_elision_rvo
 * to observe more moves/copies in some cases.
 */

class Tracker {
private:
    string name;

public:
    explicit Tracker(string n)
        : name(std::move(n)) {
        cout << "constructor: " << name << endl;
    }

    ~Tracker() {
        cout << "destructor: " << name << endl;
    }

    Tracker(const Tracker& other)
        : name(other.name + "_copy") {
        cout << "copy constructor from " << other.name << endl;
    }

    Tracker(Tracker&& other) noexcept
        : name(std::move(other.name)) {
        cout << "move constructor" << endl;
        other.name = "moved_from";
    }

    Tracker& operator=(const Tracker& other) {
        cout << "copy assignment from " << other.name << endl;
        name = other.name + "_copy_assigned";
        return *this;
    }

    Tracker& operator=(Tracker&& other) noexcept {
        cout << "move assignment" << endl;
        name = std::move(other.name);
        other.name = "moved_from";
        return *this;
    }

    const string& getName() const {
        return name;
    }
};

Tracker makeRVO() {
    cout << "\ninside makeRVO\n";
    return Tracker("RVO_temp");
}

Tracker makeNRVO() {
    cout << "\ninside makeNRVO\n";
    Tracker t("NRVO_local");
    return t;
}

Tracker makeMoveReturn() {
    cout << "\ninside makeMoveReturn\n";
    Tracker t("move_return_local");
    return std::move(t); // may prevent NRVO
}

Tracker makeMultipleReturn(bool flag) {
    cout << "\ninside makeMultipleReturn\n";

    Tracker a("A");
    Tracker b("B");

    if (flag) {
        return a;
    }

    return b;
}

unique_ptr<Tracker> makeUniqueGood() {
    return make_unique<Tracker>("unique_good");
}

unique_ptr<Tracker> makeUniqueLocal() {
    auto p = make_unique<Tracker>("unique_local");
    return p;
}

Tracker* makeRawBad() {
    auto p = make_unique<Tracker>("raw_bad");
    return p.get(); // dangling after function returns
}

vector<int> makeVector() {
    vector<int> v;
    v.push_back(1);
    v.push_back(2);
    v.push_back(3);
    return v;
}

class Holder {
private:
    string value = "holder_value";

public:
    string take() {
        return std::move(value); // intentional move from member
    }

    const string& get() const {
        return value;
    }
};

void rvoDemo() {
    cout << "\n=== RVO demo ===" << endl;

    Tracker t = makeRVO();

    cout << "result name = " << t.getName() << endl;
}

void nrvoDemo() {
    cout << "\n=== NRVO demo ===" << endl;

    Tracker t = makeNRVO();

    cout << "result name = " << t.getName() << endl;
}

void moveReturnDemo() {
    cout << "\n=== return std::move(local) demo ===" << endl;

    Tracker t = makeMoveReturn();

    cout << "result name = " << t.getName() << endl;
}

void multipleReturnDemo() {
    cout << "\n=== multiple named return demo ===" << endl;

    Tracker t = makeMultipleReturn(true);

    cout << "result name = " << t.getName() << endl;
}

void uniquePtrReturnDemo() {
    cout << "\n=== unique_ptr return by value demo ===" << endl;

    auto p1 = makeUniqueGood();
    auto p2 = makeUniqueLocal();

    cout << "p1 = " << p1->getName() << endl;
    cout << "p2 = " << p2->getName() << endl;
}

void rawPointerBadDemo() {
    cout << "\n=== raw pointer bad demo ===" << endl;

    cout << "makeRawBad returns a dangling pointer." << endl;
    cout << "We will not dereference it." << endl;

    Tracker* p = makeRawBad();
    (void)p;
}

void vectorReturnDemo() {
    cout << "\n=== vector return by value demo ===" << endl;

    vector<int> v = makeVector();

    cout << "vector contents: ";
    for (int x : v) {
        cout << x << " ";
    }
    cout << endl;
}

void memberMoveDemo() {
    cout << "\n=== moving from member demo ===" << endl;

    Holder h;

    string s = h.take();

    cout << "taken value = " << s << endl;
    cout << "holder after take = " << h.get() << endl;
}

int main() {
    rvoDemo();

    nrvoDemo();

    moveReturnDemo();

    multipleReturnDemo();

    uniquePtrReturnDemo();

    rawPointerBadDemo();

    vectorReturnDemo();

    memberMoveDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}