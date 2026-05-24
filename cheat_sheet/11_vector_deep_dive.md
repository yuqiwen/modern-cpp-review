# 11. std::vector Deep Dive

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Typical Internal Model](#2-typical-internal-model)
- [3. size vs capacity](#3-size-vs-capacity)
- [4. reserve](#4-reserve)
- [5. resize](#5-resize)
- [6. push_back](#6-push_back)
- [7. Why push_back is amortized O(1)](#7-why-push_back-is-amortized-o1)
- [8. Reallocation and Invalidation](#8-reallocation-and-invalidation)
- [9. If No Reallocation Happens](#9-if-no-reallocation-happens)
- [10. Insertion in the Middle](#10-insertion-in-the-middle)
- [11. Erase in the Middle](#11-erase-in-the-middle)
- [12. clear](#12-clear)
- [13. shrink_to_fit](#13-shrink_to_fit)
- [14. data()](#14-data)
- [15. emplace_back vs push_back](#15-emplace_back-vs-push_back)
- [16. move_if_noexcept During Reallocation](#16-move_if_noexcept-during-reallocation)
- [17. Vector of unique_ptr](#17-vector-of-unique_ptr)
- [18. erase-remove Idiom](#18-erase-remove-idiom)
- [19. Common Interview Questions](#19-common-interview-questions)
- [20. Key Takeaways](#20-key-takeaways)

## Related Code Trap

- [Vector Deep Dive Demo](../code_traps/vector_deep_dive.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

`std::vector<T>` is a dynamic array.

Its elements are stored contiguously in memory.

Conceptually:

```text
data pointer
   |
   v
+----+----+----+----+----+
| T0 | T1 | T2 | T3 | T4 |
+----+----+----+----+----+
```

Key properties:

- contiguous memory
- O(1) random access
- amortized O(1) `push_back`
- O(n) insertion/deletion in the middle
- excellent cache locality
- reallocation may invalidate iterators, pointers, and references

---

## 2. Typical Internal Model

A vector can be roughly modeled as three pointers:

```cpp
T* begin;
T* end;
T* cap;
```

Meaning:

```text
begin -> first element
end   -> one past the last constructed element
cap   -> one past the allocated storage
```

Then:

```cpp
size()     == end - begin
capacity() == cap - begin
```

Memory layout:

```text
begin                  end                 cap
  |                     |                   |
  v                     v                   v
+----+----+----+----+----+----+----+----+
| T0 | T1 | T2 | T3 |    unused storage  |
+----+----+----+----+----+----+----+----+
<------ size -------->
<------------- capacity -------------->
```

The unused storage is allocated memory, but no `T` objects are constructed there yet.

---

## 3. size vs capacity

### size

```cpp
v.size()
```

Number of constructed elements.

### capacity

```cpp
v.capacity()
```

Amount of allocated storage available before reallocation is needed.

Example:

```cpp
std::vector<int> v;
v.reserve(10);
```

After this:

```text
size = 0
capacity >= 10
```

There is storage for at least 10 elements, but there are no constructed elements yet.

---

## 4. reserve

```cpp
v.reserve(100);
```

`reserve` increases capacity if needed.

It does not change size.

Example:

```cpp
std::vector<int> v;
v.reserve(3);
```

Now:

```text
size = 0
capacity >= 3
```

This is invalid:

```cpp
v[0] = 10; // wrong, size is still 0
```

Correct:

```cpp
v.push_back(10);
```

or:

```cpp
v.resize(3);
v[0] = 10;
```

Important:

```text
reserve allocates storage but does not construct elements.
```

---

## 5. resize

```cpp
v.resize(100);
```

`resize` changes size.

If the new size is larger, it creates new elements.

If the new size is smaller, it destroys extra elements.

Example:

```cpp
std::vector<int> v;
v.resize(3);
```

Now:

```text
size = 3
capacity >= 3
```

For `int`, new elements are value-initialized to `0`.

```cpp
v[0] = 10; // OK
```

Important:

```text
resize changes the number of constructed elements.
reserve changes only allocated storage.
```

---

## 6. push_back

```cpp
v.push_back(x);
```

If there is enough capacity:

```text
construct new element at end
increase size
```

No reallocation.

If capacity is full:

```text
allocate larger storage
move/copy old elements
destroy old elements
deallocate old storage
construct new element
```

This is reallocation.

---

## 7. Why push_back is amortized O(1)

A single `push_back` may be O(n) if reallocation happens.

But vector usually grows geometrically, for example roughly 1.5x or 2x.

So over many insertions, total cost averages out to amortized O(1).

Example:

```text
capacity: 1 -> 2 -> 4 -> 8 -> 16 -> ...
```

Most pushes are cheap. Only occasional pushes reallocate.

---

## 8. Reallocation and Invalidation

Example:

```cpp
std::vector<int> v = {1, 2, 3};

int& r = v[0];
int* p = &v[0];
auto it = v.begin();

v.push_back(4); // may reallocate
```

If reallocation happens:

```text
old storage:
address 1000: 1
address 1004: 2
address 1008: 3

new storage:
address 5000: 1
address 5004: 2
address 5008: 3
address 5012: 4
```

Then:

```text
r still refers to old address 1000
p still points to old address 1000
it still points to old address 1000
```

But old storage has been destroyed/deallocated.

So all three are invalid.

Important rule:

```text
vector reallocation invalidates all iterators, pointers, and references to elements.
```

---

## 9. If No Reallocation Happens

If there is enough capacity:

```cpp
std::vector<int> v;
v.reserve(10);

v.push_back(1);
v.push_back(2);

int& r = v[0];
int* p = &v[0];
auto it = v.begin();

v.push_back(3); // no reallocation
```

Then `r`, `p`, and `it` to existing elements remain valid.

But note:

```text
end() iterator changes after push_back.
```

So any old `end()` iterator is invalid/stale.

---

## 10. Insertion in the Middle

Example:

```cpp
std::vector<int> v = {1, 2, 4, 5};
auto it = v.begin() + 2; // points to 4

v.insert(it, 3);
```

If reallocation happens:

```text
all iterators, pointers, references are invalidated
```

If no reallocation happens:

```text
iterators/references/pointers before insertion point remain valid
iterators/references/pointers at or after insertion point are invalidated
```

Why?

Because elements at and after insertion point may be shifted to make room.

---

## 11. Erase in the Middle

Example:

```cpp
std::vector<int> v = {1, 2, 3, 4};
auto it = v.begin() + 1; // points to 2

v.erase(it);
```

Elements after erased position are shifted left.

So:

```text
iterators/references/pointers at or after erased position are invalidated
iterators/references/pointers before erased position remain valid
```

Also, `erase` returns an iterator to the next element.

Correct loop pattern:

```cpp
for (auto it = v.begin(); it != v.end(); ) {
    if (*it % 2 == 0) {
        it = v.erase(it);
    } else {
        ++it;
    }
}
```

Do not do:

```cpp
for (auto it = v.begin(); it != v.end(); ++it) {
    if (*it % 2 == 0) {
        v.erase(it); // wrong, it becomes invalid
    }
}
```

---

## 12. clear

```cpp
v.clear();
```

Destroys all elements.

After `clear`:

```text
size = 0
capacity unchanged
```

References/pointers/iterators to old elements are invalid.

But the allocated storage may still be kept.

---

## 13. shrink_to_fit

```cpp
v.shrink_to_fit();
```

Requests reducing capacity to fit size.

Important:

```text
shrink_to_fit is non-binding.
```

The implementation may or may not actually reduce capacity.

If it reallocates, iterators/pointers/references are invalidated.

---

## 14. data()

```cpp
T* ptr = v.data();
```

`data()` returns a pointer to contiguous storage.

Useful for:

- C APIs
- performance code
- serialization
- GPU / system interfaces

Example:

```cpp
std::vector<int> v = {1, 2, 3};
int* p = v.data();
```

Then:

```cpp
p[0] == v[0]
```

But if vector reallocates, old `p` becomes invalid.

---

## 15. emplace_back vs push_back

```cpp
v.push_back(T(args));
```

Constructs a temporary `T`, then moves/copies it into vector.

```cpp
v.emplace_back(args);
```

Constructs `T` directly in vector storage.

Example:

```cpp
std::vector<std::string> v;

v.push_back(std::string("hello"));
v.emplace_back("world");
```

`emplace_back` can avoid a temporary.

But do not overuse it blindly. For simple already-existing objects, `push_back` is clearer.

---

## 16. move_if_noexcept During Reallocation

When vector reallocates elements, it needs to transfer old elements to new storage.

For each element, it may choose:

```cpp
move
```

or:

```cpp
copy
```

If `T`'s move constructor is `noexcept`, vector can safely move.

If move may throw and copy is available, vector may copy instead to preserve strong exception guarantee.

Conceptually:

```cpp
std::move_if_noexcept(element)
```

Rule:

```text
vector prefers noexcept move; otherwise it may copy.
```

This is why custom move constructors should often be marked `noexcept`.

---

## 17. Vector of unique_ptr

```cpp
std::vector<std::unique_ptr<T>> v;
```

This is valid.

`unique_ptr` is move-only but not copyable.

Vector can store move-only types because it can move elements during reallocation.

Example:

```cpp
v.push_back(std::make_unique<T>());
```

But this is invalid:

```cpp
std::unique_ptr<T> p = std::make_unique<T>();
v.push_back(p); // error, copy not allowed
```

Correct:

```cpp
v.push_back(std::move(p));
```

or:

```cpp
v.emplace_back(std::make_unique<T>());
```

---

## 18. erase-remove Idiom

To remove all elements matching a condition from vector:

```cpp
v.erase(
    std::remove(v.begin(), v.end(), value),
    v.end()
);
```

For predicate:

```cpp
v.erase(
    std::remove_if(v.begin(), v.end(), pred),
    v.end()
);
```

Why?

`remove` / `remove_if` does not actually shrink the vector.

It rearranges elements and returns the new logical end.

Then `erase` removes the trailing unwanted elements.

Example:

```cpp
std::vector<int> v = {1, 2, 3, 2, 4};

v.erase(std::remove(v.begin(), v.end(), 2), v.end());
```

Result:

```text
1 3 4
```

---

## 19. Common Interview Questions

### Q1. What is the difference between size and capacity?

`size` is the number of constructed elements.

`capacity` is the amount of allocated storage before reallocation is needed.

---

### Q2. What is the difference between reserve and resize?

`reserve` changes capacity but not size.

`resize` changes size and constructs or destroys elements.

---

### Q3. Why can vector push_back invalidate references?

If capacity is full, vector reallocates a larger memory block and moves/copies elements to the new storage.

Old element objects are destroyed, so old references/pointers/iterators become invalid.

---

### Q4. What happens to iterators after vector erase?

Iterators, references, and pointers at or after the erased position are invalidated.

Elements after the erased position are shifted left.

`erase` returns an iterator to the next element.

---

### Q5. Why is vector often faster than list?

Because vector stores elements contiguously, giving excellent cache locality.

Even if list has O(1) insertion given an iterator, list nodes are scattered in memory and finding the position is often O(n).

---

### Q6. Why should move constructors often be noexcept for vector?

During reallocation, vector can move elements safely if the move constructor is `noexcept`.

If move may throw and copy is available, vector may copy instead to preserve exception safety.

---

## 20. Key Takeaways

- `vector` is contiguous dynamic array.
- `size` is constructed element count.
- `capacity` is allocated storage count.
- `reserve` changes capacity only.
- `resize` changes size.
- Reallocation invalidates all iterators, pointers, and references.
- Insertion/erase in middle shifts elements.
- `erase` returns the next valid iterator.
- `data()` pointer becomes invalid after reallocation.
- `emplace_back` constructs in place.
- `vector` prefers `noexcept` move during reallocation.
- Use `vector` by default unless another container's properties are needed.
