# 06. Const Correctness

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Const Variable](#2-const-variable)
- [3. Const Reference](#3-const-reference)
- [4. Non-const Reference](#4-non-const-reference)
- [5. Const Pointer vs Pointer to Const](#5-const-pointer-vs-pointer-to-const)
- [6. How to Read Pointer Constness](#6-how-to-read-pointer-constness)
- [7. Const Member Function](#7-const-member-function)
- [8. Const Object Can Only Call Const Member Functions](#8-const-object-can-only-call-const-member-functions)
- [9. Const and Overloading](#9-const-and-overloading)
- [10. Return Type and Const](#10-return-type-and-const)
- [11. Const Does Not Always Mean Deep Const](#11-const-does-not-always-mean-deep-const)
- [12. Logical Constness](#12-logical-constness)
- [13. mutable](#13-mutable)
- [14. Const and Mutex](#14-const-and-mutex)
- [15. Const and Function Parameters](#15-const-and-function-parameters)
- [16. Top-level Const vs Low-level Const](#16-top-level-const-vs-low-level-const)
- [17. Const and std::move](#17-const-and-stdmove)
- [18. Common Interview Questions](#18-common-interview-questions)
- [19. Key Takeaways](#19-key-takeaways)

## Related Code Trap

- [Const Correctness Demo](../code_traps/const_correctness.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

`const` means something cannot be modified through this name, pointer, reference, or member function.

In C++, `const` is a contract.

It helps express:

- read-only parameters
- read-only objects
- member functions that do not modify object state
- pointer constness
- logical constness

The key idea:

```text
const protects an object from being modified through a particular access path.
```

---

## 2. Const Variable

```cpp
const int x = 10;
```

`x` cannot be modified:

```cpp
x = 20; // error
```

A `const` object must be initialized when created.

```cpp
const int y; // error
```

---

## 3. Const Reference

```cpp
void print(const std::string& s);
```

This means:

- no copy
- function cannot modify `s`
- can bind to lvalues
- can bind to temporaries

Example:

```cpp
void print(const std::string& s) {
    std::cout << s << std::endl;
}
```

Valid calls:

```cpp
std::string name = "Yuqi";
print(name);

print("hello");
print(std::string("temporary"));
```

A `const T&` is commonly used for large read-only input objects.

---

## 4. Non-const Reference

```cpp
void modify(std::string& s) {
    s += "!";
}
```

This means the function may modify the caller's object.

```cpp
std::string name = "Yuqi";
modify(name); // OK
```

But this is not allowed:

```cpp
modify("hello"); // error
```

Why?

Because `"hello"` is not a modifiable `std::string` lvalue.

Non-const references cannot bind to temporaries.

---

## 5. Const Pointer vs Pointer to Const

This is a classic C++ interview topic.

### Pointer to const

```cpp
const int* p = &x;
```

or equivalently:

```cpp
int const* p = &x;
```

This means:

```text
*p cannot be modified through p.
p itself can point somewhere else.
```

Example:

```cpp
int a = 10;
int b = 20;

const int* p = &a;

*p = 30; // error
p = &b;  // OK
```

---

### Const pointer

```cpp
int* const p = &a;
```

This means:

```text
p cannot point somewhere else.
*p can be modified.
```

Example:

```cpp
int a = 10;
int b = 20;

int* const p = &a;

*p = 30; // OK
p = &b;  // error
```

---

### Const pointer to const

```cpp
const int* const p = &a;
```

This means:

```text
p cannot point somewhere else.
*p cannot be modified through p.
```

Example:

```cpp
const int* const p = &a;

*p = 30; // error
p = &b;  // error
```

---

## 6. How to Read Pointer Constness

Read from right to left.

```cpp
const int* p;
```

`p` is a pointer to const int.

```cpp
int* const p;
```

`p` is a const pointer to int.

```cpp
const int* const p;
```

`p` is a const pointer to const int.

Another useful rule:

```text
const before * applies to the pointed-to object.
const after * applies to the pointer itself.
```

---

## 7. Const Member Function

A member function can be marked `const`:

```cpp
class User {
private:
    std::string name;

public:
    const std::string& getName() const {
        return name;
    }
};
```

The second `const` means:

```cpp
getName() const
```

This function promises not to modify the object.

Inside a `const` member function, `this` is treated like:

```cpp
const User* this;
```

So data members cannot be modified.

---

## 8. Const Object Can Only Call Const Member Functions

Example:

```cpp
class User {
public:
    void print() const {
        std::cout << "print\n";
    }

    void update() {
        std::cout << "update\n";
    }
};

const User u;

u.print();  // OK
u.update(); // error
```

Why?

Because `u` is const.

Calling a non-const member function would pass a non-const `this` pointer:

```cpp
User* this
```

But a const object can only provide:

```cpp
const User* this
```

So only const member functions can be called.

---

## 9. Const and Overloading

C++ allows overloading based on constness of member functions.

Example:

```cpp
class VectorLike {
private:
    std::vector<int> data;

public:
    int& at(size_t i) {
        return data[i];
    }

    const int& at(size_t i) const {
        return data[i];
    }
};
```

Usage:

```cpp
VectorLike v;
v.at(0) = 10; // calls non-const version

const VectorLike cv;
int x = cv.at(0); // calls const version
```

This is common in STL containers.

For example, `operator[]` often has both const and non-const versions.

---

## 10. Return Type and Const

### Returning non-const reference

```cpp
int& get() {
    return value;
}
```

Caller can modify the internal value:

```cpp
obj.get() = 10;
```

This exposes mutable access.

---

### Returning const reference

```cpp
const int& get() const {
    return value;
}
```

Caller can read but cannot modify:

```cpp
int x = obj.get();
obj.get() = 10; // error
```

For large internal objects, returning `const T&` avoids copy while preventing modification.

Example:

```cpp
const std::string& getName() const {
    return name;
}
```

But be careful: returning references to internal data exposes lifetime dependency.

---

## 11. Const Does Not Always Mean Deep Const

For raw pointers:

```cpp
class Wrapper {
private:
    int* ptr;

public:
    void setThroughPointer(int value) const {
        *ptr = value; // allowed
    }
};
```

Why is this allowed?

Inside a const member function, the member pointer `ptr` itself cannot be changed.

But the object pointed to by `ptr` may still be modified.

This is called shallow constness.

For pointer members:

```cpp
int* ptr;
```

In a const member function, it behaves like:

```cpp
int* const ptr;
```

not:

```cpp
const int* ptr;
```

So the pointer cannot be reseated, but the pointee can be modified.

---

## 12. Logical Constness

Sometimes a member function is logically const even though it modifies internal cache.

Example:

```cpp
class ExpensiveObject {
private:
    mutable bool cached = false;
    mutable int cachedValue = 0;

public:
    int getValue() const {
        if (!cached) {
            cachedValue = compute();
            cached = true;
        }
        return cachedValue;
    }

private:
    int compute() const {
        return 42;
    }
};
```

`getValue()` is logically const because from the user's perspective, it does not change the object's meaningful state.

But internally, it updates a cache.

This is why C++ has `mutable`.

---

## 13. mutable

`mutable` allows a data member to be modified even inside a const member function.

Example:

```cpp
class Counter {
private:
    mutable int accessCount = 0;

public:
    int get() const {
        accessCount++;
        return 42;
    }
};
```

Use cases:

- caching
- logging
- statistics
- lazy computation
- mutex inside const member functions

But do not overuse it.

---

## 14. Const and Mutex

A common real-world use of `mutable`:

```cpp
class ThreadSafeCounter {
private:
    int value = 0;
    mutable std::mutex m;

public:
    int get() const {
        std::lock_guard<std::mutex> lock(m);
        return value;
    }
};
```

Why `mutable std::mutex m`?

Because `get()` is logically const: it only reads `value`.

But locking the mutex technically modifies the mutex state.

So the mutex needs to be `mutable`.

---

## 15. Const and Function Parameters

Parameter choice:

```cpp
void f(int x);
```

For small types, pass by value.

```cpp
void f(const std::string& s);
```

For large read-only objects, pass by const reference.

```cpp
void f(std::string& s);
```

For objects that must be modified.

```cpp
void f(const std::string* s);
```

Pointer to const string. The pointer may be null, and the string cannot be modified through this pointer.

```cpp
void f(std::string* const s);
```

Const pointer to string. The pointer itself cannot be reseated inside the function, but the string can be modified.

In function parameters, top-level const on value parameters is usually not important for callers:

```cpp
void f(const int x);
```

This only means the local copy `x` cannot be modified inside `f`.

---

## 16. Top-level Const vs Low-level Const

### Top-level const

Top-level const means the object itself is const.

```cpp
int* const p = &x;
```

The pointer `p` itself is const.

For value parameters:

```cpp
void f(const int x);
```

`x` is a const local copy.

Top-level const is often ignored in function declarations for callers.

---

### Low-level const

Low-level const means the pointed-to or referred-to object is const.

```cpp
const int* p;
const int& r = x;
```

Low-level const matters for type compatibility.

Example:

```cpp
int x = 10;
const int* p = &x; // OK

const int y = 20;
int* q = &y; // error
```

You cannot use a non-const pointer to point to a const object.

---

## 17. Const and std::move

This is a common trap.

```cpp
const std::string s = "hello";
std::string t = std::move(s);
```

This usually copies, not moves.

Why?

`std::move(s)` produces:

```cpp
const std::string&&
```

But typical move constructor expects:

```cpp
std::string&&
```

A move usually needs to modify the source object.

Since `s` is const, it cannot be modified.

So the copy constructor is selected.

Key takeaway:

```text
Do not expect real moves from const objects.
```

---

## 18. Common Interview Questions

### Q1. What is const correctness?

Const correctness means using `const` consistently to express which objects and member functions are not allowed to modify observable state.

It makes interfaces safer and clearer.

---

### Q2. What does a const member function mean?

A const member function promises not to modify the observable state of the object.

Inside it, `this` is treated as a pointer to const object.

```cpp
const T* this
```

Therefore it cannot modify normal data members or call non-const member functions.

---

### Q3. Why can a const object only call const member functions?

Because a const object provides a `const this` pointer.

Non-const member functions require a non-const `this` pointer, so they cannot be called on a const object.

---

### Q4. What is the difference between `const int* p` and `int* const p`?

```cpp
const int* p;
```

means pointer to const int. The pointed-to int cannot be modified through `p`, but `p` can point somewhere else.

```cpp
int* const p;
```

means const pointer to int. The pointer cannot point somewhere else, but the pointed-to int can be modified.

---

### Q5. What is `mutable` used for?

`mutable` allows a data member to be modified inside const member functions.

It is used for internal state that does not affect the object's logical state, such as caches, counters, or mutexes.

---

### Q6. Why might `std::move` on a const object copy instead of move?

Because `std::move(const T)` produces `const T&&`.

Most move constructors require `T&&` because moving usually modifies the source.

Therefore the copy constructor may be selected instead.

---

## 19. Key Takeaways

- `const` is a contract about non-modification.
- `const T&` is used for large read-only input.
- `T&` is used when modification is needed.
- `const int* p`: pointer to const int.
- `int* const p`: const pointer to int.
- `const` member functions can be called on const objects.
- Const objects cannot call non-const member functions.
- Const member functions treat `this` as `const T*`.
- Member functions can be overloaded by constness.
- `mutable` supports logical constness.
- Const does not always mean deep const for raw pointer members.
- `std::move` on const objects usually does not really move.
