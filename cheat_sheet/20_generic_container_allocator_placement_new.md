# 20. Generic Container, Allocator, Placement New, and Object Lifetime

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Why `new T[n]` Is Not Enough](#2-why-new-tn-is-not-enough)
- [3. `T* data` Is Fine](#3-t-data-is-fine)
- [4. Raw Storage vs Object Lifetime](#4-raw-storage-vs-object-lifetime)
- [5. std::allocator](#5-stdallocator)
- [6. construct_at](#6-construct_at)
- [7. destroy_at](#7-destroy_at)
- [8. Placement New](#8-placement-new)
- [9. Why Destruction Must Be Manual](#9-why-destruction-must-be-manual)
- [10. Minimal SimpleVector Data Members](#10-minimal-simplevector-data-members)
- [11. push_back with Copy](#11-push_back-with-copy)
- [12. push_back with Move](#12-push_back-with-move)
- [13. emplace_back](#13-emplace_back)
- [14. Destructor](#14-destructor)
- [15. grow](#15-grow)
- [16. Why move_if_noexcept](#16-why-move_if_noexcept)
- [17. Exception Safety Problem in grow](#17-exception-safety-problem-in-grow)
- [18. Why int Version Was Easy](#18-why-int-version-was-easy)
- [19. Object Lifetime Rule](#19-object-lifetime-rule)
- [20. Common Interview Questions](#20-common-interview-questions)
- [21. Key Takeaways](#21-key-takeaways)

## Related Code Trap

- [Generic Container / Allocator / Placement New Demo](../code_traps/generic_container_allocator_placement_new.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

For a generic container like:

```cpp
template <typename T>
class SimpleVector;
```

we should separate:

```text
memory allocation
object construction
object destruction
memory deallocation
```

This is because allocated memory and live objects are not the same thing.

---

## 2. Why `new T[n]` Is Not Enough

This looks simple:

```cpp
T* data = new T[n];
```

But it has problems for a generic container.

It does two things at once:

```text
1. allocate memory for n objects
2. default-construct n T objects
```

This requires `T` to have a default constructor.

Example:

```cpp
class NoDefault {
public:
    NoDefault(int x) {}
};
```

This fails:

```cpp
NoDefault* p = new NoDefault[10]; // error
```

because `NoDefault` cannot be default-constructed.

---

## 3. `T* data` Is Fine

This is fine:

```cpp
template <typename T>
class Buffer {
private:
    T* data;
};
```

`T* data` is only a pointer.

It does not create any `T` object.

The problem happens when we do:

```cpp
data = new T[n];
```

because that constructs `n` objects.

---

## 4. Raw Storage vs Object Lifetime

Important distinction:

```text
raw storage = memory bytes
object lifetime = actual T object exists there
```

A vector may allocate capacity for 10 elements, but only construct 3 elements.

Example:

```text
capacity = 10
size = 3
```

Memory layout:

```text
+----+----+----+----+----+----+----+----+----+----+
| T0 | T1 | T2 | raw memory, no T objects yet       |
+----+----+----+----+----+----+----+----+----+----+
<------ live objects ----->
<---------------- allocated storage ---------------->
```

Only the first `size` elements should be destroyed.

The unused capacity is just raw memory.

---

## 5. std::allocator

`std::allocator<T>` lets us allocate raw memory for `T` objects without constructing them immediately.

```cpp
std::allocator<T> alloc;

T* data = alloc.allocate(capacity);
```

This allocates raw storage for `capacity` `T` objects.

But no `T` objects are constructed yet.

Later we construct one object:

```cpp
std::construct_at(data + i, value);
```

And destroy it:

```cpp
std::destroy_at(data + i);
```

Finally release memory:

```cpp
alloc.deallocate(data, capacity);
```

---

## 6. construct_at

C++20:

```cpp
std::construct_at(ptr, args...);
```

Constructs an object at an existing memory location.

Example:

```cpp
std::construct_at(data + size, value);
```

This starts the lifetime of a `T` object at `data + size`.

Equivalent old-style placement new:

```cpp
new (data + size) T(value);
```

---

## 7. destroy_at

```cpp
std::destroy_at(ptr);
```

Calls destructor for the object at `ptr`.

Example:

```cpp
std::destroy_at(data + i);
```

Equivalent to:

```cpp
(data + i)->~T();
```

Important:

```text
Only destroy objects whose lifetime has started.
Do not destroy raw unconstructed memory.
```

---

## 8. Placement New

Placement new constructs an object at a specific memory address.

Example:

```cpp
void* memory = ::operator new(sizeof(T));
T* obj = new (memory) T(args...);
```

This means:

```text
Use this existing memory address to construct a T object.
```

Then destroy:

```cpp
obj->~T();
```

Then release raw memory:

```cpp
::operator delete(memory);
```

`std::construct_at` is the modern clearer wrapper around this idea.

---

## 9. Why Destruction Must Be Manual

If we allocate raw memory:

```cpp
T* data = alloc.allocate(capacity);
```

no object exists yet.

When we construct objects manually:

```cpp
std::construct_at(data + 0, value0);
std::construct_at(data + 1, value1);
```

we must destroy them manually:

```cpp
std::destroy_at(data + 0);
std::destroy_at(data + 1);
```

Then deallocate raw storage:

```cpp
alloc.deallocate(data, capacity);
```

---

## 10. Minimal SimpleVector Data Members

A generic vector-like container needs:

```cpp
template <typename T>
class SimpleVector {
private:
    T* data = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
    std::allocator<T> alloc;
};
```

Meanings:

```text
data      -> raw storage pointer
size_     -> number of constructed/live T objects
capacity_ -> allocated storage capacity
alloc     -> allocator for raw memory
```

---

## 11. push_back with Copy

Simplified:

```cpp
void push_back(const T& value) {
    if (size_ == capacity_) {
        grow();
    }

    std::construct_at(data + size_, value);
    ++size_;
}
```

This constructs a new `T` object at the first unused slot.

---

## 12. push_back with Move

```cpp
void push_back(T&& value) {
    if (size_ == capacity_) {
        grow();
    }

    std::construct_at(data + size_, std::move(value));
    ++size_;
}
```

This move-constructs the new element.

---

## 13. emplace_back

```cpp
template <typename... Args>
void emplace_back(Args&&... args) {
    if (size_ == capacity_) {
        grow();
    }

    std::construct_at(data + size_, std::forward<Args>(args)...);
    ++size_;
}
```

This constructs `T` directly in storage using forwarded constructor arguments.

This is why variadic templates and perfect forwarding matter.

---

## 14. Destructor

Destructor must destroy only constructed elements:

```cpp
~SimpleVector() {
    for (size_t i = 0; i < size_; ++i) {
        std::destroy_at(data + i);
    }

    if (data) {
        alloc.deallocate(data, capacity_);
    }
}
```

Do not destroy all `capacity_` slots.

Only first `size_` objects are alive.

---

## 15. grow

When capacity is full, allocate bigger storage.

Simplified process:

```text
1. allocate new raw storage
2. move/copy old constructed elements into new storage
3. destroy old elements
4. deallocate old storage
5. update data/capacity
```

Conceptual code:

```cpp
void grow() {
    size_t newCap = capacity_ == 0 ? 1 : capacity_ * 2;
    T* newData = alloc.allocate(newCap);

    for (size_t i = 0; i < size_; ++i) {
        std::construct_at(newData + i, std::move_if_noexcept(data[i]));
    }

    for (size_t i = 0; i < size_; ++i) {
        std::destroy_at(data + i);
    }

    if (data) {
        alloc.deallocate(data, capacity_);
    }

    data = newData;
    capacity_ = newCap;
}
```

---

## 16. Why move_if_noexcept

During reallocation, we need to transfer old elements to new storage.

If move is `noexcept`, moving is safe and efficient.

If move may throw and copy is available, copying can preserve strong exception safety.

Conceptually:

```cpp
std::construct_at(newData + i, std::move_if_noexcept(data[i]));
```

This may call move constructor or copy constructor depending on `T`.

---

## 17. Exception Safety Problem in grow

The simplified `grow()` above is not fully exception-safe.

Problem:

```text
construct element 0 in new storage succeeds
construct element 1 succeeds
construct element 2 throws
```

Now:

```text
new storage has partially constructed objects
old storage still exists
```

We must destroy constructed objects in new storage and deallocate new storage before rethrowing.

Exception-safe version needs a counter:

```cpp
size_t constructed = 0;

try {
    for (; constructed < size_; ++constructed) {
        std::construct_at(newData + constructed,
                          std::move_if_noexcept(data[constructed]));
    }
} catch (...) {
    for (size_t j = 0; j < constructed; ++j) {
        std::destroy_at(newData + j);
    }
    alloc.deallocate(newData, newCap);
    throw;
}
```

This is why generic containers are hard.

---

## 18. Why int Version Was Easy

For `int`:

```cpp
int* data = new int[n];
```

is relatively simple because:

```text
int has trivial construction
int copy does not throw
int destructor does nothing
```

But for arbitrary `T`:

```text
T may not have default constructor
T copy constructor may throw
T move constructor may throw
T destructor may be non-trivial
T may be move-only
```

So we need allocator + construct_at + destroy_at.

---

## 19. Object Lifetime Rule

A pointer to raw storage is not enough.

Before construction:

```cpp
T* p = alloc.allocate(1);
```

Memory exists, but no `T` object exists.

This is not valid:

```cpp
*p = value; // wrong, no object lifetime has started
```

Correct:

```cpp
std::construct_at(p, value);
```

After that, a `T` object exists at `p`.

Then later:

```cpp
std::destroy_at(p);
alloc.deallocate(p, 1);
```

---

## 20. Common Interview Questions

### Q1. Why can't a generic vector simply use `new T[capacity]`?

Because it default-constructs `capacity` objects immediately and requires `T` to be default-constructible.

A real vector should allocate raw storage and construct only `size` live elements.

---

### Q2. What is placement new?

Placement new constructs an object at a given memory address.

It does not allocate memory; it only starts object lifetime in existing storage.

---

### Q3. What is the difference between allocation and construction?

Allocation reserves raw memory.

Construction creates an actual object in that memory and starts its lifetime.

---

### Q4. Why does a vector destroy only `size` elements, not `capacity`?

Because only the first `size` slots contain live constructed objects.

The remaining capacity is raw unconstructed storage.

---

### Q5. Why is generic container implementation difficult?

Because arbitrary `T` may have throwing copy/move constructors, no default constructor, non-trivial destructor, or move-only behavior.

The container must handle object lifetime and exception safety correctly.

---

## 21. Key Takeaways

- Raw memory is not the same as live object.
- `new T[n]` allocates and default-constructs n objects.
- Generic containers should separate allocation and construction.
- `std::allocator<T>::allocate` gives raw storage.
- `std::construct_at` constructs an object in existing storage.
- `std::destroy_at` destroys a constructed object.
- Placement new is the lower-level mechanism behind construct_at.
- Only destroy constructed objects.
- `size` means live objects.
- `capacity` means allocated slots.
- Reallocation must handle exception safety carefully.
