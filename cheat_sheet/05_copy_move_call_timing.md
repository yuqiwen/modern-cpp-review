# 05. Copy and Move Call Timing

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Copy Constructor](#2-copy-constructor)
- [3. Copy Assignment Operator](#3-copy-assignment-operator)
- [4. Move Constructor](#4-move-constructor)
- [5. Move Assignment Operator](#5-move-assignment-operator)
- [6. Quick Table](#6-quick-table)
- [7. Initialization vs Assignment](#7-initialization-vs-assignment)
- [8. Function Parameter Passing](#8-function-parameter-passing)
- [9. Function Return by Value](#9-function-return-by-value)
- [10. Mandatory Copy Elision](#10-mandatory-copy-elision)
- [11. std::move Does Not Move by Itself](#11-stdmove-does-not-move-by-itself)
- [12. Moved-from Object](#12-moved-from-object)
- [13. Move vs Copy for const Objects](#13-move-vs-copy-for-const-objects)
- [14. lvalue and rvalue Basic Intuition](#14-lvalue-and-rvalue-basic-intuition)
- [15. Common Interview Questions](#15-common-interview-questions)
- [16. Key Takeaways](#16-key-takeaways)

## Related Code Trap

- [Copy / Move Call Timing Demo](../code_traps/copy_move_call_timing.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

The most important distinction is:

```text
Construction creates a new object.
Assignment modifies an existing object.
```

So:

```cpp
Buffer b = a;
```

calls copy constructor.

But:

```cpp
b = a;
```

calls copy assignment operator.

This is one of the most common C++ interview traps.

---

## 2. Copy Constructor

A copy constructor creates a new object from an existing lvalue object.

```cpp
T(const T& other);
```

Example:

```cpp
Buffer a(10);
Buffer b = a;
```

`b` does not exist before this line.

So this line constructs `b` from `a`.

It calls:

```cpp
Buffer(const Buffer& other);
```

Equivalent style:

```cpp
Buffer b(a);
```

Also copy construction.

---

## 3. Copy Assignment Operator

Copy assignment modifies an already existing object.

```cpp
T& operator=(const T& other);
```

Example:

```cpp
Buffer a(10);
Buffer b(20);

b = a;
```

`b` already exists.

So this line does not construct `b`.

It calls:

```cpp
Buffer& operator=(const Buffer& other);
```

---

## 4. Move Constructor

A move constructor creates a new object from an rvalue or explicitly moved object.

```cpp
T(T&& other) noexcept;
```

Example:

```cpp
Buffer a(10);
Buffer b = std::move(a);
```

`b` does not exist before this line.

So this calls move constructor:

```cpp
Buffer(Buffer&& other) noexcept;
```

Important:

```cpp
std::move(a)
```

does not move anything by itself.

It only casts `a` to an rvalue reference, making it eligible to bind to move constructor or move assignment.

The actual move happens inside the move constructor or move assignment operator.

---

## 5. Move Assignment Operator

Move assignment modifies an already existing object by taking resources from an rvalue.

```cpp
T& operator=(T&& other) noexcept;
```

Example:

```cpp
Buffer a(10);
Buffer b(20);

b = std::move(a);
```

`b` already exists.

So this calls move assignment operator:

```cpp
Buffer& operator=(Buffer&& other) noexcept;
```

---

## 6. Quick Table

```cpp
Buffer a(10);

Buffer b = a;             // copy constructor
Buffer c(a);              // copy constructor

Buffer d = std::move(a);  // move constructor
Buffer e(std::move(b));   // move constructor

c = d;                    // copy assignment
c = std::move(d);         // move assignment
```

Rule:

```text
New object + lvalue source  -> copy constructor
New object + rvalue source  -> move constructor
Existing object + lvalue    -> copy assignment
Existing object + rvalue    -> move assignment
```

---

## 7. Initialization vs Assignment

This is initialization:

```cpp
Buffer b = a;
```

Even though it uses `=`, it is not assignment.

Because `b` is being created.

This is assignment:

```cpp
b = a;
```

Because `b` already exists.

Interview trap:

```cpp
Buffer b = a; // copy constructor, not copy assignment
```

---

## 8. Function Parameter Passing

Passing by value creates a new parameter object.

```cpp
void f(Buffer x);
```

Call with lvalue:

```cpp
Buffer a(10);
f(a);
```

This copy-constructs parameter `x` from `a`.

Call with rvalue:

```cpp
f(Buffer(10));
```

This may move-construct or be optimized by copy elision.

Call with explicit move:

```cpp
f(std::move(a));
```

This move-constructs parameter `x` from `a`.

---

## 9. Function Return by Value

```cpp
Buffer makeBuffer() {
    Buffer temp(10);
    return temp;
}
```

Modern C++ often uses NRVO:

```text
Named Return Value Optimization
```

The compiler may construct the returned object directly in the caller's storage.

So copy/move may not happen.

Example:

```cpp
Buffer b = makeBuffer();
```

May only call the normal constructor.

No copy or move is required if copy elision happens.

---

## 10. Mandatory Copy Elision

In C++17, some copy elision cases are mandatory.

Example:

```cpp
Buffer makeBuffer() {
    return Buffer(10);
}
```

The returned object is constructed directly in the caller.

So this usually does not call copy or move constructor.

```cpp
Buffer b = makeBuffer();
```

This can construct `b` directly.

Important interview point:

```text
Return by value is not necessarily expensive in modern C++.
```

---

## 11. std::move Does Not Move by Itself

This is extremely important.

```cpp
std::move(x)
```

does not move data.

It only converts `x` into an rvalue expression.

Example:

```cpp
Buffer b = std::move(a);
```

The actual move is done by:

```cpp
Buffer(Buffer&& other);
```

If the type has no move constructor, then copy constructor may still be called.

Example:

```cpp
const Buffer a(10);
Buffer b = std::move(a);
```

This usually calls copy constructor, not move constructor.

Why?

Because `std::move(a)` becomes:

```cpp
const Buffer&&
```

But move constructor usually expects:

```cpp
Buffer&&
```

A non-const rvalue reference cannot bind to a const object.

So copy constructor is used:

```cpp
Buffer(const Buffer& other);
```

---

## 12. Moved-from Object

After moving from an object, the moved-from object must remain valid but its value is unspecified.

Example:

```cpp
std::string s = "hello";
std::string t = std::move(s);
```

After this:

```cpp
s
```

is still valid.

You can destroy it, assign a new value to it, or call methods with valid preconditions.

But you should not assume it still contains `"hello"`.

Interview answer:

```text
A moved-from object is valid but in an unspecified state.
```

For custom classes, we usually reset the moved-from object to a safe destructible state.

Example:

```cpp
other.data = nullptr;
other.size = 0;
```

---

## 13. Move vs Copy for const Objects

Move usually requires modifying the source object.

For example, stealing a pointer requires setting the source pointer to `nullptr`.

Therefore move constructor usually takes:

```cpp
T(T&& other);
```

not:

```cpp
T(const T&& other);
```

If the source object is const, it cannot be modified, so true moving is usually impossible.

Example:

```cpp
const std::string s = "hello";
std::string t = std::move(s);
```

This usually copies instead of moving.

Key takeaway:

```text
Do not use std::move on const objects expecting a real move.
```

---

## 14. lvalue and rvalue Basic Intuition

An lvalue has identity and can usually appear on the left side of assignment.

```cpp
int x = 10;
```

`x` is an lvalue.

An rvalue is usually a temporary value.

```cpp
10
x + 1
Buffer(10)
```

These are rvalues.

Move operations are selected for rvalues.

`std::move(x)` turns the expression `x` into an rvalue expression.

---

## 15. Common Interview Questions

### Q1. What is the difference between copy constructor and copy assignment?

Copy constructor creates a new object from an existing object.

Copy assignment modifies an already existing object.

```cpp
Buffer b = a; // copy constructor
b = a;        // copy assignment
```

---

### Q2. What is the difference between move constructor and move assignment?

Move constructor creates a new object by taking resources from an rvalue.

Move assignment modifies an existing object by releasing its current resource and taking resources from an rvalue.

```cpp
Buffer b = std::move(a); // move constructor
b = std::move(a);        // move assignment if b already exists
```

---

### Q3. Does `std::move` move an object?

No.

`std::move` only casts an expression to an rvalue reference. It enables move constructor or move assignment to be selected.

The actual move is performed by the move constructor or move assignment operator.

---

### Q4. What is the state of a moved-from object?

A moved-from object is valid but in an unspecified state.

It can be destroyed or assigned a new value, but we should not rely on its previous value.

---

### Q5. Why might `std::move` on a const object copy instead of move?

Because moving usually modifies the source object.

`std::move(const T)` produces `const T&&`, which cannot bind to a typical move constructor `T(T&&)`.

So the copy constructor `T(const T&)` may be selected instead.

---

### Q6. Why is return by value efficient in modern C++?

Modern C++ supports copy elision such as RVO and NRVO.

In many cases, the returned object is constructed directly in the caller's storage, so no copy or move is needed.

---

## 16. Key Takeaways

- New object + lvalue source: copy constructor.
- Existing object + lvalue source: copy assignment.
- New object + rvalue source: move constructor.
- Existing object + rvalue source: move assignment.
- `Buffer b = a;` is construction, not assignment.
- `std::move` does not move by itself.
- Moved-from objects are valid but unspecified.
- Moving from const usually does not really move.
- Return by value is efficient due to copy elision.
