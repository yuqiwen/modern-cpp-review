#include <cstring>
#include <iostream>
#include <string>
#include <string_view>
#include <utility>
using namespace std;

/*
 * Topic:
 * std::string, std::string_view, and C string.
 *
 * Compile:
 * g++ -std=c++17 string_string_view_c_string.cpp -o string_string_view_c_string
 *
 * Run:
 * ./string_string_view_c_string
 */

void printStringView(string_view sv) {
    cout << "string_view: [" << sv << "], size = " << sv.size() << endl;
}

void printConstStringRef(const string& s) {
    cout << "const string&: [" << s << "], size = " << s.size() << endl;
}

void cStringDemo() {
    cout << "\n=== C string demo ===" << endl;

    const char* p = "hello";

    cout << "p = " << p << endl;
    cout << "strlen(p) = " << strlen(p) << endl;

    cout << "C string relies on null terminator '\\0'." << endl;
}

void stringOwnershipDemo() {
    cout << "\n=== std::string ownership demo ===" << endl;

    string a = "hello";
    string b = a;

    b[0] = 'H';

    cout << "a = " << a << endl;
    cout << "b = " << b << endl;

    cout << "std::string copy creates independent owned storage." << endl;
}

void stringMoveDemo() {
    cout << "\n=== std::string move demo ===" << endl;

    string a = "hello world";
    string b = std::move(a);

    cout << "b = " << b << endl;
    cout << "a is valid but unspecified after move: [" << a << "]" << endl;

    a = "new value";
    cout << "a after reassignment = " << a << endl;
}

const char* badCStringReturn() {
    string s = "local string";
    return s.c_str(); // dangling
}

void cStrLifetimeDemo() {
    cout << "\n=== c_str lifetime demo ===" << endl;

    string s = "hello";

    const char* p = s.c_str();

    cout << "p before modification = " << p << endl;

    s += " world";

    cout << "s after modification = " << s << endl;
    cout << "Old c_str pointer may be invalid after modification." << endl;

    cout << "Do not return s.c_str() from a local string." << endl;

    // const char* bad = badCStringReturn();
    // cout << bad << endl; // undefined behavior
}

string makeStringGood() {
    string s = "safe return by value";
    return s;
}

string_view badStringViewReturn() {
    string s = "local string";
    return string_view(s); // dangling
}

void stringViewDemo() {
    cout << "\n=== string_view demo ===" << endl;

    string s = "hello world";

    string_view sv1 = s;
    string_view sv2 = "literal";
    string_view sv3(s.data(), 5);

    printStringView(sv1);
    printStringView(sv2);
    printStringView(sv3);

    cout << "string_view stores pointer + length and does not own characters." << endl;
}

void danglingStringViewDemo() {
    cout << "\n=== dangling string_view demo ===" << endl;

    cout << "Dangerous code examples, do not use:" << endl;

    cout << "string_view sv = string(\"temporary\"); // dangling after statement" << endl;
    cout << "return string_view(localString);        // dangling after function returns" << endl;

    // string_view bad = string("temporary");
    // cout << bad << endl; // undefined behavior
}

void stringViewNotNullTerminatedDemo() {
    cout << "\n=== string_view may not be null-terminated demo ===" << endl;

    string s = "hello world";

    string_view sv(s.data(), 5); // views "hello" only

    cout << "sv printed by ostream = [" << sv << "]" << endl;

    cout << "sv.data() points into original string, but there may not be '\\0' after view length." << endl;
    cout << "Do not pass sv.data() to C APIs expecting null-terminated strings unless guaranteed." << endl;
}

class User {
private:
    string name;

public:
    void setName(string n) {
        name = std::move(n);
    }

    const string& getName() const {
        return name;
    }
};

void parameterGuidelineDemo() {
    cout << "\n=== parameter guideline demo ===" << endl;

    string s = "hello";

    printStringView("literal");
    printStringView(s);
    printStringView(string_view(s.data(), 3));

    printConstStringRef(s);

    User u;
    u.setName("Yuqi");

    cout << "stored name = " << u.getName() << endl;
}

int main() {
    cStringDemo();

    stringOwnershipDemo();

    stringMoveDemo();

    cStrLifetimeDemo();

    cout << "\nmakeStringGood() = " << makeStringGood() << endl;

    stringViewDemo();

    danglingStringViewDemo();

    stringViewNotNullTerminatedDemo();

    parameterGuidelineDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}