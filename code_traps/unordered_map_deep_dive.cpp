
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
using namespace std;

/*
 * Topic:
 * unordered_map deep dive.
 *
 * Compile:
 * g++ -std=c++17 unordered_map_deep_dive.cpp -o unordered_map_deep_dive
 *
 * Run:
 * ./unordered_map_deep_dive
 */

void bucketLoadFactorDemo() {
    cout << "\n=== bucket / load_factor demo ===" << endl;

    unordered_map<string, int> mp;

    mp.max_load_factor(0.7);
    mp.reserve(10);

    cout << "after reserve(10):" << endl;
    cout << "size = " << mp.size()
         << ", bucket_count = " << mp.bucket_count()
         << ", load_factor = " << mp.load_factor()
         << ", max_load_factor = " << mp.max_load_factor()
         << endl;

    mp["apple"] = 1;
    mp["banana"] = 2;
    mp["orange"] = 3;

    cout << "after inserting 3 elements:" << endl;
    cout << "size = " << mp.size()
         << ", bucket_count = " << mp.bucket_count()
         << ", load_factor = " << mp.load_factor()
         << endl;
}

void operatorBracketTrapDemo() {
    cout << "\n=== operator[] trap demo ===" << endl;

    unordered_map<string, int> mp;

    cout << "initial size = " << mp.size() << endl;

    cout << "mp[\"apple\"] = " << mp["apple"] << endl;

    cout << "after reading mp[\"apple\"], size = " << mp.size() << endl;
    cout << "operator[] inserted apple with default value 0" << endl;
}

void findContainsAtDemo() {
    cout << "\n=== find / at demo ===" << endl;

    unordered_map<string, int> mp;
    mp["apple"] = 3;

    auto it = mp.find("apple");
    if (it != mp.end()) {
        cout << "find apple: " << it->second << endl;
    }

    auto missing = mp.find("banana");
    if (missing == mp.end()) {
        cout << "find banana: not found, no insertion" << endl;
    }

    cout << "size after find missing key = " << mp.size() << endl;

    try {
        cout << mp.at("banana") << endl;
    } catch (const out_of_range&) {
        cout << "mp.at(\"banana\") throws out_of_range" << endl;
    }
}

struct NoDefault {
    int value;

    explicit NoDefault(int v)
        : value(v) {
        cout << "NoDefault constructor: " << value << endl;
    }

    NoDefault(const NoDefault& other)
        : value(other.value) {
        cout << "NoDefault copy constructor\n";
    }

    NoDefault(NoDefault&& other) noexcept
        : value(other.value) {
        cout << "NoDefault move constructor\n";
        other.value = 0;
    }

    NoDefault& operator=(const NoDefault& other) {
        cout << "NoDefault copy assignment\n";
        value = other.value;
        return *this;
    }

    NoDefault& operator=(NoDefault&& other) noexcept {
        cout << "NoDefault move assignment\n";
        value = other.value;
        other.value = 0;
        return *this;
    }
};

void noDefaultValueDemo() {
    cout << "\n=== non-default-constructible mapped value demo ===" << endl;

    unordered_map<string, NoDefault> mp;

    // mp["a"]; // error: NoDefault is not default constructible

    mp.emplace("a", NoDefault(10));

    mp.try_emplace("b", 20);

    mp.insert_or_assign("a", NoDefault(30));

    cout << "mp.at(\"a\").value = " << mp.at("a").value << endl;
    cout << "mp.at(\"b\").value = " << mp.at("b").value << endl;
}

void tryEmplaceMoveOnlyDemo() {
    cout << "\n=== try_emplace with move-only value demo ===" << endl;

    unordered_map<string, unique_ptr<int>> mp;

    mp.try_emplace("a", make_unique<int>(10));

    // If key already exists, try_emplace will not replace existing value.
    mp.try_emplace("a", make_unique<int>(20));

    cout << "*mp[\"a\"] = " << *mp.at("a") << endl;

    mp.insert_or_assign("a", make_unique<int>(30));

    cout << "after insert_or_assign, *mp[\"a\"] = " << *mp.at("a") << endl;
}

struct PairHash {
    size_t operator()(const pair<int, int>& p) const {
        size_t h1 = hash<int>{}(p.first);
        size_t h2 = hash<int>{}(p.second);

        return h1 ^ (h2 << 1);
    }
};

void pairKeyDemo() {
    cout << "\n=== pair key custom hash demo ===" << endl;

    unordered_map<pair<int, int>, string, PairHash> mp;

    mp[{1, 2}] = "one-two";
    mp[{3, 4}] = "three-four";

    cout << "mp[{1, 2}] = " << mp[{1, 2}] << endl;
}

struct Point {
    int x;
    int y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

struct PointHash {
    size_t operator()(const Point& p) const {
        size_t h1 = hash<int>{}(p.x);
        size_t h2 = hash<int>{}(p.y);

        return h1 ^ (h2 << 1);
    }
};

void customStructKeyDemo() {
    cout << "\n=== custom struct key demo ===" << endl;

    unordered_map<Point, string, PointHash> mp;

    mp[{1, 2}] = "point A";
    mp[{3, 4}] = "point B";

    Point query{1, 2};

    auto it = mp.find(query);
    if (it != mp.end()) {
        cout << "found query: " << it->second << endl;
    }
}

struct BadHash {
    size_t operator()(int) const {
        return 0;
    }
};

void badHashDemo() {
    cout << "\n=== bad hash demo ===" << endl;

    unordered_map<int, int, BadHash> mp;

    for (int i = 0; i < 10; ++i) {
        mp[i] = i * i;
    }

    cout << "size = " << mp.size()
         << ", bucket_count = " << mp.bucket_count()
         << ", load_factor = " << mp.load_factor()
         << endl;

    cout << "All keys hash to the same bucket, causing many collisions." << endl;
    cout << "This demonstrates why hash quality matters." << endl;
}

void rehashReferenceDemo() {
    cout << "\n=== rehash reference stability demo ===" << endl;

    unordered_map<string, int> mp;
    mp["apple"] = 1;

    int& ref = mp["apple"];
    int* ptr = &mp["apple"];

    cout << "address before reserve = " << static_cast<const void*>(ptr) << endl;

    mp.reserve(1000);

    cout << "address after reserve  = " << static_cast<const void*>(&mp["apple"]) << endl;

    ref = 42;

    cout << "mp[\"apple\"] = " << mp["apple"] << endl;
    cout << "Reference/pointer remained valid after rehash." << endl;
}

int main() {
    bucketLoadFactorDemo();

    operatorBracketTrapDemo();

    findContainsAtDemo();

    noDefaultValueDemo();

    tryEmplaceMoveOnlyDemo();

    pairKeyDemo();

    customStructKeyDemo();

    badHashDemo();

    rehashReferenceDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}