# 15. STL Algorithms and Comparators

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. std::sort](#2-stdsort)
- [3. Comparator Meaning](#3-comparator-meaning)
- [4. Sorting Pair / Vector by Multiple Keys](#4-sorting-pair--vector-by-multiple-keys)
- [5. Strict Weak Ordering](#5-strict-weak-ordering)
- [6. Common Bad Comparator](#6-common-bad-comparator)
- [7. Lambda Comparator](#7-lambda-comparator)
- [8. Capturing Variables in Lambda](#8-capturing-variables-in-lambda)
- [9. stable_sort](#9-stable_sort)
- [10. partial_sort](#10-partial_sort)
- [11. nth_element](#11-nth_element)
- [12. max_element / min_element](#12-max_element--min_element)
- [13. lower_bound / upper_bound on Sorted Vector](#13-lower_bound--upper_bound-on-sorted-vector)
- [14. binary_search](#14-binary_search)
- [15. priority_queue Comparator](#15-priority_queue-comparator)
- [16. priority_queue Custom Comparator](#16-priority_queue-custom-comparator)
- [17. priority_queue with Pair](#17-priority_queue-with-pair)
- [18. priority_queue Min Heap with Custom Rule](#18-priority_queue-min-heap-with-custom-rule)
- [19. Comparator Direction: sort vs priority_queue](#19-comparator-direction-sort-vs-priority_queue)
- [20. Common Interview Questions](#20-common-interview-questions)
- [21. Key Takeaways](#21-key-takeaways)

## Related Code Trap

- [STL Algorithms / Comparators Demo](../code_traps/stl_algorithms_comparators.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

STL algorithms usually work on iterator ranges:

```cpp
[begin, end)
```

Example:

```cpp
std::sort(v.begin(), v.end());
```

The algorithm does not care whether `v` is a container object. It only needs iterators.

Comparators define how elements are ordered.

Example:

```cpp
std::sort(v.begin(), v.end(), [](int a, int b) {
    return a < b;
});
```

The comparator should return true if `a` should come before `b`.

---

## 2. std::sort

```cpp
std::sort(v.begin(), v.end());
```

Default order is ascending.

Example:

```cpp
std::vector<int> v = {3, 1, 2};
std::sort(v.begin(), v.end());
```

Result:

```text
1 2 3
```

For descending order:

```cpp
std::sort(v.begin(), v.end(), std::greater<int>());
```

or:

```cpp
std::sort(v.begin(), v.end(), [](int a, int b) {
    return a > b;
});
```

---

## 3. Comparator Meaning

For `std::sort`, comparator:

```cpp
comp(a, b)
```

means:

```text
should a come before b?
```

Ascending:

```cpp
return a < b;
```

Descending:

```cpp
return a > b;
```

Important:

```text
Comparator does not mean "should swap".
Comparator means "is a ordered before b".
```

---

## 4. Sorting Pair / Vector by Multiple Keys

Example: sort pairs by first ascending, then second ascending.

```cpp
std::vector<std::pair<int, int>> v;

std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) {
    if (a.first != b.first) {
        return a.first < b.first;
    }
    return a.second < b.second;
});
```

Equivalent shorter version:

```cpp
std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) {
    return std::tie(a.first, a.second) < std::tie(b.first, b.second);
});
```

Sort by first ascending, second descending:

```cpp
std::sort(v.begin(), v.end(), [](const auto& a, const auto& b) {
    if (a.first != b.first) {
        return a.first < b.first;
    }
    return a.second > b.second;
});
```

---

## 5. Strict Weak Ordering

A comparator must define a consistent ordering.

For `std::sort`, `std::set`, `std::map`, and other ordered structures, comparator should satisfy strict weak ordering.

Most important rule:

```text
comp(a, a) must be false
```

Good:

```cpp
return a < b;
```

Bad:

```cpp
return a <= b;
```

Why?

For equal values:

```cpp
comp(a, a) == true
```

This violates the ordering rule.

Bad comparator can cause undefined behavior in `sort` and incorrect behavior in ordered containers.

---

## 6. Common Bad Comparator

Bad:

```cpp
std::sort(v.begin(), v.end(), [](int a, int b) {
    return a <= b;
});
```

Correct:

```cpp
std::sort(v.begin(), v.end(), [](int a, int b) {
    return a < b;
});
```

For descending:

Bad:

```cpp
return a >= b;
```

Correct:

```cpp
return a > b;
```

---

## 7. Lambda Comparator

Lambda syntax:

```cpp
[](const auto& a, const auto& b) {
    return a < b;
}
```

General form:

```cpp
[capture](parameters) -> return_type {
    body
}
```

Usually for comparators:

```cpp
[](const auto& a, const auto& b) {
    return condition;
}
```

Example:

```cpp
std::sort(words.begin(), words.end(), [](const std::string& a, const std::string& b) {
    return a.size() < b.size();
});
```

---

## 8. Capturing Variables in Lambda

Example:

```cpp
int target = 10;

std::sort(v.begin(), v.end(), [target](int a, int b) {
    return std::abs(a - target) < std::abs(b - target);
});
```

`[target]` captures by value.

```cpp
[&target]
```

captures by reference.

For comparators, prefer value capture unless you specifically need reference semantics.

---

## 9. stable_sort

```cpp
std::stable_sort(v.begin(), v.end(), comp);
```

`stable_sort` preserves the relative order of equivalent elements.

Example:

```text
Before:
(A, score=90), (B, score=80), (C, score=90)

Sort by score descending with stable_sort:
(A, 90), (C, 90), (B, 80)
```

A and C keep their original relative order.

`std::sort` is not stable.

---

## 10. partial_sort

```cpp
std::partial_sort(v.begin(), v.begin() + k, v.end());
```

This sorts only the first `k` elements as the smallest `k`.

Example:

```cpp
std::vector<int> v = {9, 1, 5, 3, 7};

std::partial_sort(v.begin(), v.begin() + 3, v.end());
```

After this, first 3 elements are the smallest 3 in sorted order:

```text
1 3 5
```

The remaining elements are unspecified order.

Useful for top-k when you need sorted top-k.

---

## 11. nth_element

```cpp
std::nth_element(v.begin(), v.begin() + k, v.end());
```

After this:

```text
v[k] is the element that would be at index k in sorted order
elements before k <= v[k]
elements after k >= v[k]
```

But the two sides are not sorted.

Example:

```cpp
std::vector<int> v = {9, 1, 5, 3, 7};

std::nth_element(v.begin(), v.begin() + 2, v.end());
```

Now `v[2]` is the third smallest element.

Useful for quickselect-style problems.

Average linear time.

---

## 12. max_element / min_element

```cpp
auto it = std::max_element(v.begin(), v.end());
auto it2 = std::min_element(v.begin(), v.end());
```

Return iterators.

Example:

```cpp
if (it != v.end()) {
    std::cout << *it << std::endl;
}
```

With comparator:

```cpp
auto it = std::max_element(words.begin(), words.end(),
    [](const auto& a, const auto& b) {
        return a.size() < b.size();
    });
```

For `max_element`, comparator still means:

```text
a is less than b
```

So this finds the word with largest size.

---

## 13. lower_bound / upper_bound on Sorted Vector

For sorted vector:

```cpp
std::lower_bound(v.begin(), v.end(), x);
std::upper_bound(v.begin(), v.end(), x);
```

Need sorted range.

```cpp
lower_bound: first >= x
upper_bound: first > x
```

Example:

```cpp
std::vector<int> v = {1, 3, 5, 7};

auto it = std::lower_bound(v.begin(), v.end(), 4); // points to 5
```

For `std::set` / `std::map`, prefer member function:

```cpp
s.lower_bound(x);
mp.lower_bound(key);
```

because tree member lower_bound is O(log n).

---

## 14. binary_search

```cpp
bool exists = std::binary_search(v.begin(), v.end(), x);
```

Range must be sorted.

Returns true/false only.

If you need position, use `lower_bound`.

---

## 15. priority_queue Comparator

Default:

```cpp
std::priority_queue<int> pq;
```

is max heap.

Top is largest.

```cpp
pq.top();
```

For min heap:

```cpp
std::priority_queue<int, std::vector<int>, std::greater<int>> pq;
```

---

## 16. priority_queue Custom Comparator

Important mental model:

For `priority_queue`, comparator is a bit inverted.

```cpp
std::priority_queue<T, std::vector<T>, Compare>
```

The top element is the one considered "highest priority".

Default:

```cpp
std::less<int>
```

creates max heap.

Because lower-priority elements are ordered below higher-priority ones.

Example min heap:

```cpp
std::greater<int>
```

puts smaller elements on top.

---

## 17. priority_queue with Pair

Max heap by frequency:

```cpp
using P = std::pair<int, std::string>;

struct Compare {
    bool operator()(const P& a, const P& b) const {
        return a.first < b.first;
    }
};

std::priority_queue<P, std::vector<P>, Compare> pq;
```

Why?

For priority_queue comparator:

```text
return true means a has lower priority than b
```

So:

```cpp
return a.first < b.first;
```

means smaller frequency has lower priority.

Largest frequency appears at top.

---

## 18. priority_queue Min Heap with Custom Rule

Example: top should be smallest distance.

```cpp
struct Compare {
    bool operator()(const std::pair<int, int>& a,
                    const std::pair<int, int>& b) const {
        return a.second > b.second;
    }
};
```

This means:

```text
a has lower priority if its distance is larger
```

So smaller distance has higher priority and goes to top.

---

## 19. Comparator Direction: sort vs priority_queue

For `sort`:

```cpp
return a < b;
```

means:

```text
a should come before b
```

For `priority_queue`:

```cpp
return a < b;
```

means:

```text
a has lower priority than b
```

This is why `std::less<int>` creates max heap.

Remember:

```text
sort comparator: before relation
priority_queue comparator: lower-priority relation
```

---

## 20. Common Interview Questions

### Q1. What does a sort comparator mean?

It returns true if the first argument should come before the second argument.

For ascending order, use `a < b`.

---

### Q2. Why is `return a <= b` wrong in comparator?

Because comparator must be strict.

For equal elements, `comp(a, a)` must be false.

`a <= a` is true, which violates strict weak ordering.

---

### Q3. What is stable_sort?

`stable_sort` preserves the relative order of equivalent elements.

`sort` does not guarantee this.

---

### Q4. What does nth_element do?

It rearranges the range so that the nth element is the same element that would appear there in sorted order.

Elements before it are not greater than it, and elements after it are not less than it, but both sides are not sorted.

---

### Q5. Difference between sort comparator and priority_queue comparator?

For `sort`, comparator means the first element should come before the second.

For `priority_queue`, comparator means the first element has lower priority than the second.

That is why `std::less<int>` gives a max heap.

---

## 21. Key Takeaways

- STL algorithms work on iterator ranges.
- `sort` requires random-access iterators.
- `sort` comparator means "comes before".
- Comparator must be strict: never use `<=` or `>=`.
- `stable_sort` preserves equivalent order.
- `nth_element` is useful for kth element / quickselect.
- `lower_bound` requires sorted range.
- `priority_queue` comparator means lower priority.
- `std::less<int>` gives max heap.
- `std::greater<int>` gives min heap.
