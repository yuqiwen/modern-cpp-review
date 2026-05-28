#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <string>
#include <tuple>
#include <vector>
using namespace std;

/*
 * Topic:
 * STL algorithms and comparators.
 *
 * Compile:
 * g++ -std=c++17 stl_algorithms_comparators.cpp -o stl_algorithms_comparators
 *
 * Run:
 * ./stl_algorithms_comparators
 */

void printVector(const vector<int>& v, const string& label) {
    cout << label << ": ";
    for (int x : v) {
        cout << x << " ";
    }
    cout << endl;
}

void sortDemo() {
    cout << "\n=== sort demo ===" << endl;

    vector<int> v = {3, 1, 5, 2, 4};

    sort(v.begin(), v.end());
    printVector(v, "ascending");

    sort(v.begin(), v.end(), greater<int>());
    printVector(v, "descending");
}

void pairSortDemo() {
    cout << "\n=== pair sort demo ===" << endl;

    vector<pair<int, int>> v = {
        {1, 3}, {1, 2}, {2, 1}, {2, 5}, {1, 5}
    };

    sort(v.begin(), v.end(), [](const auto& a, const auto& b) {
        if (a.first != b.first) {
            return a.first < b.first;
        }
        return a.second > b.second;
    });

    cout << "first ascending, second descending: ";
    for (const auto& [a, b] : v) {
        cout << "(" << a << "," << b << ") ";
    }
    cout << endl;
}

void lambdaCaptureDemo() {
    cout << "\n=== lambda capture demo ===" << endl;

    vector<int> v = {1, 10, 3, 8, 5};
    int target = 6;

    sort(v.begin(), v.end(), [target](int a, int b) {
        return abs(a - target) < abs(b - target);
    });

    printVector(v, "sorted by distance to target 6");
}

void stableSortDemo() {
    cout << "\n=== stable_sort demo ===" << endl;

    vector<pair<string, int>> students = {
        {"Alice", 90},
        {"Bob", 80},
        {"Charlie", 90},
        {"David", 80}
    };

    stable_sort(students.begin(), students.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });

    cout << "stable_sort by score descending: ";
    for (const auto& [name, score] : students) {
        cout << name << "(" << score << ") ";
    }
    cout << endl;
}

void nthElementDemo() {
    cout << "\n=== nth_element demo ===" << endl;

    vector<int> v = {9, 1, 5, 3, 7};

    nth_element(v.begin(), v.begin() + 2, v.end());

    printVector(v, "after nth_element index 2");
    cout << "v[2] is the third smallest element: " << v[2] << endl;
}

void lowerBoundDemo() {
    cout << "\n=== lower_bound / upper_bound demo ===" << endl;

    vector<int> v = {1, 3, 5, 7};

    auto lb = lower_bound(v.begin(), v.end(), 4);
    auto ub = upper_bound(v.begin(), v.end(), 5);

    if (lb != v.end()) {
        cout << "lower_bound(4) = " << *lb << endl;
    }

    if (ub != v.end()) {
        cout << "upper_bound(5) = " << *ub << endl;
    }
}

void priorityQueueDefaultDemo() {
    cout << "\n=== priority_queue default demo ===" << endl;

    priority_queue<int> pq;

    pq.push(3);
    pq.push(10);
    pq.push(5);

    cout << "max heap pop order: ";
    while (!pq.empty()) {
        cout << pq.top() << " ";
        pq.pop();
    }
    cout << endl;
}

void priorityQueueMinHeapDemo() {
    cout << "\n=== priority_queue min heap demo ===" << endl;

    priority_queue<int, vector<int>, greater<int>> pq;

    pq.push(3);
    pq.push(10);
    pq.push(5);

    cout << "min heap pop order: ";
    while (!pq.empty()) {
        cout << pq.top() << " ";
        pq.pop();
    }
    cout << endl;
}

struct Task {
    string name;
    int priority;
};

struct TaskCompare {
    bool operator()(const Task& a, const Task& b) const {
        // Return true if a has lower priority than b.
        return a.priority < b.priority;
    }
};

void priorityQueueCustomDemo() {
    cout << "\n=== priority_queue custom comparator demo ===" << endl;

    priority_queue<Task, vector<Task>, TaskCompare> pq;

    pq.push({"low", 1});
    pq.push({"high", 10});
    pq.push({"medium", 5});

    cout << "task priority order: ";
    while (!pq.empty()) {
        cout << pq.top().name << "(" << pq.top().priority << ") ";
        pq.pop();
    }
    cout << endl;
}

void badComparatorReminder() {
    cout << "\n=== bad comparator reminder ===" << endl;

    cout << "Do not write comparator as a <= b or a >= b." << endl;
    cout << "Comparator must be strict: comp(a, a) must be false." << endl;
}

int main() {
    sortDemo();

    pairSortDemo();

    lambdaCaptureDemo();

    stableSortDemo();

    nthElementDemo();

    lowerBoundDemo();

    priorityQueueDefaultDemo();

    priorityQueueMinHeapDemo();

    priorityQueueCustomDemo();

    badComparatorReminder();

    cout << "\n=== end of main ===" << endl;

    return 0;
}