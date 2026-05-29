# 16. priority_queue Comparator

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Template Parameters](#2-template-parameters)
- [3. Default Max Heap](#3-default-max-heap)
- [4. Min Heap](#4-min-heap)
- [5. Compare with sort](#5-compare-with-sort)
- [6. Custom Comparator Struct](#6-custom-comparator-struct)
- [7. Custom Min Heap by Field](#7-custom-min-heap-by-field)
- [8. Lambda Comparator](#8-lambda-comparator)
- [9. Why `decltype(cmp)`?](#9-why-decltypecmp)
- [10. Pair Default Ordering](#10-pair-default-ordering)
- [11. Common LeetCode Pattern: Top K Frequent](#11-common-leetcode-pattern-top-k-frequent)
- [12. Easier Rule for LeetCode Heap Comparator](#12-easier-rule-for-leetcode-heap-comparator)
- [13. Comparator Must Be Strict](#13-comparator-must-be-strict)
- [14. priority_queue Does Not Support Iteration in Sorted Order](#14-priority_queue-does-not-support-iteration-in-sorted-order)
- [15. Common Interview Questions](#15-common-interview-questions)
- [16. Key Takeaways](#16-key-takeaways)

## Related Code Trap

- [priority_queue Comparator Demo](../code_traps/priority_queue_comparator.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

`std::priority_queue` is a container adaptor that gives access to the highest-priority element.

Default:

```cpp
std::priority_queue<int> pq;
```

This is a max heap.

```cpp
pq.push(3);
pq.push(10);
pq.push(5);

pq.top(); // 10
```

Key mental model:

```text
For priority_queue comparator:
comp(a, b) == true means a has lower priority than b.
```

This is different from `std::sort`.

---

## 2. Template Parameters

```cpp
std::priority_queue<T, Container, Compare>
```

Example:

```cpp
std::priority_queue<int, std::vector<int>, std::less<int>> pq;
```

This is the default max heap.

The underlying container is usually:

```cpp
std::vector<T>
```

---

## 3. Default Max Heap

```cpp
std::priority_queue<int> pq;
```

Equivalent to:

```cpp
std::priority_queue<int, std::vector<int>, std::less<int>> pq;
```

`std::less<int>` means:

```cpp
a < b
```

For priority queue, this means:

```text
a has lower priority than b if a < b
```

So larger values have higher priority.

Top is largest.

---

## 4. Min Heap

```cpp
std::priority_queue<int, std::vector<int>, std::greater<int>> pq;
```

`std::greater<int>` means:

```cpp
a > b
```

For priority queue:

```text
a has lower priority than b if a > b
```

So larger values have lower priority.

Top is smallest.

Example:

```cpp
std::priority_queue<int, std::vector<int>, std::greater<int>> pq;

pq.push(3);
pq.push(10);
pq.push(5);

pq.top(); // 3
```

---

## 5. Compare with sort

For `sort`:

```cpp
std::sort(v.begin(), v.end(), [](int a, int b) {
    return a < b;
});
```

Meaning:

```text
a should come before b
```

So result is ascending.

For `priority_queue`:

```cpp
std::priority_queue<int, std::vector<int>, std::less<int>> pq;
```

Meaning:

```text
smaller value has lower priority
```

So larger value goes to top.

Summary:

```text
sort comparator: should a come before b?
priority_queue comparator: does a have lower priority than b?
```

---

## 6. Custom Comparator Struct

Example: task with larger priority number first.

```cpp
struct Task {
    std::string name;
    int priority;
};

struct Compare {
    bool operator()(const Task& a, const Task& b) const {
        return a.priority < b.priority;
    }
};
```

Usage:

```cpp
std::priority_queue<Task, std::vector<Task>, Compare> pq;
```

Why?

```cpp
return a.priority < b.priority;
```

means:

```text
a has lower priority if its priority number is smaller
```

Therefore larger priority number appears at top.

---

## 7. Custom Min Heap by Field

Example: pair `(node, distance)`, smallest distance first.

```cpp
using P = std::pair<int, int>; // node, distance

struct Compare {
    bool operator()(const P& a, const P& b) const {
        return a.second > b.second;
    }
};
```

Usage:

```cpp
std::priority_queue<P, std::vector<P>, Compare> pq;
```

Why?

```cpp
return a.second > b.second;
```

means:

```text
a has lower priority if its distance is larger
```

So smaller distance has higher priority.

This is common in Dijkstra.

---

## 8. Lambda Comparator

For `priority_queue`, lambda comparator needs special syntax.

```cpp
auto cmp = [](const auto& a, const auto& b) {
    return a.second > b.second;
};

std::priority_queue<
    std::pair<int, int>,
    std::vector<std::pair<int, int>>,
    decltype(cmp)
> pq(cmp);
```

Important:

```cpp
pq(cmp)
```

is needed because the comparator object must be passed into the priority queue constructor.

---

## 9. Why `decltype(cmp)`?

A lambda has a unique anonymous type.

So we cannot write its type directly.

We use:

```cpp
decltype(cmp)
```

to refer to the lambda's type.

Example:

```cpp
auto cmp = [](int a, int b) {
    return a > b;
};

std::priority_queue<int, std::vector<int>, decltype(cmp)> pq(cmp);
```

This creates a min heap.

---

## 10. Pair Default Ordering

`std::pair` has lexicographical comparison.

For max heap:

```cpp
std::priority_queue<std::pair<int, int>> pq;
```

Top is the largest pair by:

```text
first, then second
```

Example:

```cpp
(5, 1) > (3, 100)
(5, 10) > (5, 1)
```

So:

```cpp
pq.push({5, 1});
pq.push({3, 100});
pq.push({5, 10});

pq.top(); // (5, 10)
```

---

## 11. Common LeetCode Pattern: Top K Frequent

For top K frequent words, often use a min heap of size K.

Goal:

```text
keep best K elements
pop worst element when size > K
```

Suppose we want top frequency high first, and lexicographically smaller word first.

The "worst" among kept elements should be:

```text
lower frequency
or same frequency but lexicographically larger
```

For a min heap whose top is worst:

```cpp
struct Compare {
    bool operator()(const std::pair<std::string, int>& a,
                    const std::pair<std::string, int>& b) const {
        if (a.second == b.second) {
            return a.first < b.first;
        }
        return a.second > b.second;
    }
};
```

This one is tricky.

For `priority_queue`, `true` means `a` has lower priority than `b`.

We want top to be worst.

So "better" elements should have lower priority in this min-heap setup.

This deserves practice; when confused, test with two elements manually.

---

## 12. Easier Rule for LeetCode Heap Comparator

When writing `priority_queue` comparator, ask:

```text
Which element should be popped first?
```

Then design comparator so that element becomes `top()`.

Because `priority_queue` always pops `top()`.

For a min heap:

```cpp
return a > b;
```

Top is smallest.

For custom object:

```cpp
return a.score > b.score;
```

Top has smallest score.

---

## 13. Comparator Must Be Strict

Like `sort`, priority queue comparator should also be strict.

Bad:

```cpp
return a.priority <= b.priority;
```

Good:

```cpp
return a.priority < b.priority;
```

For equal elements:

```text
comp(a, a) must be false
```

---

## 14. priority_queue Does Not Support Iteration in Sorted Order

`priority_queue` only guarantees:

```cpp
pq.top()
```

is highest priority.

It does not provide sorted iteration.

To get elements in priority order:

```cpp
while (!pq.empty()) {
    auto x = pq.top();
    pq.pop();
}
```

This destroys the heap.

If you need sorted order and repeated traversal, consider:

```cpp
std::set
std::multiset
```

---

## 15. Common Interview Questions

### Q1. Why does `std::priority_queue<int>` give a max heap?

Because default comparator is `std::less<int>`.

For priority_queue, `less(a, b)` means `a` has lower priority than `b` when `a < b`.

Therefore larger elements have higher priority and appear at top.

---

### Q2. How do you create a min heap?

Use `std::greater<int>`:

```cpp
std::priority_queue<int, std::vector<int>, std::greater<int>> pq;
```

---

### Q3. What does the priority_queue comparator mean?

It returns true if the first argument has lower priority than the second.

---

### Q4. Why does lambda comparator require `decltype(cmp)` and `pq(cmp)`?

The lambda has an anonymous compiler-generated type, so `decltype(cmp)` names that type.

The comparator object `cmp` must be passed to the priority queue constructor because the priority queue needs an instance of that comparator.

---

### Q5. Is priority_queue iteration sorted?

No.

`priority_queue` only guarantees the top element has highest priority.

To get sorted priority order, repeatedly call `top()` and `pop()`.

---

## 16. Key Takeaways

- `priority_queue` is a heap adaptor.
- Default is max heap.
- `std::greater<int>` creates min heap.
- For `priority_queue`, comparator means lower priority.
- `sort` comparator means comes before.
- Lambda comparator needs `decltype(cmp)` and constructor argument `pq(cmp)`.
- Pair default comparison is lexicographical.
- Comparator must be strict.
- To debug comparator, test with two elements manually.
