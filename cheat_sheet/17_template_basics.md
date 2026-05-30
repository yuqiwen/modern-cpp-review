# 17. Template Basics

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Function Template](#2-function-template)
- [3. Template Type Deduction](#3-template-type-deduction)
- [4. Class Template](#4-class-template)
- [5. typename vs class in Template Parameters](#5-typename-vs-class-in-template-parameters)
- [6. Non-type Template Parameters](#6-non-type-template-parameters)
- [7. Template Instantiation](#7-template-instantiation)
- [8. Template Requirements Are Implicit](#8-template-requirements-are-implicit)
- [9. Explicit Template Argument](#9-explicit-template-argument)
- [10. Function Template Overloading](#10-function-template-overloading)
- [11. Template Specialization](#11-template-specialization)
- [12. Function Template Specialization](#12-function-template-specialization)
- [13. Partial Specialization](#13-partial-specialization)
- [14. Templates and Header Files](#14-templates-and-header-files)
- [15. Dependent Names](#15-dependent-names)
- [16. `typename` in Dependent Type](#16-typename-in-dependent-type)
- [17. Type Alias in Templates](#17-type-alias-in-templates)
- [18. Template Code Bloat](#18-template-code-bloat)
- [19. Templates vs Virtual Functions](#19-templates-vs-virtual-functions)
- [20. Common Interview Questions](#20-common-interview-questions)
- [21. Key Takeaways](#21-key-takeaways)

## Related Code Trap

- [Template Basics Demo](../code_traps/template_basics.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

Templates allow C++ to write generic code.

Instead of writing:

```cpp
int maxInt(int a, int b);
double maxDouble(double a, double b);
std::string maxString(std::string a, std::string b);
```

We can write:

```cpp
template <typename T>
T maxValue(T a, T b) {
    return a > b ? a : b;
}
```

The compiler generates type-specific versions when the template is used.

This is called template instantiation.

---

## 2. Function Template

Example:

```cpp
template <typename T>
T add(T a, T b) {
    return a + b;
}
```

Usage:

```cpp
int x = add(1, 2);          // T = int
double y = add(1.5, 2.5);   // T = double
```

The compiler generates something conceptually like:

```cpp
int add<int>(int a, int b);
double add<double>(double a, double b);
```

---

## 3. Template Type Deduction

For function templates, the compiler can often deduce `T`.

```cpp
template <typename T>
T identity(T x) {
    return x;
}

identity(10);      // T = int
identity(3.14);    // T = double
```

But this can fail:

```cpp
add(1, 2.5); // error if template is T add(T, T)
```

Why?

The first argument suggests:

```text
T = int
```

The second suggests:

```text
T = double
```

Compiler cannot choose one automatically.

Fix:

```cpp
add<double>(1, 2.5);
```

or write a two-type template:

```cpp
template <typename T, typename U>
auto add(T a, U b) {
    return a + b;
}
```

---

## 4. Class Template

A class template defines a family of classes.

Example:

```cpp
template <typename T>
class Box {
private:
    T value;

public:
    explicit Box(const T& v)
        : value(v) {}

    const T& get() const {
        return value;
    }

    void set(const T& v) {
        value = v;
    }
};
```

Usage:

```cpp
Box<int> intBox(10);
Box<std::string> stringBox("hello");
```

The compiler generates different class types:

```text
Box<int>
Box<std::string>
```

Important:

```text
Box<int> and Box<double> are different types.
```

---

## 5. typename vs class in Template Parameters

These are usually equivalent:

```cpp
template <typename T>
```

and:

```cpp
template <class T>
```

Both mean `T` is a type parameter.

Modern C++ often uses `typename` because it is clearer.

Example:

```cpp
template <typename T>
T square(T x) {
    return x * x;
}
```

Same as:

```cpp
template <class T>
T square(T x) {
    return x * x;
}
```

---

## 6. Non-type Template Parameters

Templates can also take values as parameters.

Example:

```cpp
template <typename T, std::size_t N>
class Array {
private:
    T data[N];

public:
    std::size_t size() const {
        return N;
    }
};
```

Usage:

```cpp
Array<int, 10> arr;
Array<double, 5> arr2;
```

Here `N` is known at compile time.

This is how `std::array<T, N>` works.

---

## 7. Template Instantiation

Templates are not normal functions/classes by themselves.

They are blueprints.

When the compiler sees:

```cpp
Box<int> b(10);
```

it instantiates:

```text
Box<int>
```

When it sees:

```cpp
Box<std::string> s("hello");
```

it instantiates:

```text
Box<std::string>
```

This is why template errors often appear only when a specific type is used.

---

## 8. Template Requirements Are Implicit

Example:

```cpp
template <typename T>
T maxValue(T a, T b) {
    return a > b ? a : b;
}
```

This requires:

```text
T supports operator>
T is copyable/movable enough to return
```

If we call:

```cpp
maxValue(1, 2); // OK
```

But if:

```cpp
struct NoCompare {};
NoCompare a, b;
maxValue(a, b); // error
```

Then compilation fails because `NoCompare` does not support `operator>`.

Before C++20 concepts, these requirements are implicit.

---

## 9. Explicit Template Argument

Sometimes we specify the template type manually:

```cpp
template <typename T>
T convert(double x) {
    return static_cast<T>(x);
}
```

Usage:

```cpp
int x = convert<int>(3.14);
```

The compiler cannot deduce `T` from return type alone, so we provide it explicitly.

---

## 10. Function Template Overloading

Templates can be overloaded with normal functions.

Example:

```cpp
template <typename T>
void print(const T& x) {
    std::cout << "template\n";
}

void print(int x) {
    std::cout << "int overload\n";
}
```

Usage:

```cpp
print(10);     // int overload
print(3.14);   // template
```

Non-template exact match is often preferred over template.

---

## 11. Template Specialization

Specialization means customizing a template for a specific type.

Primary template:

```cpp
template <typename T>
class TypeName {
public:
    static std::string name() {
        return "unknown";
    }
};
```

Full specialization:

```cpp
template <>
class TypeName<int> {
public:
    static std::string name() {
        return "int";
    }
};
```

Usage:

```cpp
TypeName<double>::name(); // "unknown"
TypeName<int>::name();    // "int"
```

---

## 12. Function Template Specialization

Function templates can be specialized, but overloading is often preferred.

Example specialization:

```cpp
template <typename T>
void printType(const T&) {
    std::cout << "generic\n";
}

template <>
void printType<int>(const int&) {
    std::cout << "int\n";
}
```

But usually simpler:

```cpp
void printType(const int&) {
    std::cout << "int overload\n";
}
```

---

## 13. Partial Specialization

Class templates can be partially specialized.

Example:

```cpp
template <typename T>
class Traits {
public:
    static constexpr bool isPointer = false;
};

template <typename T>
class Traits<T*> {
public:
    static constexpr bool isPointer = true;
};
```

Usage:

```cpp
Traits<int>::isPointer;   // false
Traits<int*>::isPointer;  // true
```

Function templates cannot be partially specialized directly; use overloads instead.

---

## 14. Templates and Header Files

Template definitions usually need to be visible at the point of instantiation.

This means template code is commonly placed in header files.

Example:

```cpp
// box.h
template <typename T>
class Box {
public:
    T value;
};
```

If only the declaration is in the header and the definition is in a `.cpp`, the compiler may not know how to instantiate `Box<int>` in another translation unit.

Rule:

```text
Template definitions usually live in headers.
```

---

## 15. Dependent Names

A dependent name is a name that depends on a template parameter.

Example:

```cpp
template <typename T>
void f() {
    typename T::value_type x;
}
```

Here:

```text
T::value_type
```

depends on `T`.

The compiler does not know whether `T::value_type` is a type or a static member variable.

So we write:

```cpp
typename T::value_type
```

to tell the compiler it is a type.

This topic is deeper and will be revisited in advanced templates.

---

## 16. `typename` in Dependent Type

Example:

```cpp
template <typename Container>
void printFirst(const Container& c) {
    typename Container::const_iterator it = c.begin();
    std::cout << *it << std::endl;
}
```

Why `typename`?

Because:

```cpp
Container::const_iterator
```

depends on template parameter `Container`.

The compiler needs help knowing it is a type.

Without `typename`, this may fail to compile.

---

## 17. Type Alias in Templates

Use `using` to simplify dependent names:

```cpp
template <typename Container>
void printAll(const Container& c) {
    using Iter = typename Container::const_iterator;

    for (Iter it = c.begin(); it != c.end(); ++it) {
        std::cout << *it << " ";
    }
}
```

Modern C++ often uses `auto`:

```cpp
for (auto it = c.begin(); it != c.end(); ++it) {
    std::cout << *it << " ";
}
```

But understanding `typename` is still important for interviews and template code.

---

## 18. Template Code Bloat

Because templates generate code for each used type, too many instantiations can increase binary size.

Example:

```cpp
Box<int>
Box<double>
Box<std::string>
Box<User>
```

Each may generate separate code.

This is one tradeoff of templates.

---

## 19. Templates vs Virtual Functions

Templates provide compile-time polymorphism.

Virtual functions provide runtime polymorphism.

### Templates

```text
type known at compile time
can be inlined/optimized
no vtable overhead
may increase compile time/code size
```

### Virtual functions

```text
type chosen at runtime
runtime dispatch through vtable
more flexible for heterogeneous collections
```

Example:

```cpp
template <typename Strategy>
void run(const Strategy& s) {
    s.execute();
}
```

This is compile-time polymorphism.

```cpp
class Strategy {
public:
    virtual ~Strategy() = default;
    virtual void execute() = 0;
};
```

This is runtime polymorphism.

---

## 20. Common Interview Questions

### Q1. What is a template?

A template is a compile-time blueprint for generating functions or classes for specific types.

---

### Q2. What is template instantiation?

Template instantiation is the process where the compiler generates concrete code for a specific template argument, such as `Box<int>` or `Box<std::string>`.

---

### Q3. Difference between `typename` and `class` in template parameters?

For type template parameters, they are mostly equivalent.

`typename` is often preferred because it clearly means a type parameter.

---

### Q4. Why are template definitions usually in headers?

Because the compiler needs to see the full template definition at the point where it instantiates the template for a specific type.

---

### Q5. What is template specialization?

Specialization customizes a template for a specific type or pattern of types.

Full specialization handles one exact type.

Partial specialization handles a family of types, such as all pointer types.

---

### Q6. What is a dependent name?

A dependent name is a name that depends on a template parameter.

If it refers to a type, we often need to prefix it with `typename`.

---

## 21. Key Takeaways

- Templates generate type-specific code at compile time.
- Function templates can often deduce type arguments.
- Class templates require explicit template arguments before C++17 CTAD situations.
- `typename` and `class` are mostly equivalent for type parameters.
- Non-type template parameters are compile-time values.
- Templates are blueprints; concrete code is generated when instantiated.
- Template requirements are implicit before concepts.
- Template definitions usually live in headers.
- Class templates can be fully or partially specialized.
- Function templates can be overloaded; overloads are often preferred to specialization.
- Templates provide compile-time polymorphism.
