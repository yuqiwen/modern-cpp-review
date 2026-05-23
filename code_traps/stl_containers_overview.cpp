#include <deque>
#include <iostream>
#include <list>
#include <map>
#include <queue>
#include <string>
#include <unordered_map>
#include <vector>
using namespace std;

/*
 * Topic:
 * STL containers overview.
 *
 * Compile:
 * g++ -std=c++17 stl_containers_overview.cpp -o stl_containers_overview
 *
 * Run:
 * ./stl_containers_overview
 */

void vectorDemo() {
    cout << "\n=== vector demo ===" << endl;

    vector<int> v;
    v.reserve(3);

    cout << "initial size = " << v.size()
         << ", capacity = " << v.capacity() << endl;

    v.push_back(1);
    v.push_back(2);
    v.push_back(3);

    cout << "after push_back, size = " << v.size()
         << ", capacity = " << v.capacity() << endl;

    int* p = &v[0];

    cout << "address before possible reallocation = "
         << static_cast<const void*>(p) << endl;

    v.push_back(4);

    cout << "after another push_back, size = " << v.size()
         << ", capacity = " << v.capacity() << endl;

    cout << "current address of v[0] = "
         << static_cast<const void*>(&v[0]) << endl;

    cout << "If address changed, old pointer p is dangling." << endl;
}

void dequeDemo() {
    cout << "\n=== deque demo ===" << endl;

    deque<int> dq;

    dq.push_back(2);
    dq.push_front(1);
    dq.push_back(3);

    cout << "deque contents: ";
    for (int x : dq) {
        cout << x << " ";
    }
    cout << endl;

    cout << "deque supports push_front and push_back efficiently." << endl;
}

void listDemo() {
    cout << "\n=== list demo ===" << endl;

    list<int> lst = {1, 2, 4};

    auto it = lst.begin();
    advance(it, 2); // points to 4

    lst.insert(it, 3);

    cout << "list contents after insert: ";
    for (int x : lst) {
        cout << x << " ";
    }
    cout << endl;

    cout << "list insertion given an iterator is O(1), but finding the position may be O(n)." << endl;
}

void mapDemo() {
    cout << "\n=== map demo ===" << endl;

    map<int, string> mp;
    mp[3] = "three";
    mp[1] = "one";
    mp[2] = "two";

    cout << "map iterates in sorted key order: ";
    for (const auto& [key, value] : mp) {
        cout << key << ":" << value << " ";
    }
    cout << endl;

    auto it = mp.lower_bound(2);
    if (it != mp.end()) {
        cout << "lower_bound(2) = " << it->first << ":" << it->second << endl;
    }
}

void unorderedMapDemo() {
    cout << "\n=== unordered_map demo ===" << endl;

    unordered_map<string, int> mp;

    mp["apple"] = 3;
    mp["banana"] = 5;
    mp["orange"] = 2;

    cout << "unordered_map contents, order not guaranteed: ";
    for (const auto& [key, value] : mp) {
        cout << key << ":" << value << " ";
    }
    cout << endl;

    cout << "bucket_count = " << mp.bucket_count()
         << ", load_factor = " << mp.load_factor() << endl;

    auto it = mp.find("banana");
    if (it != mp.end()) {
        cout << "found banana = " << it->second << endl;
    }

    cout << "unordered_map gives average O(1) lookup but no sorted order." << endl;
}

void priorityQueueDemo() {
    cout << "\n=== priority_queue demo ===" << endl;

    priority_queue<int> maxHeap;
    maxHeap.push(3);
    maxHeap.push(10);
    maxHeap.push(5);

    cout << "max heap pop order: ";
    while (!maxHeap.empty()) {
        cout << maxHeap.top() << " ";
        maxHeap.pop();
    }
    cout << endl;

    priority_queue<int, vector<int>, greater<int>> minHeap;
    minHeap.push(3);
    minHeap.push(10);
    minHeap.push(5);

    cout << "min heap pop order: ";
    while (!minHeap.empty()) {
        cout << minHeap.top() << " ";
        minHeap.pop();
    }
    cout << endl;
}

void containerSelectionDemo() {
    cout << "\n=== container selection reminder ===" << endl;

    cout << "vector: default, contiguous, cache-friendly" << endl;
    cout << "deque: efficient push/pop at both ends" << endl;
    cout << "list: stable iterators, but poor cache locality" << endl;
    cout << "map: ordered keys, O(log n)" << endl;
    cout << "unordered_map: average O(1), no order" << endl;
    cout << "priority_queue: repeated max/min access" << endl;
}

int main() {
    vectorDemo();

    dequeDemo();

    listDemo();

    mapDemo();

    unorderedMapDemo();

    priorityQueueDemo();

    containerSelectionDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}