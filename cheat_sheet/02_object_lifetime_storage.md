# 02. Object Lifetime and Storage Duration

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Storage vs Lifetime](#2-storage-vs-lifetime)
- [3. Automatic Storage Duration](#3-automatic-storage-duration)
- [4. Dynamic Storage Duration](#4-dynamic-storage-duration)
- [5. Static Storage Duration](#5-static-storage-duration)
- [6. Thread Storage Duration](#6-thread-storage-duration)
- [7. Scope and Lifetime](#7-scope-and-lifetime)
- [8. Destructor Timing](#8-destructor-timing)
- [9. Dangling Pointer](#9-dangling-pointer)
- [10. Dangling Reference](#10-dangling-reference)
- [11. Return by Value Is Fine](#11-return-by-value-is-fine)
- [12. Returning Dynamic Object](#12-returning-dynamic-object)
- [13. Returning Reference to Static Local](#13-returning-reference-to-static-local)
- [14. Common Interview Questions](#14-common-interview-questions)
- [15. Key Takeaways](#15-key-takeaways)

## Related Code Trap

- [Object Lifetime / Storage Demo](../code_traps/object_lifetime_storage.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

In C++, it is not enough to know the type of an object.

We also need to know:

- Where is the object stored?
- Who owns the object?
- When does its lifetime begin?
- When does its lifetime end?
- Is any pointer or reference still accessing it after destruction?

Many C++ bugs come from using an object after its lifetime has ended.

---

## 2. Storage vs Lifetime

Storage means memory.

Lifetime means the period during which an object is valid and can be used.

Sometimes storage exists before the object lifetime begins.

Example:

```cpp
alignas(T) unsigned char buffer[sizeof(T)];
```

This gives us raw storage, but there is no `T` object yet.

The `T` object's lifetime begins only after placement new:

```cpp
new (buffer) T();
```

This distinction matters in low-level C++, allocators, `std::optional`, and custom containers.

---

## 3. Automatic Storage Duration

Local variables usually have automatic storage duration.

```cpp
void f() {
    int x = 10;
    std::string s = "hello";
}
```

`x` and `s` are created when execution reaches their declarations.

They are destroyed when leaving the scope.

```cpp
void f() {
    std::string s = "hello";
} // s destructor runs here
```

Automatic objects are often informally described as stack objects.

---

## 4. Dynamic Storage Duration

Dynamic objects are created with `new`.

```cpp
int* p = new int(10);
delete p;
```

The object lives until it is explicitly destroyed with `delete`.

If we forget to delete it, we get a memory leak.

Modern C++ prefers smart pointers:

```cpp
auto p = std::make_unique<int>(10);
```

Now the object is automatically deleted when the `unique_ptr` is destroyed.

---

## 5. Static Storage Duration

Objects with static storage duration live from program startup to program termination.

Examples:

```cpp
int globalX = 10;
```

```cpp
void f() {
    static int count = 0;
    count++;
}
```

A static local variable is initialized only once and keeps its value across function calls.

Example:

```cpp
int nextId() {
    static int id = 0;
    return ++id;
}
```

Each call continues from the previous value.

---

## 6. Thread Storage Duration

```cpp
thread_local int x = 0;
```

Each thread gets its own copy of `x`.

This will be discussed later in the multithreading section.

---

## 7. Scope and Lifetime

Scope controls where a name can be used.

Lifetime controls when an object is valid.

They are related but not exactly the same.

Example:

```cpp
int* p = nullptr;

{
    int x = 10;
    p = &x;
}

std::cout << *p << std::endl; // undefined behavior
```

`p` still exists, but `x` has already been destroyed.

So `p` is a dangling pointer.

---

## 8. Destructor Timing

### Automatic Object

```cpp
void f() {
    std::string s = "hello";
}
```

`s` is destroyed automatically when leaving the scope.

---

### Dynamic Object

```cpp
std::string* p = new std::string("hello");
delete p;
```

The string is destroyed only when `delete p` is executed.

---

### Smart Pointer

```cpp
void f() {
    auto p = std::make_unique<std::string>("hello");
}
```

When `p` leaves scope, the `unique_ptr` destructor runs and deletes the managed object.

This is the foundation of RAII.

---

## 9. Dangling Pointer

Wrong:

```cpp
int* badPtr() {
    int x = 10;
    return &x;
}
```

`x` is a local object.

Its lifetime ends when the function returns.

The returned pointer points to an object that no longer exists.

Using it is undefined behavior.

---

## 10. Dangling Reference

Wrong:

```cpp
int& badRef() {
    int x = 10;
    return x;
}
```

The returned reference refers to a destroyed local object.

This is undefined behavior.

---

## 11. Return by Value Is Fine

Correct:

```cpp
int goodValue() {
    int x = 10;
    return x;
}
```

This returns the value of `x`, not a pointer or reference to the local object.

For class types, modern C++ may use RVO or NRVO to avoid unnecessary copying.

Example:

```cpp
std::string makeName() {
    std::string s = "Yuqi";
    return s;
}
```

This is fine.

---

## 12. Returning Dynamic Object

This is not dangling:

```cpp
int* makeIntRaw() {
    int* p = new int(10);
    return p;
}
```

The object is dynamically allocated and does not die when the function returns.

However, this is dangerous because the caller must remember to delete it.

```cpp
int* p = makeIntRaw();
delete p;
```

Modern C++ prefers:

```cpp
std::unique_ptr<int> makeInt() {
    return std::make_unique<int>(10);
}
```

This clearly transfers ownership to the caller.

---

## 13. Returning Reference to Static Local

This is valid:

```cpp
int& counter() {
    static int x = 0;
    return x;
}
```

`x` has static storage duration.

It does not die when the function returns.

But this creates shared hidden state.

Example:

```cpp
int& r = counter();
r++;
```

This modifies the static `x`.

---

## 14. Common Interview Questions

### Q1. What is object lifetime?

Object lifetime is the period during which an object exists and can be used safely.

Accessing an object before its lifetime begins or after its lifetime ends is undefined behavior.

---

### Q2. What is the difference between automatic and dynamic storage duration?

Automatic objects are created and destroyed automatically based on scope.

Dynamic objects are created with `new` and live until explicitly destroyed with `delete`, or until managed by a smart pointer.

---

### Q3. Why is returning a pointer to a local variable wrong?

Because the local variable is destroyed when the function returns.

The returned pointer becomes dangling.

Dereferencing it is undefined behavior.

---

### Q4. Why is returning by value safe?

Because the function returns a value, not a reference or pointer to the local object.

Modern C++ can optimize the return using RVO or move semantics.

---

### Q5. Why is returning a reference to a static local variable valid?

Because static local variables have static storage duration.

They remain alive after the function returns.

However, they represent shared state and should be used carefully.

---

## 15. Key Takeaways

- Storage is memory; lifetime is object validity.
- Automatic objects are destroyed when leaving scope.
- Dynamic objects live until deleted or managed by smart pointers.
- Static objects live until program termination.
- Returning pointer/reference to local objects causes dangling.
- Returning by value is safe.
- Smart pointers make ownership and lifetime clearer.
- Many C++ bugs are lifetime bugs.
