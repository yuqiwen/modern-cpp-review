# 09. C++ Casts

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. static_cast](#2-static_cast)
- [3. static_cast for Upcast](#3-static_cast-for-upcast)
- [4. static_cast for Downcast](#4-static_cast-for-downcast)
- [5. dynamic_cast](#5-dynamic_cast)
- [6. dynamic_cast with Reference](#6-dynamic_cast-with-reference)
- [7. static_cast vs dynamic_cast for Downcast](#7-static_cast-vs-dynamic_cast-for-downcast)
- [8. dynamic_cast Requires Polymorphic Base](#8-dynamic_cast-requires-polymorphic-base)
- [9. const_cast](#9-const_cast)
- [10. const_cast Danger](#10-const_cast-danger)
- [11. reinterpret_cast](#11-reinterpret_cast)
- [12. reinterpret_cast Is Not Type Conversion](#12-reinterpret_cast-is-not-type-conversion)
- [13. C-style Cast](#13-c-style-cast)
- [14. Upcast and Downcast](#14-upcast-and-downcast)
- [15. Cross-cast](#15-cross-cast)
- [16. Casting and Ownership](#16-casting-and-ownership)
- [17. dynamic_pointer_cast](#17-dynamic_pointer_cast)
- [18. Common Interview Questions](#18-common-interview-questions)
- [19. Key Takeaways](#19-key-takeaways)

## Related Code Trap

- [C++ Casts Demo](../code_traps/cpp_casts.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

C++ provides explicit cast operators to make conversions clear.

Main casts:

```cpp
static_cast<T>(expr)
dynamic_cast<T>(expr)
const_cast<T>(expr)
reinterpret_cast<T>(expr)
```

Avoid C-style casts:

```cpp
(T)expr
```

because they can silently perform different kinds of casts and hide danger.

---

## 2. static_cast

`static_cast` performs compile-time checked conversions.

Common uses:

- numeric conversion
- upcast / downcast in inheritance when you are sure
- enum conversion
- calling explicit constructors/conversions

Example:

```cpp
double d = 3.14;
int x = static_cast<int>(d);
```

This gives:

```text
x = 3
```

---

## 3. static_cast for Upcast

Derived-to-base conversion is safe.

```cpp
class Base {};
class Derived : public Base {};

Derived d;
Base* b = static_cast<Base*>(&d);
```

Actually, this cast is often implicit:

```cpp
Base* b = &d;
```

Because every `Derived` object contains a `Base` subobject.

---

## 4. static_cast for Downcast

Base-to-derived conversion with `static_cast` is allowed if the inheritance relationship exists.

```cpp
Base* b = getBasePointer();
Derived* d = static_cast<Derived*>(b);
```

But this does not check the real runtime type.

If `b` does not actually point to a `Derived` object, using `d` is undefined behavior.

So:

```cpp
static_cast<Derived*>(b)
```

means:

```text
I promise this Base* really points to a Derived object.
```

No runtime verification.

---

## 5. dynamic_cast

`dynamic_cast` performs runtime-checked casts in polymorphic class hierarchies.

A class must be polymorphic, meaning it has at least one virtual function.

Usually the base class has a virtual destructor:

```cpp
class Base {
public:
    virtual ~Base() = default;
};
```

Downcast example:

```cpp
Base* b = getBasePointer();

if (Derived* d = dynamic_cast<Derived*>(b)) {
    // success: b really points to a Derived object
} else {
    // failed: b does not point to Derived
}
```

For pointer casts, failed `dynamic_cast` returns `nullptr`.

---

## 6. dynamic_cast with Reference

For references:

```cpp
Derived& d = dynamic_cast<Derived&>(baseRef);
```

If it fails, it throws:

```cpp
std::bad_cast
```

So pointer version is often easier when failure is expected:

```cpp
if (auto* d = dynamic_cast<Derived*>(b)) {
    // success
}
```

---

## 7. static_cast vs dynamic_cast for Downcast

Example:

```cpp
class Animal {
public:
    virtual ~Animal() = default;
};

class Dog : public Animal {
public:
    void bark();
};

class Cat : public Animal {
public:
    void meow();
};

Animal* a = new Cat();
```

Dangerous:

```cpp
Dog* d = static_cast<Dog*>(a);
d->bark(); // undefined behavior
```

Safe:

```cpp
Dog* d = dynamic_cast<Dog*>(a);
if (d) {
    d->bark();
} else {
    std::cout << "not a dog\n";
}
```

Interview summary:

```text
Use static_cast when the conversion is known to be valid.
Use dynamic_cast when downcasting polymorphic objects and the runtime type may vary.
```

---

## 8. dynamic_cast Requires Polymorphic Base

This works:

```cpp
class Base {
public:
    virtual ~Base() = default;
};

class Derived : public Base {};
```

```cpp
Base* b = new Derived();
Derived* d = dynamic_cast<Derived*>(b);
```

This does not work:

```cpp
class Base {};
class Derived : public Base {};
```

Because `Base` has no virtual function, it is not polymorphic.

`dynamic_cast` needs runtime type information, usually provided through the polymorphic object model.

---

## 9. const_cast

`const_cast` can add or remove `const` / `volatile`.

Example:

```cpp
void legacyApi(char* p);

const char* s = "hello";
// legacyApi(s); // error
legacyApi(const_cast<char*>(s));
```

But this is only safe if the underlying object is actually non-const.

---

## 10. const_cast Danger

Example:

```cpp
const int x = 10;
int* p = const_cast<int*>(&x);
*p = 20; // undefined behavior
```

Why?

Because `x` was originally declared as const.

Removing const and modifying a truly const object is undefined behavior.

Safe-ish case:

```cpp
int x = 10;
const int& r = x;

int& m = const_cast<int&>(r);
m = 20; // OK, because original object x is non-const
```

Rule:

```text
const_cast can remove const from the access path, but it does not make a truly const object mutable.
```

---

## 11. reinterpret_cast

`reinterpret_cast` is a low-level cast that reinterprets bits or addresses.

Examples:

```cpp
std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
```

or:

```cpp
char* bytes = reinterpret_cast<char*>(&obj);
```

It is dangerous because it can easily violate type aliasing, alignment, and object lifetime rules.

Use it only for low-level systems programming, serialization, hardware interfaces, or carefully controlled cases.

---

## 12. reinterpret_cast Is Not Type Conversion

Example:

```cpp
int x = 65;
char* p = reinterpret_cast<char*>(&x);
```

This does not convert integer value `65` to character `'A'`.

It treats the memory bytes of `x` as `char` bytes.

So `reinterpret_cast` is about memory representation, not semantic conversion.

For numeric conversion use:

```cpp
static_cast<char>(x);
```

---

## 13. C-style Cast

C-style cast:

```cpp
Derived* d = (Derived*)basePtr;
```

Problem:

It may behave like:

- `static_cast`
- `const_cast`
- `reinterpret_cast`
- or a combination

This hides intent.

Modern C++ prefers explicit named casts.

---

## 14. Upcast and Downcast

### Upcast

Derived to base:

```cpp
Derived* d = new Derived();
Base* b = d;
```

Safe and often implicit.

### Downcast

Base to derived:

```cpp
Base* b = getObject();
Derived* d = dynamic_cast<Derived*>(b);
```

Needs care because not every `Base*` points to a `Derived`.

---

## 15. Cross-cast

In multiple inheritance, `dynamic_cast` can also cast across sibling base types when the object is polymorphic.

Example:

```cpp
class InterfaceA {
public:
    virtual ~InterfaceA() = default;
};

class InterfaceB {
public:
    virtual ~InterfaceB() = default;
};

class Impl : public InterfaceA, public InterfaceB {};
```

```cpp
InterfaceA* a = new Impl();
InterfaceB* b = dynamic_cast<InterfaceB*>(a);
```

This may adjust the pointer correctly if the real object is `Impl`.

This is another reason `dynamic_cast` is useful in polymorphic hierarchies.

---

## 16. Casting and Ownership

Casting raw pointers does not change ownership.

Example:

```cpp
Base* b = new Derived();
Derived* d = dynamic_cast<Derived*>(b);
```

Both `b` and `d` point into the same object.

Do not delete both.

If ownership is needed, use smart pointers carefully.

---

## 17. dynamic_pointer_cast

For `std::shared_ptr`, use:

```cpp
std::dynamic_pointer_cast<Derived>(basePtr)
```

Example:

```cpp
std::shared_ptr<Base> b = std::make_shared<Derived>();

std::shared_ptr<Derived> d = std::dynamic_pointer_cast<Derived>(b);
```

If the cast succeeds, `d` shares ownership of the same object.

If it fails, `d` is empty.

There are also:

```cpp
std::static_pointer_cast<T>(p)
std::const_pointer_cast<T>(p)
std::reinterpret_pointer_cast<T>(p) // C++17
```

For `unique_ptr`, downcasting ownership is more manual and should be done carefully.

---

## 18. Common Interview Questions

### Q1. What is the difference between static_cast and dynamic_cast?

`static_cast` is checked at compile time and does not verify the runtime type.

`dynamic_cast` performs a runtime check for polymorphic classes.

For downcasting from base to derived, `dynamic_cast` is safer when the actual runtime type is uncertain.

---

### Q2. When does dynamic_cast return nullptr?

For pointer casts, `dynamic_cast` returns `nullptr` when the object is not actually of the target type.

Example:

```cpp
Animal* a = new Cat();
Dog* d = dynamic_cast<Dog*>(a); // nullptr
```

---

### Q3. Why does dynamic_cast require a polymorphic base?

Because it needs runtime type information to check the actual dynamic type.

A polymorphic class has virtual functions, which usually enables RTTI support for dynamic casts.

---

### Q4. What is const_cast used for?

`const_cast` changes const or volatile qualification.

It can remove const from an access path, but modifying an object that was originally declared const is undefined behavior.

---

### Q5. What is reinterpret_cast?

`reinterpret_cast` performs low-level reinterpretation of bits or addresses.

It does not perform semantic conversion and can easily cause undefined behavior if used incorrectly.

---

### Q6. Why avoid C-style casts in C++?

Because a C-style cast can silently perform several kinds of conversions.

Named C++ casts make the programmer's intent clearer and make dangerous casts easier to spot.

---

## 19. Key Takeaways

- Prefer named C++ casts over C-style casts.
- `static_cast` is for known safe compile-time conversions.
- `dynamic_cast` is for runtime-checked polymorphic casts.
- Failed pointer `dynamic_cast` returns `nullptr`.
- Failed reference `dynamic_cast` throws `std::bad_cast`.
- `const_cast` only changes const/volatile qualification.
- Removing const and modifying a truly const object is undefined behavior.
- `reinterpret_cast` is low-level and dangerous.
- Casting does not transfer ownership.
- For `shared_ptr`, use `dynamic_pointer_cast` / `static_pointer_cast`.
