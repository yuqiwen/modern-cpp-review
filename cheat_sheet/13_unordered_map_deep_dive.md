# 13. unordered_map Deep Dive

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Internal Model](#2-internal-model)
- [3. Hash Function](#3-hash-function)
- [4. Equality Function](#4-equality-function)
- [5. Collision](#5-collision)
- [6. Average O(1) vs Worst-case O(n)](#6-average-o1-vs-worst-case-on)
- [7. bucket_count](#7-bucket_count)
- [8. load_factor](#8-load_factor)
- [9. max_load_factor](#9-max_load_factor)
- [10. rehash](#10-rehash)
- [11. reserve](#11-reserve)
- [12. rehash vs reserve](#12-rehash-vs-reserve)
- [13. operator[]](#13-operator)
- [14. find](#14-find)
- [15. contains](#15-contains)
- [16. at](#16-at)
- [17. insert](#17-insert)
- [18. emplace](#18-emplace)
- [19. insert_or_assign](#19-insert_or_assign)
- [20. try_emplace](#20-try_emplace)
- [21. operator[] Requires Default-Constructible Value](#21-operator-requires-default-constructible-value)
- [22. Custom Hash for Pair](#22-custom-hash-for-pair)
- [23. Custom Hash for Struct](#23-custom-hash-for-struct)
- [24. Hash Quality](#24-hash-quality)
- [25. unordered_map and Ordering](#25-unordered_map-and-ordering)
- [26. Common Interview Questions](#26-common-interview-questions)
- [27. Key Takeaways](#27-key-takeaways)

## Related Code Trap

- [unordered_map Deep Dive Demo](../code_traps/unordered_map_deep_dive.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

`std::unordered_map<K, V>` is a hash table.

It stores key-value pairs:

```cpp
std::unordered_map<std::string, int> mp;
mp["apple"] = 3;
```

Key properties:

- average lookup: `O(1)`
- average insertion: `O(1)`
- average deletion: `O(1)`
- worst-case operations: `O(n)`
- no sorted order
- performance depends on hash quality, equality comparison, load factor, and rehashing

---

## 2. Internal Model

A common mental model:

```text
bucket array
+----------+
| bucket 0 | -> node(k1, v1) -> node(k2, v2)
| bucket 1 | -> nullptr
| bucket 2 | -> node(k3, v3)
| bucket 3 | -> node(k4, v4)
+----------+
```

Each element is stored as a node.

Each bucket stores a chain of nodes whose keys hash into that bucket.

The C++ standard does not force this exact implementation, but this model is very useful for interviews.

---

## 3. Hash Function

A hash function converts a key into a hash value.

Example:

```cpp
std::hash<std::string>{}("apple");
```

This returns a `size_t`.

Then the container maps the hash value to a bucket index:

```text
bucket_index = hash(key) % bucket_count
```

Conceptually:

```cpp
size_t h = std::hash<std::string>{}(key);
size_t index = h % bucket_count;
```

The real implementation may optimize this, but conceptually this is enough.

---

## 4. Equality Function

Hashing alone is not enough.

Two different keys may have the same hash or land in the same bucket.

So `unordered_map` also uses equality comparison:

```cpp
key1 == key2
```

Lookup process:

```text
1. compute hash(key)
2. find bucket
3. scan nodes in bucket
4. compare actual keys using equality
```

So the hash function chooses the bucket, and equality checks the exact key.

---

## 5. Collision

A collision happens when multiple keys map to the same bucket.

Example:

```text
hash("apple") % bucket_count == 3
hash("banana") % bucket_count == 3
```

Then both keys are stored in bucket 3.

Conceptually:

```text
bucket 3 -> ("apple", 1) -> ("banana", 2)
```

Collision is normal.

Good hash functions spread keys evenly across buckets.

Bad hash functions create long chains and degrade performance.

---

## 6. Average O(1) vs Worst-case O(n)

Average case:

```text
hash distributes keys evenly
each bucket has few elements
lookup scans only a short chain
```

So lookup is average `O(1)`.

Worst case:

```text
all keys collide into one bucket
```

Then lookup may scan all elements:

```text
bucket 0 -> node1 -> node2 -> node3 -> ... -> nodeN
```

So worst-case lookup is `O(n)`.

Interview answer:

```text
unordered_map provides average O(1), but worst-case O(n) due to collisions.
```

---

## 7. bucket_count

```cpp
mp.bucket_count()
```

Returns the number of buckets.

Example:

```cpp
std::unordered_map<std::string, int> mp;
std::cout << mp.bucket_count() << std::endl;
```

The bucket count can grow when the map rehashes.

---

## 8. load_factor

```cpp
mp.load_factor()
```

Means:

```text
size / bucket_count
```

Example:

```cpp
std::unordered_map<int, int> mp;
mp[1] = 10;
mp[2] = 20;

float lf = mp.load_factor();
```

If load factor becomes too high, average bucket length increases.

This can make operations slower.

---

## 9. max_load_factor

```cpp
mp.max_load_factor()
```

Controls when the table should grow.

Example:

```cpp
mp.max_load_factor(0.7);
```

This asks the container to keep the load factor around or below 0.7.

Lower max load factor:

- more buckets
- more memory
- fewer collisions on average

Higher max load factor:

- fewer buckets
- less memory
- more collisions on average

---

## 10. rehash

```cpp
mp.rehash(n);
```

Requests at least `n` buckets.

Rehash process:

```text
1. allocate new bucket array
2. recompute bucket index for every node
3. relink nodes into new buckets
```

Rehash invalidates iterators.

References and pointers to elements remain valid.

---

## 11. reserve

```cpp
mp.reserve(n);
```

Requests enough buckets to store at least `n` elements without exceeding `max_load_factor`.

Conceptually:

```text
needed buckets >= n / max_load_factor
```

So:

```cpp
mp.max_load_factor(0.5);
mp.reserve(1000);
```

may allocate at least around 2000 buckets.

Use `reserve` before inserting many elements to reduce repeated rehashing.

---

## 12. rehash vs reserve

```cpp
mp.rehash(bucket_count);
```

You specify bucket count.

```cpp
mp.reserve(element_count);
```

You specify expected number of elements.

Usually for normal usage, prefer:

```cpp
mp.reserve(expected_number_of_elements);
```

because it is more intuitive.

---

## 13. operator[]

```cpp
mp[key]
```

Important behavior:

```text
If key exists:
    returns reference to mapped value

If key does not exist:
    inserts a new key-value pair with default-constructed value
    returns reference to that value
```

Example:

```cpp
std::unordered_map<std::string, int> mp;

std::cout << mp["apple"] << std::endl;
```

This inserts:

```text
"apple" -> 0
```

because `int` default value is `0`.

This is a common bug.

---

## 14. find

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

`find` does not insert.

Use `find` when you only want to check existence.

Example:

```cpp
auto it = mp.find("apple");
if (it != mp.end()) {
    std::cout << it->second << std::endl;
}
```

---

## 15. contains

C++20:

```cpp
if (mp.contains(key)) {
    // key exists
}
```

`contains` checks existence and does not insert.

C++17 does not have `contains`, so use:

```cpp
mp.find(key) != mp.end()
```

---

## 16. at

```cpp
mp.at(key)
```

If key exists, returns reference to value.

If key does not exist, throws:

```cpp
std::out_of_range
```

Example:

```cpp
try {
    int x = mp.at("apple");
} catch (const std::out_of_range&) {
    std::cout << "not found\n";
}
```

Use `at` when missing key should be an error.

---

## 17. insert

```cpp
mp.insert({"apple", 3});
```

If key does not exist, inserts.

If key already exists, does not overwrite.

Return value:

```cpp
auto [it, inserted] = mp.insert({"apple", 3});
```

`inserted` is true if insertion happened.

---

## 18. emplace

```cpp
mp.emplace("apple", 3);
```

Constructs the element in place.

Like `insert`, it does not overwrite existing key.

Return value:

```cpp
auto [it, inserted] = mp.emplace("apple", 3);
```

---

## 19. insert_or_assign

C++17:

```cpp
mp.insert_or_assign("apple", 5);
```

If key exists:

```text
assign new value
```

If key does not exist:

```text
insert new key-value pair
```

This is useful when you want overwrite behavior but do not want accidental default construction from `operator[]`.

---

## 20. try_emplace

C++17:

```cpp
mp.try_emplace(key, args...);
```

If key does not exist, constructs mapped value in place.

If key already exists, does not construct the mapped value.

This is useful when mapped value construction is expensive or move-only.

Example:

```cpp
std::unordered_map<std::string, std::unique_ptr<int>> mp;

mp.try_emplace("a", std::make_unique<int>(10));
```

If `"a"` already exists, it will not replace the existing value.

---

## 21. operator[] Requires Default-Constructible Value

Because `operator[]` inserts a default value when key is missing:

```cpp
mp[key]
```

requires `V` to be default-constructible.

Example:

```cpp
struct NoDefault {
    NoDefault(int x) {}
};

std::unordered_map<std::string, NoDefault> mp;

// mp["a"]; // error, NoDefault has no default constructor
```

Use `emplace`, `try_emplace`, or `insert_or_assign` instead:

```cpp
mp.emplace("a", NoDefault(10));
mp.try_emplace("b", 20);
mp.insert_or_assign("c", NoDefault(30));
```

---

## 22. Custom Hash for Pair

`unordered_map` does not have a default hash for `std::pair<int, int>` in older/common standard library usage.

We can define one:

```cpp
struct PairHash {
    size_t operator()(const std::pair<int, int>& p) const {
        size_t h1 = std::hash<int>{}(p.first);
        size_t h2 = std::hash<int>{}(p.second);
        return h1 ^ (h2 << 1);
    }
};
```

Usage:

```cpp
std::unordered_map<std::pair<int, int>, int, PairHash> mp;
mp[{1, 2}] = 10;
```

This is common in coding interviews.

---

## 23. Custom Hash for Struct

Example:

```cpp
struct Point {
    int x;
    int y;

    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
};

struct PointHash {
    size_t operator()(const Point& p) const {
        size_t h1 = std::hash<int>{}(p.x);
        size_t h2 = std::hash<int>{}(p.y);
        return h1 ^ (h2 << 1);
    }
};
```

Usage:

```cpp
std::unordered_map<Point, int, PointHash> mp;
```

Need both:

```text
hash function
equality comparison
```

Hash says where to look.

Equality says whether this is the exact key.

---

## 24. Hash Quality

A weak hash function can cause many collisions.

Bad example:

```cpp
struct BadHash {
    size_t operator()(int x) const {
        return 0;
    }
};
```

All keys go into the same bucket.

Then lookup becomes `O(n)`.

Good hash should spread keys evenly.

For interview custom hash, simple combinations are usually enough.

For production or adversarial input, hash quality matters more.

---

## 25. unordered_map and Ordering

`unordered_map` does not maintain sorted order.

Iteration order is not stable or sorted.

Example:

```cpp
for (auto& [k, v] : mp) {
    std::cout << k << std::endl;
}
```

Do not assume any order.

If you need sorted keys, use:

```cpp
std::map<K, V>
```

---

## 26. Common Interview Questions

### Q1. How does unordered_map work?

It uses a hash function to map keys to buckets.

Each bucket stores elements whose keys map to that bucket.

Lookup computes the hash, finds the bucket, then compares keys inside that bucket using equality.

---

### Q2. Why is unordered_map average O(1) but worst-case O(n)?

Because with a good hash function, keys are distributed across buckets and each bucket is short.

In the worst case, many or all keys collide into one bucket, making lookup scan a long chain.

---

### Q3. Difference between operator[] and find?

`operator[]` inserts a default-constructed value if the key does not exist.

`find` only searches and does not insert.

Use `find` or `contains` when checking existence.

---

### Q4. Difference between reserve and rehash?

`reserve(n)` prepares enough buckets for about `n` elements.

`rehash(n)` requests at least `n` buckets.

For expected element count, prefer `reserve`.

---

### Q5. What does rehash invalidate?

Rehash invalidates iterators.

References and pointers to elements remain valid.

---

### Q6. How do you use a custom key in unordered_map?

The key type needs a hash function and equality comparison.

For a custom struct, define `operator==` and provide a hash functor.

---

## 27. Key Takeaways

- `unordered_map` is hash-table-based.
- Hash maps keys to buckets.
- Equality checks exact key match.
- Collision means multiple keys in same bucket.
- Average O(1), worst-case O(n).
- `load_factor = size / bucket_count`.
- `reserve(n)` means reserve for n elements.
- `rehash(n)` means request n buckets.
- `operator[]` inserts default value if missing.
- `find` does not insert.
- `at` throws if missing.
- `try_emplace` avoids constructing value if key already exists.
- Custom key requires hash and equality.
- Iteration order is not sorted or stable.
