# 21. Generic Container Rule of Five and Exception Safety

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Destructor](#2-destructor)
- [3. Copy Constructor](#3-copy-constructor)
- [4. Copy Assignment](#4-copy-assignment)
- [5. swap](#5-swap)
- [6. Move Constructor](#6-move-constructor)
- [7. Move Assignment](#7-move-assignment)
- [8. grow and Strong Exception Safety](#8-grow-and-strong-exception-safety)
- [9. Why `move_if_noexcept`](#9-why-move_if_noexcept)
- [10. push_back Exception Safety](#10-push_back-exception-safety)
- [11. emplace_back Exception Safety](#11-emplace_back-exception-safety)
- [12. Why Copy Assignment Uses Copy-and-Swap](#12-why-copy-assignment-uses-copy-and-swap)
- [13. Strong Exception Guarantee](#13-strong-exception-guarantee)
- [14. Basic Exception Guarantee](#14-basic-exception-guarantee)
- [15. No-throw Guarantee](#15-no-throw-guarantee)
- [16. Rule of Five for SimpleVector](#16-rule-of-five-for-simplevector)
- [17. Rule of Zero Reminder](#17-rule-of-zero-reminder)
- [18. Common Interview Questions](#18-common-interview-questions)
- [19. Key Takeaways](#19-key-takeaways)

## Related Code Trap

- [Generic Container Rule of Five Demo](../code_traps/generic_container_rule_of_five.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

A generic container that manually owns raw memory must define how ownership behaves.

If a class owns a resource, it usually needs Rule of Five:

```cpp
destructor
copy constructor
copy assignment
move constructor
move assignment
```

For a vector-like container:

```cpp
template <typename T>
class SimpleVector {
private:
    T* data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
    std::allocator<T> alloc_;
};
```

The class owns:

```text
raw storage
constructed objects inside that storage
```

So it must manage them carefully.

---

## 2. Destructor

Destructor must:

```text
1. destroy live objects
2. deallocate raw storage
```

Code:

```cpp
~SimpleVector() {
    clear();

    if (data_) {
        alloc_.deallocate(data_, capacity_);
    }
}
```

`clear()` destroys only live objects:

```cpp
void clear() {
    for (size_t i = 0; i < size_; ++i) {
        std::destroy_at(data_ + i);
    }
    size_ = 0;
}
```

Do not destroy all `capacity_`.

Only first `size_` objects are alive.

---

## 3. Copy Constructor

Copy constructor should create a deep copy.

```cpp
SimpleVector(const SimpleVector& other)
```

It should:

```text
1. allocate new storage
2. copy-construct each live element from other
3. set size/capacity correctly
```

Simplified:

```cpp
SimpleVector(const SimpleVector& other)
    : data_(nullptr), size_(0), capacity_(0) {
    if (other.size_ == 0) {
        return;
    }

    data_ = alloc_.allocate(other.size_);
    capacity_ = other.size_;

    size_t constructed = 0;

    try {
        for (; constructed < other.size_; ++constructed) {
            std::construct_at(data_ + constructed, other.data_[constructed]);
        }
    } catch (...) {
        for (size_t i = 0; i < constructed; ++i) {
            std::destroy_at(data_ + i);
        }

        alloc_.deallocate(data_, capacity_);

        data_ = nullptr;
        capacity_ = 0;

        throw;
    }

    size_ = other.size_;
}
```

Why exception handling?

Because copying `T` may throw.

If copying element 3 throws, elements 0-2 in new storage must be destroyed and memory must be deallocated.

---

## 4. Copy Assignment

Copy assignment:

```cpp
SimpleVector& operator=(const SimpleVector& other)
```

Must handle:

```text
self-assignment
deep copy
exception safety
old resource cleanup
```

A common safe pattern is copy-and-swap.

```cpp
SimpleVector& operator=(const SimpleVector& other) {
    if (this == &other) {
        return *this;
    }

    SimpleVector temp(other);
    swap(temp);

    return *this;
}
```

Why safe?

```text
1. temp copies other
2. if copy throws, this remains unchanged
3. if copy succeeds, swap resources
4. temp destructor cleans old resource
```

This gives strong exception safety.

---

## 5. swap

We need a `swap` member:

```cpp
void swap(SimpleVector& other) noexcept {
    using std::swap;
    swap(data_, other.data_);
    swap(size_, other.size_);
    swap(capacity_, other.capacity_);
}
```

`swap` should be `noexcept` if it only swaps pointers and sizes.

---

## 6. Move Constructor

Move constructor should steal ownership.

```cpp
SimpleVector(SimpleVector&& other) noexcept
    : data_(other.data_),
      size_(other.size_),
      capacity_(other.capacity_) {
    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;
}
```

This is cheap.

It does not move each element individually.

It just steals:

```text
data pointer
size
capacity
```

Why `noexcept`?

Containers such as `std::vector<SimpleVector<T>>` can move `SimpleVector` safely during reallocation.

---

## 7. Move Assignment

Move assignment should:

```text
1. release current resource
2. steal other's resource
3. reset other
```

Code:

```cpp
SimpleVector& operator=(SimpleVector&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    clear();

    if (data_) {
        alloc_.deallocate(data_, capacity_);
    }

    data_ = other.data_;
    size_ = other.size_;
    capacity_ = other.capacity_;

    other.data_ = nullptr;
    other.size_ = 0;
    other.capacity_ = 0;

    return *this;
}
```

Self-move assignment is rare but checking `this == &other` is safe.

---

## 8. grow and Strong Exception Safety

When vector grows, it must transfer old elements to new storage.

Simplified process:

```text
1. allocate new storage
2. construct elements in new storage
3. destroy old elements
4. deallocate old storage
5. update data/capacity
```

The dangerous part is step 2.

Element construction may throw.

So we need:

```cpp
size_t constructed = 0;

try {
    for (; constructed < size_; ++constructed) {
        std::construct_at(
            newData + constructed,
            std::move_if_noexcept(data_[constructed])
        );
    }
} catch (...) {
    for (size_t i = 0; i < constructed; ++i) {
        std::destroy_at(newData + i);
    }

    alloc_.deallocate(newData, newCapacity);
    throw;
}
```

If transfer fails, old storage is still untouched and valid.

This gives strong exception safety.

---

## 9. Why `move_if_noexcept`

During reallocation, if we move old elements and move throws halfway, old elements may already be modified.

Copy is safer because source elements remain unchanged if copying into new storage fails.

So vector uses logic like:

```cpp
std::move_if_noexcept(data_[i])
```

Meaning:

```text
if T's move constructor is noexcept:
    move
else if T is copyable:
    copy
else:
    move anyway
```

This helps preserve strong exception safety when possible.

---

## 10. push_back Exception Safety

For:

```cpp
void push_back(const T& value)
```

Logic:

```cpp
if (size_ == capacity_) {
    grow();
}

std::construct_at(data_ + size_, value);
++size_;
```

Important:

```text
increment size_ only after construction succeeds
```

Wrong:

```cpp
++size_;
std::construct_at(data_ + size_ - 1, value);
```

If construction throws, `size_` would lie and destructor may try to destroy a non-existing object.

---

## 11. emplace_back Exception Safety

Same idea:

```cpp
template <typename... Args>
T& emplace_back(Args&&... args) {
    if (size_ == capacity_) {
        grow();
    }

    std::construct_at(data_ + size_, std::forward<Args>(args)...);
    ++size_;

    return data_[size_ - 1];
}
```

If construction throws, `size_` is not changed.

---

## 12. Why Copy Assignment Uses Copy-and-Swap

Bad copy assignment may destroy old data before new copy succeeds.

Bad pattern:

```cpp
clear();
deallocate old storage;
allocate new storage;
copy elements;
```

If allocation or copy throws, the original object is already destroyed.

Copy-and-swap avoids this:

```cpp
SimpleVector temp(other);
swap(temp);
```

If temp construction fails, current object is unchanged.

If temp construction succeeds, swap is noexcept.

Then temp destructor destroys old resource.

---

## 13. Strong Exception Guarantee

Strong exception guarantee means:

```text
operation either succeeds completely
or object remains unchanged
```

Example:

```cpp
v.push_back(x);
```

If it throws during reallocation, `v` should still contain the original elements.

This is why grow constructs new storage first and only commits after success.

---

## 14. Basic Exception Guarantee

Basic guarantee means:

```text
if operation throws, object is still valid and destructible,
but its value may have changed
```

Strong guarantee is better but harder.

Generic containers often try to provide strong guarantee when possible.

---

## 15. No-throw Guarantee

No-throw guarantee means:

```text
operation will not throw
```

Move constructor for resource-owning types should often be `noexcept`:

```cpp
SimpleVector(SimpleVector&& other) noexcept;
```

Because it only steals pointers.

---

## 16. Rule of Five for SimpleVector

Final structure:

```cpp
template <typename T>
class SimpleVector {
public:
    SimpleVector();
    ~SimpleVector();

    SimpleVector(const SimpleVector& other);
    SimpleVector& operator=(const SimpleVector& other);

    SimpleVector(SimpleVector&& other) noexcept;
    SimpleVector& operator=(SimpleVector&& other) noexcept;
};
```

Because this class manually owns raw memory, Rule of Five is appropriate.

If we used `std::vector<T>` as member instead, Rule of Zero would be better.

---

## 17. Rule of Zero Reminder

Prefer Rule of Zero when possible:

```cpp
class Owner {
private:
    std::vector<T> data;
};
```

No manual destructor/copy/move needed.

But we are implementing a vector-like container for learning, so Rule of Five is necessary.

---

## 18. Common Interview Questions

### Q1. Why does a generic container need Rule of Five?

Because it manually owns raw memory and constructed objects.

It must define how to destroy, copy, move, and assign that resource safely.

---

### Q2. How do you write exception-safe copy assignment?

Use copy-and-swap.

Create a temporary copy first. If copying fails, the original object is unchanged.

Then swap resources.

---

### Q3. Why should move constructor be noexcept?

Because it only steals pointers and sizes, so it should not throw.

Marking it `noexcept` also allows standard containers to move it during reallocation.

---

### Q4. Why does push_back increment size only after construction?

Because construction may throw.

If size is incremented before construction and construction fails, the container may think an object exists where no object was constructed.

---

### Q5. What is strong exception guarantee?

The operation either completes successfully or leaves the object unchanged.

---

## 19. Key Takeaways

- Resource-owning generic containers need Rule of Five.
- Copy constructor performs deep copy.
- Move constructor steals resource.
- Copy assignment should use copy-and-swap.
- Move assignment releases current resource, then steals.
- grow must clean up partially constructed new storage if construction throws.
- `size_` should change only after construction succeeds.
- `move_if_noexcept` helps preserve exception safety.
- Move constructor should usually be `noexcept`.
