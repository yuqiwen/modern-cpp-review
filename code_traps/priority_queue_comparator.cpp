#include <functional>
#include <iostream>
#include <queue>
#include <string>
#include <utility>
#include <vector>
using namespace std;

/*
 * Topic:
 * priority_queue comparator.
 *
 * Compile:
 * g++ -std=c++17 priority_queue_comparator.cpp -o priority_queue_comparator
 *
 * Run:
 * ./priority_queue_comparator
 */

void defaultMaxHeapDemo() {
    cout << "\n=== default max heap demo ===" << endl;

    priority_queue<int> pq;

    pq.push(3);
    pq.push(10);
    pq.push(5);

    cout << "pop order: ";
    while (!pq.empty()) {
        cout << pq.top() << " ";
        pq.pop();
    }
    cout << endl;
}

void minHeapDemo() {
    cout << "\n=== min heap demo ===" << endl;

    priority_queue<int, vector<int>, greater<int>> pq;

    pq.push(3);
    pq.push(10);
    pq.push(5);

    cout << "pop order: ";
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
        // true means a has lower priority than b.
        // Smaller priority number is lower priority.
        return a.priority < b.priority;
    }
};

void taskPriorityDemo() {
    cout << "\n=== task priority demo ===" << endl;

    priority_queue<Task, vector<Task>, TaskCompare> pq;

    pq.push({"low", 1});
    pq.push({"medium", 5});
    pq.push({"high", 10});

    cout << "pop order: ";
    while (!pq.empty()) {
        cout << pq.top().name << "(" << pq.top().priority << ") ";
        pq.pop();
    }
    cout << endl;
}

void pairDefaultDemo() {
    cout << "\n=== pair default ordering demo ===" << endl;

    priority_queue<pair<int, int>> pq;

    pq.push({5, 1});
    pq.push({3, 100});
    pq.push({5, 10});

    cout << "top pair = (" << pq.top().first << ", " << pq.top().second << ")" << endl;
    cout << "pair comparison is lexicographical: first, then second." << endl;
}

using NodeDist = pair<int, int>; // node, distance

struct DistCompare {
    bool operator()(const NodeDist& a, const NodeDist& b) const {
        // true means a has lower priority.
        // Larger distance should be lower priority.
        return a.second > b.second;
    }
};

void dijkstraStyleDemo() {
    cout << "\n=== Dijkstra-style min distance heap demo ===" << endl;

    priority_queue<NodeDist, vector<NodeDist>, DistCompare> pq;

    pq.push({1, 10});
    pq.push({2, 3});
    pq.push({3, 7});

    cout << "pop order by smallest distance: ";
    while (!pq.empty()) {
        auto [node, dist] = pq.top();
        cout << "(" << node << "," << dist << ") ";
        pq.pop();
    }
    cout << endl;
}

void lambdaComparatorDemo() {
    cout << "\n=== lambda comparator demo ===" << endl;

    auto cmp = [](const pair<string, int>& a, const pair<string, int>& b) {
        // min heap by frequency
        // lower frequency should be higher priority/top
        return a.second > b.second;
    };

    priority_queue<
        pair<string, int>,
        vector<pair<string, int>>,
        decltype(cmp)
    > pq(cmp);

    pq.push({"apple", 3});
    pq.push({"banana", 1});
    pq.push({"orange", 2});

    cout << "pop order by smallest frequency: ";
    while (!pq.empty()) {
        auto [word, freq] = pq.top();
        cout << word << "(" << freq << ") ";
        pq.pop();
    }
    cout << endl;
}

struct TopKWordCompare {
    bool operator()(const pair<string, int>& a,
                    const pair<string, int>& b) const {
        // We want top() to be the worst element among current kept elements.
        // Worst means lower frequency.
        // If same frequency, lexicographically larger word is worse.
        //
        // priority_queue comp(a,b)=true means a has lower priority than b.
        // So the "better" element should have lower priority in this reversed setup.
        if (a.second == b.second) {
            return a.first < b.first;
        }
        return a.second > b.second;
    }
};

void topKWordsMinHeapDemo() {
    cout << "\n=== top K words min-heap demo ===" << endl;

    priority_queue<
        pair<string, int>,
        vector<pair<string, int>>,
        TopKWordCompare
    > pq;

    vector<pair<string, int>> items = {
        {"i", 2},
        {"love", 2},
        {"leetcode", 1},
        {"coding", 1}
    };

    int k = 2;

    for (const auto& item : items) {
        pq.push(item);

        if (static_cast<int>(pq.size()) > k) {
            pq.pop();
        }
    }

    vector<pair<string, int>> result;
    while (!pq.empty()) {
        result.push_back(pq.top());
        pq.pop();
    }

    reverse(result.begin(), result.end());

    cout << "top k words: ";
    for (const auto& [word, freq] : result) {
        cout << word << "(" << freq << ") ";
    }
    cout << endl;
}

int main() {
    defaultMaxHeapDemo();

    minHeapDemo();

    taskPriorityDemo();

    pairDefaultDemo();

    dijkstraStyleDemo();

    lambdaComparatorDemo();

    topKWordsMinHeapDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}