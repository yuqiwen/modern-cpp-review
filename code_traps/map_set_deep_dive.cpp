#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <string>
#include <utility>
using namespace std;

/*
 * Topic:
 * map and set deep dive.
 *
 * Compile:
 * g++ -std=c++17 map_set_deep_dive.cpp -o map_set_deep_dive
 *
 * Run:
 * ./map_set_deep_dive
 */

void orderedIterationDemo() {
    cout << "\n=== ordered iteration demo ===" << endl;

    map<int, string> mp;

    mp[3] = "three";
    mp[1] = "one";
    mp[2] = "two";

    cout << "map iterates in sorted key order: ";
    for (const auto& [key, value] : mp) {
        cout << key << ":" << value << " ";
    }
    cout << endl;

    set<int> s = {5, 1, 3, 2, 4};

    cout << "set iterates in sorted order: ";
    for (int x : s) {
        cout << x << " ";
    }
    cout << endl;
}

void descendingOrderDemo() {
    cout << "\n=== descending order demo ===" << endl;

    set<int, greater<int>> s = {5, 1, 3, 2, 4};

    cout << "set with greater<int>: ";
    for (int x : s) {
        cout << x << " ";
    }
    cout << endl;
}

struct Person {
    string name;
    int age;
};

struct ByAgeThenName {
    bool operator()(const Person& a, const Person& b) const {
        if (a.age != b.age) {
            return a.age < b.age;
        }
        return a.name < b.name;
    }
};

void customComparatorDemo() {
    cout << "\n=== custom comparator demo ===" << endl;

    set<Person, ByAgeThenName> people;

    people.insert({"Alice", 25});
    people.insert({"Bob", 20});
    people.insert({"Charlie", 25});

    for (const auto& p : people) {
        cout << p.name << "(" << p.age << ") ";
    }
    cout << endl;
}

void lowerUpperBoundDemo() {
    cout << "\n=== lower_bound / upper_bound demo ===" << endl;

    set<int> s = {1, 3, 5, 7};

    auto lb4 = s.lower_bound(4);
    auto ub4 = s.upper_bound(4);

    if (lb4 != s.end()) {
        cout << "lower_bound(4) = " << *lb4 << endl;
    }

    if (ub4 != s.end()) {
        cout << "upper_bound(4) = " << *ub4 << endl;
    }

    auto lb5 = s.lower_bound(5);
    auto ub5 = s.upper_bound(5);

    if (lb5 != s.end()) {
        cout << "lower_bound(5) = " << *lb5 << endl;
    }

    if (ub5 != s.end()) {
        cout << "upper_bound(5) = " << *ub5 << endl;
    }
}

void rangeQueryDemo() {
    cout << "\n=== range query demo ===" << endl;

    set<int> s = {1, 3, 5, 7, 9, 11};

    int L = 4;
    int R = 10;

    cout << "values in [" << L << ", " << R << "]: ";

    auto it = s.lower_bound(L);

    while (it != s.end() && *it <= R) {
        cout << *it << " ";
        ++it;
    }

    cout << endl;
}

void mapOperatorBracketTrapDemo() {
    cout << "\n=== map operator[] trap demo ===" << endl;

    map<string, int> mp;

    cout << "initial size = " << mp.size() << endl;

    cout << "mp[\"apple\"] = " << mp["apple"] << endl;

    cout << "after mp[\"apple\"], size = " << mp.size() << endl;
    cout << "operator[] inserted apple with default value 0" << endl;
}

void insertEmplaceDemo() {
    cout << "\n=== insert / emplace / insert_or_assign demo ===" << endl;

    map<string, int> mp;

    auto [it1, inserted1] = mp.insert({"apple", 3});
    cout << "insert apple 3, inserted = " << inserted1 << endl;

    auto [it2, inserted2] = mp.insert({"apple", 5});
    cout << "insert apple 5, inserted = " << inserted2 << endl;
    cout << "apple value = " << mp["apple"] << endl;

    mp.insert_or_assign("apple", 10);
    cout << "after insert_or_assign, apple value = " << mp["apple"] << endl;
}

void iteratorStabilityDemo() {
    cout << "\n=== iterator stability demo ===" << endl;

    map<int, string> mp;

    mp[10] = "ten";
    mp[20] = "twenty";

    auto it = mp.find(10);
    string& ref = it->second;

    mp[15] = "fifteen";
    mp[5] = "five";

    cout << "iterator still valid: " << it->first << " -> " << it->second << endl;
    cout << "reference still valid: " << ref << endl;

    mp.erase(10);

    cout << "after erasing key 10, iterator/reference to key 10 are invalid" << endl;
}

void equalRangeMultisetDemo() {
    cout << "\n=== multiset equal_range demo ===" << endl;

    multiset<int> ms = {1, 2, 2, 2, 3, 4};

    auto [first, last] = ms.equal_range(2);

    cout << "all values equal to 2: ";
    for (auto it = first; it != last; ++it) {
        cout << *it << " ";
    }
    cout << endl;
}

void keyConstDemo() {
    cout << "\n=== map key const demo ===" << endl;

    map<string, int> mp;
    mp["apple"] = 3;

    auto it = mp.find("apple");

    if (it != mp.end()) {
        cout << "key = " << it->first << ", value = " << it->second << endl;

        // This is illegal because map keys are const:
        // it->first = "banana";

        cout << "map keys cannot be modified in place." << endl;
    }
}

int main() {
    orderedIterationDemo();

    descendingOrderDemo();

    customComparatorDemo();

    lowerUpperBoundDemo();

    rangeQueryDemo();

    mapOperatorBracketTrapDemo();

    insertEmplaceDemo();

    iteratorStabilityDemo();

    equalRangeMultisetDemo();

    keyConstDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}