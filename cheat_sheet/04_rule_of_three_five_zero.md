# 04. Rule of Three, Rule of Five, and Rule of Zero

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Compiler-generated Copy Is Shallow Copy](#2-compiler-generated-copy-is-shallow-copy)
- [3. Rule of Three](#3-rule-of-three)
- [4. Copy Constructor](#4-copy-constructor)
- [5. Copy Assignment Operator](#5-copy-assignment-operator)
- [6. Self-assignment](#6-self-assignment)
- [7. Copy-and-swap Idiom](#7-copy-and-swap-idiom)
- [8. Rule of Five](#8-rule-of-five)
- [9. Move Constructor](#9-move-constructor)
- [10. Move Assignment Operator](#10-move-assignment-operator)
- [11. Why Move Operations Should Often Be noexcept](#11-why-move-operations-should-often-be-noexcept)
- [12. Rule of Zero](#12-rule-of-zero)
- [13. Rule of Zero Example](#13-rule-of-zero-example)
- [14. Special Member Functions Summary](#14-special-member-functions-summary)
- [15. `= default`](#15--default)
- [16. `= delete`](#16--delete)
- [17. Common Interview Questions](#17-common-interview-questions)
- [18. Key Takeaways](#18-key-takeaways)
- [Note: What Changes for Template Type T?](#note-what-changes-for-template-type-t)

## Related Code Trap

- [Rule of Three / Five / Zero Demo](../code_traps/rule_of_three_five_zero.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

If a class owns a resource, then we must define how that resource is:

- destroyed
- copied
- assigned
- moved

A resource can be:

- heap memory
- file handle
- socket
- mutex
- GPU buffer
- database connection

The dangerous case is when a class owns a raw pointer.

Example:

```cpp
class BadBuffer {
private:
    int* data;

public:
    BadBuffer(int size) {
        data = new int[size];
    }

    ~BadBuffer() {
        delete[] data;
    }
};
```

This destructor looks correct, but copying this class is dangerous.

---

## 2. Compiler-generated Copy Is Shallow Copy

If we do not define a copy constructor, C++ may generate one.

For raw pointers, the generated copy constructor copies the pointer value, not the pointed-to data.

Example:

```cpp
BadBuffer a(10);
BadBuffer b = a;
```

The default copy behaves like:

```cpp
b.data = a.data;
```

Now both objects point to the same heap array.

When both destructors run:

```cpp
delete[] a.data;
delete[] b.data;
```

This causes double delete.

---

## 3. Rule of Three

If a class needs one of these:

```cpp
~T();
T(const T&);
T& operator=(const T&);
```

It usually needs all three.

They are:

1. Destructor
2. Copy constructor
3. Copy assignment operator

Why?

Because if we manually manage a resource, we must define:

- how to release it
- how to copy it during construction
- how to copy it during assignment

---

## 4. Copy Constructor

The copy constructor creates a new object from an existing object.

```cpp
Buffer(const Buffer& other);
```

Example:

```cpp
Buffer a(10);
Buffer b = a; // copy constructor
```

For owning pointers, copy constructor should usually perform deep copy.

```cpp
Buffer(const Buffer& other)
    : size(other.size), data(new int[other.size]) {
    std::copy(other.data, other.data + size, data);
}
```

Now `a` and `b` own different arrays with the same values.

---

## 5. Copy Assignment Operator

Copy assignment assigns to an existing object.

```cpp
Buffer& operator=(const Buffer& other);
```

Example:

```cpp
Buffer a(10);
Buffer b(20);

b = a; // copy assignment
```

Important points:

- handle self-assignment
- release old resource
- allocate/copy new resource
- return `*this`

Basic version:

```cpp
Buffer& operator=(const Buffer& other) {
    if (this == &other) {
        return *this;
    }

    delete[] data;

    size = other.size;
    data = new int[size];
    std::copy(other.data, other.data + size, data);

    return *this;
}
```

---

## 6. Self-assignment

Self-assignment means:

```cpp
a = a;
```

If we do not handle it, this can be dangerous.

Bad:

```cpp
Buffer& operator=(const Buffer& other) {
    delete[] data;
    size = other.size;
    data = new int[size];
    std::copy(other.data, other.data + size, data);
    return *this;
}
```

If `other` is the same object as `*this`, then after `delete[] data`, `other.data` is also invalid.

So we should check:

```cpp
if (this == &other) {
    return *this;
}
```

---

## 7. Copy-and-swap Idiom

A safer copy assignment style:

```cpp
Buffer& operator=(Buffer other) {
    swap(other);
    return *this;
}
```

Here `other` is a copy of the right-hand side.

Then we swap our current resource with `other`.

When `other` goes out of scope, it destroys the old resource.

Example:

```cpp
void swap(Buffer& other) noexcept {
    std::swap(size, other.size);
    std::swap(data, other.data);
}
```

Advantages:

- handles self-assignment
- gives strong exception safety
- simpler logic

---

## 8. Rule of Five

In modern C++, move semantics adds two more special member functions:

```cpp
T(T&&);
T& operator=(T&&);
```

So Rule of Five says:

If a class manages a resource, consider defining all five:

```cpp
~T();
T(const T&);
T& operator=(const T&);
T(T&&) noexcept;
T& operator=(T&&) noexcept;
```

They are:

1. Destructor
2. Copy constructor
3. Copy assignment
4. Move constructor
5. Move assignment

---

## 9. Move Constructor

Move constructor transfers resource ownership from a temporary or movable object.

```cpp
Buffer(Buffer&& other) noexcept
    : size(other.size), data(other.data) {
    other.size = 0;
    other.data = nullptr;
}
```

The key idea:

```text
Steal the resource, then leave the source object in a valid destructible state.
```

Why set `other.data = nullptr`?

Because when `other` is destroyed, its destructor will call:

```cpp
delete[] other.data;
```

Deleting `nullptr` is safe.

---

## 10. Move Assignment Operator

Move assignment transfers ownership into an existing object.

```cpp
Buffer& operator=(Buffer&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    delete[] data;

    size = other.size;
    data = other.data;

    other.size = 0;
    other.data = nullptr;

    return *this;
}
```

Steps:

1. Check self-assignment.
2. Release current resource.
3. Steal resource from `other`.
4. Reset `other`.
5. Return `*this`.

---

## 11. Why Move Operations Should Often Be noexcept

Move constructor and move assignment should often be marked `noexcept`.

Example:

```cpp
Buffer(Buffer&& other) noexcept;
Buffer& operator=(Buffer&& other) noexcept;
```

Why?

STL containers such as `std::vector` may prefer move during reallocation only if moving is guaranteed not to throw.

If move is not `noexcept`, `vector` may choose copy instead to preserve exception safety.

Interview answer:

```text
Move operations should often be noexcept because standard containers can use them safely during reallocation. If move may throw, containers may fall back to copying.
```

---

## 12. Rule of Zero

Rule of Zero says:

Prefer designing classes that do not manually manage resources.

Use RAII types as members:

```cpp
class GoodBuffer {
private:
    std::vector<int> data;
};
```

or:

```cpp
class Owner {
private:
    std::unique_ptr<int[]> data;
};
```

Then we usually do not need to write:

- destructor
- copy constructor
- copy assignment
- move constructor
- move assignment

Because the member objects already manage their own resources correctly.

This is the best modern C++ style when possible.

---

## 13. Rule of Zero Example

Instead of:

```cpp
class Buffer {
private:
    int* data;
    int size;
};
```

Prefer:

```cpp
class Buffer {
private:
    std::vector<int> data;
};
```

Now:

- destructor is automatic
- copy is deep copy
- move is efficient
- exception safety is handled by `std::vector`

This is much safer.

---

## 14. Special Member Functions Summary

C++ special member functions include:

```cpp
T();                            // default constructor
~T();                           // destructor
T(const T&);                    // copy constructor
T& operator=(const T&);         // copy assignment
T(T&&);                         // move constructor
T& operator=(T&&);              // move assignment
```

These control how objects are created, destroyed, copied, and moved.

---

## 15. `= default`

Use `= default` when the compiler-generated behavior is correct and you want to explicitly request it.

```cpp
class User {
public:
    User() = default;
    ~User() = default;
};
```

This is useful for clarity.

---

## 16. `= delete`

Use `= delete` when an operation should be forbidden.

Example: non-copyable resource owner.

```cpp
class File {
public:
    File(const File&) = delete;
    File& operator=(const File&) = delete;
};
```

This prevents accidental copying.

Common examples:

```cpp
std::unique_ptr<T>
std::mutex
std::thread
```

These are not copyable because copying ownership or thread state would be unsafe or unclear.

---

## 17. Common Interview Questions

### Q1. What is the Rule of Three?

If a class needs a custom destructor, copy constructor, or copy assignment operator, it usually needs all three.

This is because managing resource destruction usually also requires defining copy behavior.

---

### Q2. What is the Rule of Five?

Rule of Five extends Rule of Three with move semantics.

If a class manages a resource, it may need:

```cpp
~T();
T(const T&);
T& operator=(const T&);
T(T&&) noexcept;
T& operator=(T&&) noexcept;
```

---

### Q3. What is the Rule of Zero?

Rule of Zero says we should avoid manually managing resources when possible.

Use RAII types like `std::vector`, `std::string`, `std::unique_ptr`, and standard library containers as members, so the compiler-generated special member functions are correct.

---

### Q4. Why is shallow copy dangerous for owning raw pointers?

Because two objects may end up owning the same resource.

When both destructors run, the same resource may be deleted twice, causing undefined behavior.

---

### Q5. What should a move constructor do?

A move constructor should transfer ownership of the resource from the source object to the new object, then leave the source object in a valid destructible state.

---

### Q6. Why should move constructors often be `noexcept`?

Because standard containers such as `std::vector` may only use move during reallocation if it is `noexcept`.

If moving can throw, the container may use copying instead to maintain exception safety.

---

## 18. Key Takeaways

- Raw owning pointers require careful copy/destruction design.
- Rule of Three: destructor, copy constructor, copy assignment.
- Rule of Five: Rule of Three plus move constructor and move assignment.
- Rule of Zero: prefer standard RAII members and avoid writing special member functions.
- Default copy of raw pointers is shallow copy.
- Shallow copy of ownership can cause double delete.
- Move steals resources and resets the source object.
- Move operations should often be `noexcept`.
- Prefer `std::vector`, `std::string`, `std::unique_ptr`, and RAII types.

## Note: What Changes for Template Type T?

The `int*` examples are simplified to explain ownership, deep copy, move, and double delete.

For a generic type `T`, writing a real container is more complex because:

- `T` may not have a default constructor.
- `T`'s copy constructor may throw.
- `T`'s move constructor may not be `noexcept`.
- `T` may be non-copyable but movable.
- `T` may need a non-trivial destructor.
- Raw storage and object lifetime may need to be managed separately.

A real generic container usually separates memory allocation from object construction:

```cpp
T* data = allocator.allocate(capacity);
std::construct_at(data + i, value);
std::destroy_at(data + i);
allocator.deallocate(data, capacity);
