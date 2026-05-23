# 10. STL Containers Overview

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. vector](#2-vector)
- [3. vector size vs capacity](#3-vector-size-vs-capacity)
- [4. vector reallocation](#4-vector-reallocation)
- [5. deque](#5-deque)
- [6. list](#6-list)
- [7. forward_list](#7-forward_list)
- [8. map](#8-map)
- [9. set](#9-set)
- [10. unordered_map](#10-unordered_map)
- [11. unordered_map rehash](#11-unordered_map-rehash)
- [12. priority_queue](#12-priority_queue)
- [13. stack and queue](#13-stack-and-queue)
- [14. Iterator Stability Overview](#14-iterator-stability-overview)
- [15. Cache Locality](#15-cache-locality)
- [16. Container Selection Guide](#16-container-selection-guide)
- [17. Common Interview Questions](#17-common-interview-questions)
- [18. Key Takeaways](#18-key-takeaways)

## Related Code Trap

- [STL Containers Overview Demo](../code_traps/stl_containers_overview.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

STL containers provide common data structures with well-defined complexity and iterator behavior.

The most common containers:

```cpp
std::vector<T>
std::deque<T>
std::list<T>
std::map<K, V>
std::set<T>
std::unordered_map<K, V>
std::unordered_set<T>
std::priority_queue<T>
```

Choosing a container depends on:

- random access requirement
- insertion/deletion pattern
- whether ordering is needed
- whether stable references/iterators are needed
- memory locality
- lookup pattern

---

## 2. vector

```cpp
std::vector<int> v;
```

`vector` is a dynamic array.

Conceptually:

```text
contiguous memory
+---+---+---+---+---+
| 1 | 2 | 3 | 4 | 5 |
+---+---+---+---+---+
```

Key properties:

- contiguous memory
- fast random access: `O(1)`
- fast push_back amortized: `O(1)`
- insertion/deletion in the middle: `O(n)`
- excellent cache locality
- reallocation can invalidate pointers/references/iterators

Use `vector` by default unless you have a specific reason not to.

---

## 3. vector size vs capacity

```cpp
std::vector<int> v;
```

`size()`:

```text
number of constructed elements
```

`capacity()`:

```text
amount of allocated storage
```

Example:

```cpp
v.reserve(100);
```

After this:

```text
capacity >= 100
size == 0
```

`reserve()` allocates memory but does not create elements.

```cpp
v.resize(100);
```

After this:

```text
size == 100
```

`resize()` creates or destroys elements to match the requested size.

---

## 4. vector reallocation

When `vector` capacity is full and you push more elements, it may allocate a bigger memory block.

Process:

```text
1. allocate new larger storage
2. move/copy old elements into new storage
3. destroy old elements
4. deallocate old storage
```

This can invalidate:

- iterators
- pointers
- references

Example:

```cpp
std::vector<int> v = {1, 2, 3};
int* p = &v[0];

v.push_back(4); // may reallocate

// p may now be dangling
```

This is one of the most important vector interview topics.

---

## 5. deque

```cpp
std::deque<int> dq;
```

`deque` stands for double-ended queue.

It supports efficient insertion/removal at both ends:

```cpp
dq.push_front(1);
dq.push_back(2);
dq.pop_front();
dq.pop_back();
```

Conceptually, it is not one contiguous array like vector.

It is often implemented as multiple fixed-size blocks plus an index map.

```text
map of blocks
   |
   v
+------+     +------+     +------+
|block |     |block |     |block |
+------+     +------+     +------+
```

Key properties:

- random access: `O(1)`
- push_front / push_back: `O(1)` amortized
- not contiguous memory
- worse cache locality than vector
- good when frequent front insertion/removal is needed

Use `deque` when you need efficient operations at both ends.

---

## 6. list

```cpp
std::list<int> lst;
```

`list` is a doubly linked list.

Conceptually:

```text
[node] <-> [node] <-> [node]
```

Key properties:

- insertion/deletion given an iterator: `O(1)`
- no random access
- accessing the nth element: `O(n)`
- poor cache locality
- each element has extra pointer overhead
- iterators and references are stable for most operations

Important:

```text
list is not automatically faster than vector.
```

Even though insertion/deletion is `O(1)` once you have the iterator, finding the position is often `O(n)`, and cache locality is bad.

---

## 7. forward_list

```cpp
std::forward_list<int> fl;
```

Singly linked list.

Key properties:

- smaller memory overhead than `list`
- only forward iteration
- no `size()` in old standards / may not be constant depending implementation and standard context
- useful in very specific memory-sensitive cases

Most interviews focus more on `vector`, `list`, `map`, and `unordered_map`.

---

## 8. map

```cpp
std::map<std::string, int> mp;
```

`map` is an ordered associative container.

It is usually implemented as a balanced binary search tree, commonly red-black tree.

Key properties:

- keys are sorted
- lookup: `O(log n)`
- insert/delete: `O(log n)`
- iterating gives keys in sorted order
- stable iterators/references except erased elements
- higher memory overhead than unordered_map

Example:

```cpp
std::map<int, std::string> mp;
mp[3] = "three";
mp[1] = "one";

for (auto& [k, v] : mp) {
    std::cout << k << " ";
}
```

Output:

```text
1 3
```

Use `map` when you need sorted order or ordered operations like `lower_bound`.

---

## 9. set

```cpp
std::set<int> s;
```

`set` is like `map` but stores only keys.

Usually balanced tree.

Key properties:

- sorted unique keys
- lookup/insert/delete: `O(log n)`
- supports ordered operations
- stable iterators except erased elements

Use `set` when you need ordered unique elements.

---

## 10. unordered_map

```cpp
std::unordered_map<std::string, int> mp;
```

`unordered_map` is a hash table.

Key properties:

- average lookup: `O(1)`
- average insert/delete: `O(1)`
- worst case: `O(n)`
- no sorted order
- rehash can invalidate iterators
- performance depends on hash quality and load factor

Conceptually:

```text
buckets:
0 -> [key-value nodes]
1 -> [key-value nodes]
2 -> [key-value nodes]
...
```

Use `unordered_map` when you need fast lookup and do not care about order.

---

## 11. unordered_map rehash

`unordered_map` stores elements in buckets.

When load factor becomes too high, it may rehash:

```text
1. allocate more buckets
2. redistribute existing elements based on hash
```

This can invalidate iterators.

References and pointers to elements are generally not invalidated by rehash in standard node-based unordered containers, but iterators are invalidated.

Still, be careful when storing iterators across insertions.

This topic deserves a separate deep dive.

---

## 12. priority_queue

```cpp
std::priority_queue<int> pq;
```

`priority_queue` is a container adaptor.

By default, it is a max heap.

```cpp
pq.push(3);
pq.push(10);
pq.push(5);

pq.top(); // 10
```

Key properties:

- top: `O(1)`
- push: `O(log n)`
- pop: `O(log n)`
- no iteration in priority order
- default underlying container is `vector`

Min heap:

```cpp
std::priority_queue<int, std::vector<int>, std::greater<int>> minHeap;
```

Use it for top-k, scheduling, Dijkstra, event simulation.

---

## 13. stack and queue

`stack` and `queue` are container adaptors.

```cpp
std::stack<int> st;
std::queue<int> q;
```

They usually use `deque` by default.

`stack`:

```cpp
push
pop
top
```

`queue`:

```cpp
push
pop
front
back
```

They restrict the interface to match stack/queue semantics.

---

## 14. Iterator Stability Overview

Rough idea:

```text
vector:
- reallocation invalidates all iterators/references/pointers
- insertion/deletion in middle invalidates affected range

deque:
- more complicated
- operations at ends may invalidate iterators
- references are often more stable than vector but rules are subtle

list:
- insertion does not invalidate existing iterators/references
- erase invalidates only erased elements

map/set:
- insertion does not invalidate iterators/references
- erase invalidates only erased elements

unordered_map/unordered_set:
- rehash invalidates iterators
- erase invalidates erased element
```

Iterator invalidation is a major interview topic and should be studied separately.

---

## 15. Cache Locality

Cache locality is often ignored but very important for performance.

`vector` has excellent cache locality because elements are contiguous.

```text
vector:
+---+---+---+---+
| A | B | C | D |
+---+---+---+---+
```

`list` has poor cache locality because nodes are scattered on the heap.

```text
node -> node -> node
```

Even if linked list insertion is `O(1)`, vector can often be faster in practice due to cache locality.

Interview point:

```text
Big-O does not capture memory locality.
```

---

## 16. Container Selection Guide

### Default choice

```cpp
std::vector<T>
```

Use vector unless you have a reason not to.

---

### Need fast push/pop at both ends

```cpp
std::deque<T>
```

---

### Need stable iterators and frequent erase/insert given iterator

```cpp
std::list<T>
```

But be careful: list often loses due to poor cache locality.

---

### Need sorted keys / lower_bound / ordered iteration

```cpp
std::map<K, V>
std::set<T>
```

---

### Need fastest average lookup and no order

```cpp
std::unordered_map<K, V>
std::unordered_set<T>
```

---

### Need repeatedly access max/min

```cpp
std::priority_queue<T>
```

---

## 17. Common Interview Questions

### Q1. Why is vector usually the default container?

Because it has contiguous memory, fast random access, amortized constant-time push_back, and excellent cache locality.

It is often faster in practice than linked structures.

---

### Q2. Difference between vector and deque?

`vector` stores elements contiguously and has excellent cache locality.

`deque` supports efficient push/pop at both front and back, but is usually implemented as segmented storage, so it is not contiguous like vector.

---

### Q3. Difference between map and unordered_map?

`map` is ordered and usually implemented as a balanced tree. Operations are `O(log n)` and iteration is sorted.

`unordered_map` is hash-based. Operations are average `O(1)`, but no ordering is guaranteed.

---

### Q4. Why can list insertion be O(1) but still slower than vector?

Because list nodes are allocated separately and have poor cache locality. Also, finding the insertion position is often `O(n)`.

Vector may move elements but benefits from contiguous memory and cache efficiency.

---

### Q5. What is rehash in unordered_map?

Rehash means allocating a new bucket array and redistributing elements based on their hash values.

It happens when the load factor becomes too high and can invalidate iterators.

---

### Q6. What is iterator invalidation?

Iterator invalidation means an iterator, pointer, or reference no longer safely refers to the intended element after a container operation.

For example, vector reallocation invalidates all existing iterators, pointers, and references.

---

## 18. Key Takeaways

- Use `vector` by default.
- `vector` is contiguous and cache-friendly.
- `deque` supports efficient operations at both ends but is not contiguous.
- `list` has stable iterators but poor cache locality.
- `map` is ordered and tree-based.
- `unordered_map` is hash-based and average O(1).
- `priority_queue` is heap-based.
- Big-O is not enough; memory locality matters.
- Iterator invalidation rules are critical in C++.
