#include <iostream>
#include <list>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

/*
 * Topic:
 * Iterator invalidation and unordered_map rehash.
 *
 * Compile:
 * g++ -std=c++17 iterator_invalidation_unordered_map.cpp -o iterator_invalidation_unordered_map
 *
 * Run:
 * ./iterator_invalidation_unordered_map
 */

void vectorReallocationDemo() {
    cout << "\n=== vector reallocation invalidation demo ===" << endl;

    vector<int> v;
    v.reserve(3);

    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    int* oldPtr = &v[0];
    auto oldData = v.data();

    cout << "before push, data = " << static_cast<const void*>(oldData) << endl;

    v.push_back(4);

    cout << "after push, data  = " << static_cast<const void*>(v.data()) << endl;

    if (oldData != v.data()) {
        cout << "Reallocation happened. oldPtr is invalid and must not be dereferenced." << endl;
    } else {
        cout << "No reallocation happened. oldPtr remains valid." << endl;
        cout << "*oldPtr = " << *oldPtr << endl;
    }
}

void vectorEraseDemo() {
    cout << "\n=== vector erase invalidation demo ===" << endl;

    vector<int> v = {1, 2, 3, 4, 5, 6};

    for (auto it = v.begin(); it != v.end(); ) {
        if (*it % 2 == 0) {
            it = v.erase(it);
        } else {
            ++it;
        }
    }

    cout << "after erasing even numbers: ";
    for (int x : v) {
        cout << x << " ";
    }
    cout << endl;
}

void listStabilityDemo() {
    cout << "\n=== list iterator stability demo ===" << endl;

    list<int> lst = {1, 2, 3};

    auto it = next(lst.begin()); // points to 2
    int& ref = *it;

    lst.push_front(0);
    lst.push_back(4);

    cout << "iterator still points to: " << *it << endl;
    cout << "reference still refers to: " << ref << endl;

    lst.erase(it);

    cout << "After erasing that element, the old iterator/reference are invalid." << endl;
}

void mapStabilityDemo() {
    cout << "\n=== map iterator stability demo ===" << endl;

    map<int, string> mp;
    mp[2] = "two";

    auto it = mp.find(2);
    string& ref = it->second;

    mp[1] = "one";
    mp[3] = "three";

    cout << "iterator still points to: " << it->first << " -> " << it->second << endl;
    cout << "reference still refers to: " << ref << endl;

    mp.erase(2);

    cout << "After erasing key 2, iterator/reference to key 2 are invalid." << endl;
}

void unorderedMapRehashDemo() {
    cout << "\n=== unordered_map rehash demo ===" << endl;

    unordered_map<string, int> mp;

    mp.reserve(2);
    mp["apple"] = 1;
    mp["banana"] = 2;

    auto it = mp.find("apple");
    int& ref = it->second;
    int* ptr = &it->second;

    cout << "before rehash:" << endl;
    cout << "bucket_count = " << mp.bucket_count()
         << ", load_factor = " << mp.load_factor() << endl;
    cout << "address of apple value = " << static_cast<const void*>(ptr) << endl;

    mp.reserve(1000); // likely rehash

    cout << "after reserve(1000):" << endl;
    cout << "bucket_count = " << mp.bucket_count()
         << ", load_factor = " << mp.load_factor() << endl;
    cout << "address from ptr still = " << static_cast<const void*>(ptr) << endl;

    ref = 10;
    *ptr = 20;

    cout << "mp[\"apple\"] = " << mp["apple"] << endl;
    cout << "Reference and pointer to element value remained valid after rehash." << endl;

    cout << "The old iterator is invalid after rehash and should not be used." << endl;

    auto newIt = mp.find("apple");
    cout << "new iterator value = " << newIt->second << endl;
}

void unorderedMapEraseDemo() {
    cout << "\n=== unordered_map erase demo ===" << endl;

    unordered_map<string, int> mp;
    mp["apple"] = 1;
    mp["banana"] = 2;

    auto it = mp.find("apple");

    if (it != mp.end()) {
        cout << "found apple before erase: " << it->second << endl;
    }

    mp.erase("apple");

    cout << "After erase, iterator/reference/pointer to apple are invalid." << endl;

    auto newIt = mp.find("apple");
    if (newIt == mp.end()) {
        cout << "apple no longer exists" << endl;
    }
}

void unorderedMapReserveDemo() {
    cout << "\n=== unordered_map reserve demo ===" << endl;

    unordered_map<int, int> mp;

    cout << "initial bucket_count = " << mp.bucket_count() << endl;

    mp.reserve(1000);

    cout << "after reserve(1000), bucket_count = " << mp.bucket_count() << endl;

    for (int i = 0; i < 1000; ++i) {
        mp[i] = i * i;
    }

    cout << "after inserting 1000 elements:" << endl;
    cout << "size = " << mp.size()
         << ", bucket_count = " << mp.bucket_count()
         << ", load_factor = " << mp.load_factor() << endl;
}

int main() {
    vectorReallocationDemo();

    vectorEraseDemo();

    listStabilityDemo();

    mapStabilityDemo();

    unorderedMapRehashDemo();

    unorderedMapEraseDemo();

    unorderedMapReserveDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}