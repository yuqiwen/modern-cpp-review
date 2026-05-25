# 12. Iterator Invalidation and unordered_map Rehash

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Reference / Pointer / Iterator](#2-reference--pointer--iterator)
- [3. vector Reallocation](#3-vector-reallocation)
- [4. vector push_back Without Reallocation](#4-vector-push_back-without-reallocation)
- [5. vector insert](#5-vector-insert)
- [6. vector erase](#6-vector-erase)
- [7. list Invalidation](#7-list-invalidation)
- [8. map / set Invalidation](#8-map--set-invalidation)
- [9. unordered_map Internal Model](#9-unordered_map-internal-model)
- [10. unordered_map Rehash](#10-unordered_map-rehash)
- [11. unordered_map Rehash: Iterator vs Reference](#11-unordered_map-rehash-iterator-vs-reference)
- [12. unordered_map Erase](#12-unordered_map-erase)
- [13. unordered_map insert and rehash](#13-unordered_map-insert-and-rehash)
- [14. bucket_count / load_factor / max_load_factor](#14-bucket_count--load_factor--max_load_factor)
- [15. deque Invalidation Overview](#15-deque-invalidation-overview)
- [16. Iterator Invalidation Summary Table](#16-iterator-invalidation-summary-table)
- [17. Common Bugs](#17-common-bugs)
- [18. Interview Questions](#18-interview-questions)
- [19. Key Takeaways](#19-key-takeaways)

## Related Code Trap

- [Iterator Invalidation / unordered_map Rehash Demo](../code_traps/iterator_invalidation_unordered_map.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

Iterator invalidation means an iterator can no longer be safely used after a container operation.

Pointer/reference invalidation means a pointer or reference no longer safely refers to the original object.

The key question is:

```text
Did the element object get destroyed, moved, or did the container traversal structure change?
```

Different containers have different invalidation rules because they have different internal structures.

---

## 2. Reference / Pointer / Iterator

Example:

```cpp
std::vector<int> v = {1, 2, 3};

int& r = v[0];
int* p = &v[0];
auto it = v.begin();
```

Conceptually:

```text
r  = alias to v[0]
p  = address of v[0]
it = iterator object that knows how to find/traverse v[0]
```

If the actual `v[0]` object is moved or destroyed, all three become invalid.

But in some node-based containers, the element object may stay at the same address while the iterator traversal structure changes.

That is why references/pointers and iterators may have different invalidation rules.

---

## 3. vector Reallocation

`std::vector` stores elements contiguously.

```text
old storage:
+---+---+---+
| 1 | 2 | 3 |
+---+---+---+
```

If capacity is full and we push another element:

```cpp
v.push_back(4);
```

Vector may reallocate:

```text
1. allocate new larger storage
2. move/copy old elements into new storage
3. destroy old elements
4. deallocate old storage
```

After reallocation:

```text
new storage:
+---+---+---+---+
| 1 | 2 | 3 | 4 |
+---+---+---+---+
```

Old references/pointers/iterators still point to old storage, which is no longer valid.

Rule:

```text
vector reallocation invalidates all iterators, pointers, and references.
```

---

## 4. vector push_back Without Reallocation

If capacity is enough:

```cpp
std::vector<int> v;
v.reserve(10);

v.push_back(1);
v.push_back(2);

int& r = v[0];
int* p = &v[0];
auto it = v.begin();

v.push_back(3);
```

No reallocation happens.

Then references/pointers/iterators to existing elements usually remain valid.

But:

```text
old end() is invalid/stale because the end position changed.
```

---

## 5. vector insert

```cpp
std::vector<int> v = {1, 2, 4};
auto it = v.begin() + 2; // points to 4

v.insert(it, 3);
```

If reallocation happens:

```text
all iterators, pointers, and references are invalidated
```

If no reallocation happens:

```text
elements at or after insertion point may be shifted
therefore iterators/references/pointers at or after insertion point are invalidated
elements before insertion point remain valid
```

---

## 6. vector erase

```cpp
std::vector<int> v = {1, 2, 3, 4};
auto it = v.begin() + 1; // points to 2

v.erase(it);
```

Elements after erased position shift left.

Rule:

```text
iterators/references/pointers at or after erased position are invalidated
ones before erased position remain valid
```

Correct loop:

```cpp
for (auto it = v.begin(); it != v.end(); ) {
    if (shouldErase(*it)) {
        it = v.erase(it);
    } else {
        ++it;
    }
}
```

`erase` returns the next valid iterator.

---

## 7. list Invalidation

`std::list` is node-based.

```text
node <-> node <-> node
```

Insertion:

```cpp
lst.insert(pos, value);
```

Usually does not invalidate existing iterators/references.

Erase:

```cpp
lst.erase(it);
```

Invalidates only the erased element.

Why?

Because other nodes stay at the same addresses.

Rule:

```text
list insertion does not invalidate existing iterators/references.
list erase invalidates only erased elements.
```

---

## 8. map / set Invalidation

`std::map` and `std::set` are tree-based node containers.

Insertion:

```cpp
mp.insert({key, value});
```

Does not invalidate existing iterators/references.

Erase:

```cpp
mp.erase(key);
```

Invalidates only erased elements.

Why?

Tree links may change due to rebalancing, but element nodes generally stay valid.

Rule:

```text
map/set insertion does not invalidate existing iterators/references.
erase invalidates only erased elements.
```

---

## 9. unordered_map Internal Model

`std::unordered_map` is a hash table.

Classic model:

```text
bucket array
+----------+
| bucket 0 | -> node(k1, v1) -> node(k2, v2)
| bucket 1 | -> nullptr
| bucket 2 | -> node(k3, v3)
+----------+
```

Each element is stored in a node.

Buckets point to chains of nodes.

This is commonly described as:

```text
bucket array + linked lists / chains
```

The standard does not force one exact implementation, but this model is useful and close to common implementations.

---

## 10. unordered_map Rehash

Rehash happens when the bucket array needs to grow or when requested explicitly.

Example:

```cpp
mp.reserve(1000);
```

or insertions trigger growth.

Conceptual process:

```text
1. allocate a new bucket array
2. recompute bucket index for each node
3. relink nodes into new buckets
```

Before:

```text
old buckets:
bucket 0 -> node A -> node B
bucket 1 -> node C
```

After:

```text
new buckets:
bucket 2 -> node B
bucket 5 -> node A -> node C
```

The important point:

```text
nodes themselves usually stay at the same addresses
bucket structure changes
node next links may change
```

---

## 11. unordered_map Rehash: Iterator vs Reference

Before rehash:

```cpp
auto it = mp.find("apple");
int& r = mp["apple"];
int* p = &mp["apple"];
```

After rehash:

```cpp
mp.reserve(10000);
```

Rules:

```text
iterators are invalidated
references and pointers to elements remain valid
```

Why?

The reference/pointer points to the element inside a node.

The node still exists.

But the iterator depends on bucket traversal structure, and rehash changed that structure.

So:

```cpp
r = 10;  // OK
*p = 20; // OK

it->second = 30; // wrong: iterator invalid
```

---

## 12. unordered_map Erase

Erase destroys the node.

```cpp
auto it = mp.find("apple");
int& r = it->second;

mp.erase("apple");
```

After erase:

```text
it invalid
r invalid
pointers to that element invalid
```

Because the element object was destroyed.

Rule:

```text
erase invalidates iterators/references/pointers to erased elements.
```

Other elements remain valid unless rehash happens due to other operations.

---

## 13. unordered_map insert and rehash

Insertion may or may not trigger rehash.

If no rehash happens:

```text
existing iterators/references/pointers remain valid
```

If rehash happens:

```text
iterators invalidated
references/pointers remain valid
```

Best practice:

```cpp
mp.reserve(expected_count);
```

before inserting many elements.

This reduces rehash frequency.

---

## 14. bucket_count / load_factor / max_load_factor

`unordered_map` has buckets.

```cpp
mp.bucket_count()
```

returns number of buckets.

```cpp
mp.load_factor()
```

roughly:

```text
size / bucket_count
```

```cpp
mp.max_load_factor()
```

controls when rehash should happen.

Example:

```cpp
std::unordered_map<std::string, int> mp;
mp.max_load_factor(0.7);
mp.reserve(1000);
```

This asks the map to allocate enough buckets to store about 1000 elements with load factor no more than 0.7.

---

## 15. deque Invalidation Overview

`std::deque` is segmented storage.

It is more complicated than vector.

Roughly:

```text
map of blocks -> fixed-size blocks of elements
```

Properties:

- random access O(1)
- push_front / push_back efficient
- not contiguous
- invalidation rules are more subtle than vector

General interview-level rule:

```text
Do not assume deque iterators remain valid after structural modifications.
References are often more stable than vector for end operations, but check exact rules when needed.
```

For most interviews, know vector/list/map/unordered_map rules better.

---

## 16. Iterator Invalidation Summary Table

```text
vector:
  reallocation -> all iterators/references/pointers invalid
  insert no reallocation -> at/after insertion invalid
  erase -> at/after erased position invalid

list:
  insert -> existing iterators/references remain valid
  erase -> only erased elements invalid

map/set:
  insert -> existing iterators/references remain valid
  erase -> only erased elements invalid

unordered_map/set:
  rehash -> iterators invalid, references/pointers remain valid
  erase -> erased elements invalid
  insert -> may trigger rehash
```

---

## 17. Common Bugs

### Bug 1: vector pointer after reallocation

```cpp
std::vector<int> v = {1, 2, 3};
int* p = &v[0];

v.push_back(4); // may reallocate

std::cout << *p; // undefined behavior if reallocated
```

---

### Bug 2: vector erase loop

Wrong:

```cpp
for (auto it = v.begin(); it != v.end(); ++it) {
    if (*it % 2 == 0) {
        v.erase(it);
    }
}
```

Correct:

```cpp
for (auto it = v.begin(); it != v.end(); ) {
    if (*it % 2 == 0) {
        it = v.erase(it);
    } else {
        ++it;
    }
}
```

---

### Bug 3: unordered_map iterator after rehash

Wrong:

```cpp
auto it = mp.find("apple");

mp.reserve(10000); // rehash

it->second = 5; // invalid iterator
```

Correct:

```cpp
auto key = it->first;

mp.reserve(10000);

auto newIt = mp.find(key);
newIt->second = 5;
```

Or keep a reference/pointer if you specifically need the element value and know it will not be erased:

```cpp
int& value = mp["apple"];
mp.reserve(10000);
value = 5; // OK
```

---

## 18. Interview Questions

### Q1. What is iterator invalidation?

Iterator invalidation means an iterator can no longer be safely used after a container operation because the element or the container traversal structure it depends on changed.

---

### Q2. Why does vector reallocation invalidate references?

Because vector stores elements contiguously.

When it reallocates, it moves or copies elements into new storage and destroys the old elements.

References still refer to old destroyed objects, so they become invalid.

---

### Q3. Why does unordered_map rehash invalidate iterators but not references?

Because unordered_map is node-based.

Rehash changes bucket structure and relinks nodes, so iterators depending on traversal structure become invalid.

But element nodes remain at the same addresses, so references and pointers to elements remain valid.

---

### Q4. What does erase invalidate?

Erase destroys the erased element.

Therefore iterators, references, and pointers to erased elements become invalid.

For vector, erase also shifts later elements, so elements at or after the erased position are invalidated.

For node-based containers like list/map/unordered_map, erase usually invalidates only erased elements.

---

### Q5. How can we reduce unordered_map rehash?

Use `reserve(expected_size)` before inserting many elements.

You can also adjust `max_load_factor`.

---

## 19. Key Takeaways

- Invalidation depends on container structure.
- Vector reallocation moves elements, invalidating everything.
- Vector erase shifts elements, invalidating at/after erase point.
- Node-based containers keep element nodes stable.
- list/map insertion does not invalidate existing iterators.
- unordered_map rehash invalidates iterators but not references/pointers.
- unordered_map erase invalidates erased elements.
- Use `reserve` to reduce vector reallocation and unordered_map rehash.
