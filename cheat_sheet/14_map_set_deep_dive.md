# 14. map and set Deep Dive

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. map](#2-map)
- [3. set](#3-set)
- [4. map vs set](#4-map-vs-set)
- [5. Ordered Iteration](#5-ordered-iteration)
- [6. Why O(log n)?](#6-why-olog-n)
- [7. Comparator](#7-comparator)
- [8. Custom Comparator](#8-custom-comparator)
- [9. Comparator Must Define Strict Weak Ordering](#9-comparator-must-define-strict-weak-ordering)
- [10. find](#10-find)
- [11. operator[] for map](#11-operator-for-map)
- [12. at](#12-at)
- [13. insert / emplace / try_emplace / insert_or_assign](#13-insert--emplace--try_emplace--insert_or_assign)
- [14. lower_bound](#14-lower_bound)
- [15. upper_bound](#15-upper_bound)
- [16. lower_bound vs upper_bound](#16-lower_bound-vs-upper_bound)
- [17. equal_range](#17-equal_range)
- [18. Range Query](#18-range-query)
- [19. Iterator Stability](#19-iterator-stability)
- [20. Why map keys are const](#20-why-map-keys-are-const)
- [21. multimap / multiset](#21-multimap--multiset)
- [22. map vs unordered_map](#22-map-vs-unordered_map)
- [23. Common Interview Questions](#23-common-interview-questions)
- [24. Key Takeaways](#24-key-takeaways)

## Related Code Trap

- [map and set Deep Dive Demo](../code_traps/map_set_deep_dive.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

`std::map`, `std::set`, `std::multimap`, and `std::multiset` are ordered associative containers.

They are usually implemented as balanced binary search trees, commonly red-black trees.

Key properties:

- keys are sorted
- lookup: O(log n)
- insertion: O(log n)
- deletion: O(log n)
- iteration is in sorted order
- iterators/references are stable except erased elements
- support ordered queries like `lower_bound`, `upper_bound`, and range queries

---

## 2. map

```cpp
std::map<Key, Value> mp;
```

`map` stores key-value pairs.

Each key is unique.

Example:

```cpp
std::map<std::string, int> mp;

mp["apple"] = 3;
mp["banana"] = 5;
```

The element type is:

```cpp
std::pair<const Key, Value>
```

For example:

```cpp
std::pair<const std::string, int>
```

The key is `const` inside the map element because changing a key in place could break the tree ordering.

---

## 3. set

```cpp
std::set<T> s;
```

`set` stores unique keys only.

Example:

```cpp
std::set<int> s;
s.insert(3);
s.insert(1);
s.insert(2);
```

Iteration order:

```text
1 2 3
```

Use `set` when you need sorted unique values.

---

## 4. map vs set

```cpp
std::map<K, V>
```

stores:

```text
key -> value
```

```cpp
std::set<T>
```

stores:

```text
key only
```

`set<T>` is conceptually similar to:

```cpp
map<T, nothing>
```

But implemented as a dedicated container.

---

## 5. Ordered Iteration

Unlike `unordered_map`, `map` iterates in sorted key order.

Example:

```cpp
std::map<int, std::string> mp;
mp[3] = "three";
mp[1] = "one";
mp[2] = "two";

for (const auto& [k, v] : mp) {
    std::cout << k << " ";
}
```

Output:

```text
1 2 3
```

This is one of the biggest reasons to use `map`.

---

## 6. Why O(log n)?

Balanced binary search tree.

Conceptually:

```text
        4
      /   \
     2     6
    / \   / \
   1   3 5   7
```

At each step, comparison decides whether to go left or right.

Tree height is O(log n), so lookup/insert/delete are O(log n).

A red-black tree keeps the tree approximately balanced.

---

## 7. Comparator

`map` and `set` use a comparator to maintain order.

Default:

```cpp
std::less<Key>
```

which usually means ascending order using `<`.

Example:

```cpp
std::map<int, std::string> mp;
```

is equivalent to:

```cpp
std::map<int, std::string, std::less<int>> mp;
```

For descending order:

```cpp
std::map<int, std::string, std::greater<int>> mp;
```

For set:

```cpp
std::set<int, std::greater<int>> s;
```

---

## 8. Custom Comparator

Example:

```cpp
struct Person {
    std::string name;
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

std::set<Person, ByAgeThenName> people;
```

The comparator defines the ordering.

Important:

```text
For set/map, equality is based on comparator, not operator==.
```

Two keys `a` and `b` are considered equivalent if:

```cpp
!comp(a, b) && !comp(b, a)
```

---

## 9. Comparator Must Define Strict Weak Ordering

A comparator must be consistent.

For `std::set` / `std::map`, comparator must define strict weak ordering.

A correct comparator:

```cpp
return a < b;
```

Bad comparator:

```cpp
return a <= b;
```

Why bad?

For equal elements:

```cpp
comp(a, a) == true
```

This violates strict weak ordering.

A comparator must satisfy:

```text
comp(a, a) must be false
```

If comparator is invalid, tree behavior can become incorrect.

---

## 10. find

```cpp
auto it = mp.find(key);
```

If found:

```cpp
it != mp.end()
```

If not found:

```cpp
it == mp.end()
```

`find` is O(log n).

For `set`:

```cpp
auto it = s.find(value);
```

---

## 11. operator[] for map

`map` has `operator[]`, similar to `unordered_map`.

```cpp
mp[key]
```

If key exists:

```text
returns reference to mapped value
```

If key does not exist:

```text
inserts key with default-constructed mapped value
returns reference to value
```

Example:

```cpp
std::map<std::string, int> mp;
std::cout << mp["apple"] << std::endl;
```

This inserts:

```text
"apple" -> 0
```

Common bug:

```cpp
if (mp["apple"] > 0) {}
```

This may insert `"apple"` accidentally.

Use `find` / `contains` for existence check.

---

## 12. at

```cpp
mp.at(key)
```

If key exists, returns reference to value.

If key does not exist, throws:

```cpp
std::out_of_range
```

Use when missing key is an error.

---

## 13. insert / emplace / try_emplace / insert_or_assign

For `map`, these are similar to `unordered_map`.

### insert

```cpp
mp.insert({key, value});
```

Does not overwrite existing key.

### emplace

```cpp
mp.emplace(key, value);
```

Constructs element in place.

Does not overwrite existing key.

### try_emplace

```cpp
mp.try_emplace(key, args...);
```

Only constructs mapped value if key does not already exist.

Useful for expensive or move-only values.

### insert_or_assign

```cpp
mp.insert_or_assign(key, value);
```

If key exists, assigns new value.

If key does not exist, inserts.

---

## 14. lower_bound

This is extremely important.

```cpp
auto it = mp.lower_bound(key);
```

Returns iterator to the first element whose key is:

```text
>= key
```

For set:

```cpp
auto it = s.lower_bound(x);
```

Returns first element:

```text
>= x
```

Example:

```cpp
std::set<int> s = {1, 3, 5, 7};

auto it = s.lower_bound(4);
```

`it` points to:

```text
5
```

---

## 15. upper_bound

```cpp
auto it = mp.upper_bound(key);
```

Returns iterator to the first element whose key is:

```text
> key
```

Example:

```cpp
std::set<int> s = {1, 3, 5, 7};

auto it = s.upper_bound(5);
```

`it` points to:

```text
7
```

---

## 16. lower_bound vs upper_bound

Given:

```cpp
std::set<int> s = {1, 3, 5, 7};
```

```cpp
s.lower_bound(5); // points to 5
s.upper_bound(5); // points to 7
```

For a missing key:

```cpp
s.lower_bound(4); // points to 5
s.upper_bound(4); // points to 5
```

Rule:

```text
lower_bound(x): first >= x
upper_bound(x): first > x
```

---

## 17. equal_range

```cpp
auto [first, last] = mp.equal_range(key);
```

Returns:

```text
[lower_bound(key), upper_bound(key))
```

For `map` / `set` with unique keys, this range contains either zero or one element.

For `multimap` / `multiset`, it can contain multiple equivalent elements.

---

## 18. Range Query

Because `map` and `set` are ordered, we can do range queries.

Example: print all values in `[L, R]`.

```cpp
std::set<int> s = {1, 3, 5, 7, 9};

auto it = s.lower_bound(L);

while (it != s.end() && *it <= R) {
    std::cout << *it << " ";
    ++it;
}
```

This is a major advantage over `unordered_set`.

---

## 19. Iterator Stability

For `map` / `set`:

Insertion:

```text
does not invalidate existing iterators/references
```

Erase:

```text
invalidates only erased elements
```

Why?

Elements are stored in separate tree nodes.

Tree rebalancing changes links between nodes, but the nodes themselves remain valid.

Example:

```cpp
auto it = mp.find(10);
mp[20] = "twenty";
```

`it` remains valid.

But:

```cpp
mp.erase(10);
```

Then iterator/reference to key `10` is invalid.

---

## 20. Why map keys are const

`map` element type:

```cpp
std::pair<const Key, Value>
```

This means you cannot modify the key through an iterator:

```cpp
auto it = mp.find("apple");
it->first = "banana"; // error
```

Why?

Changing the key in place could break the tree order.

To change a key:

```text
erase old key
insert new key
```

Example:

```cpp
auto node = mp.extract("apple");
node.key() = "banana";
mp.insert(std::move(node));
```

`extract` is C++17 and allows safe key mutation outside the tree.

---

## 21. multimap / multiset

`map` and `set` require unique keys.

`multimap` and `multiset` allow duplicate equivalent keys.

Example:

```cpp
std::multiset<int> ms;
ms.insert(1);
ms.insert(1);
ms.insert(1);
```

Now it contains three `1`s.

For duplicates, use:

```cpp
auto [first, last] = ms.equal_range(1);
```

---

## 22. map vs unordered_map

### map

```text
ordered
tree-based
O(log n)
supports lower_bound / range query
stable iterators except erased
```

### unordered_map

```text
unordered
hash-table-based
average O(1)
no sorted order
rehash invalidates iterators
```

Use `map` when:

- sorted iteration is needed
- range query is needed
- lower_bound / upper_bound is needed
- predictable O(log n) matters

Use `unordered_map` when:

- average fast lookup is enough
- no ordering is needed

---

## 23. Common Interview Questions

### Q1. How is std::map usually implemented?

Usually as a balanced binary search tree, commonly a red-black tree.

This gives O(log n) lookup, insertion, and deletion while maintaining sorted order.

---

### Q2. Difference between map and unordered_map?

`map` is ordered and tree-based with O(log n) operations.

`unordered_map` is hash-table-based with average O(1) operations but no sorted order.

Use `map` for ordered iteration and range queries.

Use `unordered_map` for fast average lookup when order does not matter.

---

### Q3. What is lower_bound?

`lower_bound(x)` returns the first element whose key is greater than or equal to `x`.

```text
first >= x
```

---

### Q4. What is upper_bound?

`upper_bound(x)` returns the first element whose key is strictly greater than `x`.

```text
first > x
```

---

### Q5. Why are map keys const?

Because modifying a key in place could break the tree ordering.

To change a key, erase and reinsert, or use node extraction in C++17.

---

### Q6. What does map insertion invalidate?

Insertion does not invalidate existing iterators or references.

Erase invalidates only erased elements.

---

## 24. Key Takeaways

- `map` stores sorted unique key-value pairs.
- `set` stores sorted unique keys.
- Usually implemented as balanced binary search trees.
- Lookup/insert/delete are O(log n).
- Iteration is sorted.
- `lower_bound(x)` gives first `>= x`.
- `upper_bound(x)` gives first `> x`.
- Supports range queries.
- Insertion does not invalidate existing iterators/references.
- Erase invalidates only erased elements.
- Keys in map are const.
- Use `map` for ordering/range queries.
- Use `unordered_map` for average O(1) lookup without order.
