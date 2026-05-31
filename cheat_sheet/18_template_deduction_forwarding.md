# 18. Template Type Deduction and Forwarding Reference

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Normal Rvalue Reference](#2-normal-rvalue-reference)
- [3. Forwarding Reference](#3-forwarding-reference)
- [4. Deduction with Lvalue](#4-deduction-with-lvalue)
- [5. Deduction with Rvalue](#5-deduction-with-rvalue)
- [6. Reference Collapsing Rules](#6-reference-collapsing-rules)
- [7. Named Rvalue Reference Is an Lvalue](#7-named-rvalue-reference-is-an-lvalue)
- [8. std::move vs std::forward](#8-stdmove-vs-stdforward)
- [9. Why std::forward Is Needed](#9-why-stdforward-is-needed)
- [10. Perfect Forwarding](#10-perfect-forwarding)
- [11. emplace_back and Perfect Forwarding](#11-emplace_back-and-perfect-forwarding)
- [12. Forwarding Reference vs Rvalue Reference](#12-forwarding-reference-vs-rvalue-reference)
- [13. How to Make Member Function Forwarding](#13-how-to-make-member-function-forwarding)
- [14. auto&&](#14-auto)
- [15. const T&& Is Not Forwarding Reference](#15-const-t-is-not-forwarding-reference)
- [16. Common Mistake: Using std::move Instead of std::forward](#16-common-mistake-using-stdmove-instead-of-stdforward)
- [17. Common Mistake: Forgetting std::forward](#17-common-mistake-forgetting-stdforward)
- [18. Common Interview Questions](#18-common-interview-questions)
- [19. Key Takeaways](#19-key-takeaways)

## Related Code Trap

- [Template Deduction / Forwarding Demo](../code_traps/template_deduction_forwarding.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

In normal C++, `T&&` means rvalue reference.

Example:

```cpp
void f(std::string&& s);
```

This accepts rvalues:

```cpp
f(std::string("hello")); // OK
```

But not lvalues:

```cpp
std::string name = "Yuqi";
f(name); // error
```

However, in a function template:

```cpp
template <typename T>
void f(T&& x);
```

`T&&` has special deduction rules.

It can bind to both lvalues and rvalues.

This is called a forwarding reference.

---

## 2. Normal Rvalue Reference

```cpp
void consume(std::string&& s) {
    std::cout << s << std::endl;
}
```

Valid:

```cpp
consume(std::string("temporary"));
consume("hello");
```

Invalid:

```cpp
std::string name = "Yuqi";
consume(name); // error
```

Why?

`name` is an lvalue.

A non-const rvalue reference cannot bind to an lvalue.

---

## 3. Forwarding Reference

```cpp
template <typename T>
void wrapper(T&& x) {
    // ...
}
```

Here `T&&` is a forwarding reference when:

```text
T is a deduced template parameter
and the parameter form is exactly T&&
```

This can accept both:

```cpp
std::string name = "Yuqi";

wrapper(name);              // lvalue
wrapper(std::string("hi")); // rvalue
```

---

## 4. Deduction with Lvalue

```cpp
template <typename T>
void f(T&& x);
```

Call:

```cpp
std::string s = "hello";
f(s);
```

Since `s` is an lvalue, template deduction gives:

```text
T = std::string&
```

So parameter type becomes:

```cpp
T&& = std::string& &&
```

Reference collapsing:

```text
std::string& && collapses to std::string&
```

So inside function:

```cpp
x is std::string&
```

---

## 5. Deduction with Rvalue

Call:

```cpp
f(std::string("hello"));
```

Since argument is rvalue, template deduction gives:

```text
T = std::string
```

So parameter type becomes:

```cpp
T&& = std::string&&
```

Inside function:

```cpp
x is std::string&&
```

But there is a trap:

```text
x itself is a named variable, so expression x is an lvalue.
```

This is why we need `std::forward`.

---

## 6. Reference Collapsing Rules

C++ reference collapsing rules:

```text
&  + &  -> &
&  + && -> &
&& + &  -> &
&& + && -> &&
```

Simple memory:

```text
If any side is &, result is &.
Only && + && gives &&.
```

Examples:

```cpp
using A = int&;
using B = A&&; // int&

using C = int&&;
using D = C&;  // int&
using E = C&&; // int&&
```

This is what allows forwarding references to bind to both lvalues and rvalues.

---

## 7. Named Rvalue Reference Is an Lvalue

This is extremely important.

```cpp
void f(std::string&& s) {
    g(s);
}
```

Inside `f`, `s` has type `std::string&&`, but the expression `s` is an lvalue because it has a name.

So:

```cpp
g(s);
```

passes an lvalue.

To pass it as an rvalue:

```cpp
g(std::move(s));
```

In template forwarding:

```cpp
g(std::forward<T>(x));
```

---

## 8. std::move vs std::forward

### std::move

```cpp
std::move(x)
```

Always casts `x` to an rvalue.

Use it when you want to unconditionally allow moving from `x`.

Example:

```cpp
std::string a = "hello";
std::string b = std::move(a);
```

---

### std::forward

```cpp
std::forward<T>(x)
```

Conditionally casts based on `T`.

If original argument was lvalue:

```text
forward as lvalue
```

If original argument was rvalue:

```text
forward as rvalue
```

Use it in forwarding references.

---

## 9. Why std::forward Is Needed

Suppose we have overloads:

```cpp
void process(const std::string& s) {
    std::cout << "lvalue overload\n";
}

void process(std::string&& s) {
    std::cout << "rvalue overload\n";
}
```

Bad wrapper:

```cpp
template <typename T>
void wrapper(T&& x) {
    process(x);
}
```

Even if caller passes rvalue:

```cpp
wrapper(std::string("hi"));
```

Inside wrapper, `x` is named, so `process(x)` calls lvalue overload.

Correct wrapper:

```cpp
template <typename T>
void wrapper(T&& x) {
    process(std::forward<T>(x));
}
```

Now:

```cpp
std::string s = "hi";
wrapper(s);                 // lvalue overload
wrapper(std::string("hi")); // rvalue overload
```

---

## 10. Perfect Forwarding

Perfect forwarding means:

```text
preserve the original value category and constness of arguments
```

Example:

```cpp
template <typename T>
void wrapper(T&& x) {
    target(std::forward<T>(x));
}
```

For multiple arguments:

```cpp
template <typename... Args>
void wrapper(Args&&... args) {
    target(std::forward<Args>(args)...);
}
```

This is used heavily in:

- `emplace_back`
- `make_unique`
- `make_shared`
- factory functions
- generic wrappers

---

## 11. emplace_back and Perfect Forwarding

`vector::emplace_back` conceptually does:

```cpp
template <typename... Args>
T& emplace_back(Args&&... args) {
    // allocate/grow if needed
    construct T in-place using forwarded args
    return reference_to_new_element;
}
```

Then:

```cpp
std::vector<std::string> v;
v.emplace_back("hello");
```

This forwards `"hello"` into the `std::string` constructor.

For custom type:

```cpp
struct User {
    User(std::string name, int age);
};

std::vector<User> users;
users.emplace_back("Yuqi", 25);
```

`emplace_back` constructs `User("Yuqi", 25)` directly in vector storage.

---

## 12. Forwarding Reference vs Rvalue Reference

This is a common trap.

### Forwarding reference

```cpp
template <typename T>
void f(T&& x);
```

`T` is deduced.

So `T&&` is forwarding reference.

---

### Rvalue reference

```cpp
template <typename T>
class Box {
public:
    void set(T&& x);
};
```

Here `T` is the class template parameter, not deduced by `set`.

If `Box<std::string>` is created, then:

```cpp
void set(std::string&& x);
```

This is a normal rvalue reference, not forwarding reference.

---

## 13. How to Make Member Function Forwarding

Inside class template:

```cpp
template <typename T>
class Box {
public:
    template <typename U>
    void set(U&& x) {
        value = std::forward<U>(x);
    }

private:
    T value;
};
```

Here `U` is deduced by the member function call.

So `U&&` is a forwarding reference.

---

## 14. auto&&

`auto&&` can also be a forwarding reference in many contexts.

Example:

```cpp
auto&& x = expr;
```

It can bind to lvalues and rvalues, preserving value category.

Common in range-based loops:

```cpp
for (auto&& item : container) {
    // generic reference to item
}
```

This is useful in generic code.

---

## 15. const T&& Is Not Forwarding Reference

```cpp
template <typename T>
void f(const T&& x);
```

This is not a forwarding reference.

Because the form is not exactly:

```cpp
T&&
```

It is:

```cpp
const T&&
```

This only binds to const rvalues and is rarely useful.

---

## 16. Common Mistake: Using std::move Instead of std::forward

Bad:

```cpp
template <typename T>
void wrapper(T&& x) {
    process(std::move(x));
}
```

This always forwards as rvalue.

Problem:

```cpp
std::string s = "hello";
wrapper(s);
```

This moves from `s`, even though caller passed an lvalue.

Usually not what we want.

Correct:

```cpp
template <typename T>
void wrapper(T&& x) {
    process(std::forward<T>(x));
}
```

This preserves caller intent.

---

## 17. Common Mistake: Forgetting std::forward

Bad:

```cpp
template <typename T>
void wrapper(T&& x) {
    process(x);
}
```

This always passes `x` as lvalue because `x` is named.

Correct:

```cpp
template <typename T>
void wrapper(T&& x) {
    process(std::forward<T>(x));
}
```

---

## 18. Common Interview Questions

### Q1. What is a forwarding reference?

A forwarding reference is a parameter of the form `T&&` where `T` is a deduced template parameter.

It can bind to both lvalues and rvalues and preserve the original value category.

---

### Q2. Why is `T&&` sometimes not an rvalue reference?

In a function template with deduced `T`, `T&&` becomes a forwarding reference.

If the argument is an lvalue, `T` deduces to `T&`, and reference collapsing makes the parameter an lvalue reference.

---

### Q3. What is reference collapsing?

Reference collapsing rules determine the final reference type when references combine.

If either reference is `&`, the result is `&`.

Only `&& &&` becomes `&&`.

---

### Q4. Why do we need std::forward?

Because a named parameter is always an lvalue expression, even if its type is an rvalue reference.

`std::forward<T>` preserves whether the original argument was an lvalue or rvalue.

---

### Q5. Difference between std::move and std::forward?

`std::move` always casts to rvalue.

`std::forward<T>` conditionally casts based on template type `T`, preserving the original value category.

Use `std::move` when unconditionally moving.

Use `std::forward` in forwarding-reference templates.

---

## 19. Key Takeaways

- `T&&` in a deduced function template is a forwarding reference.
- Forwarding references can bind to both lvalues and rvalues.
- Lvalue argument deduces `T` as `U&`.
- Rvalue argument deduces `T` as `U`.
- Reference collapsing makes this work.
- Named rvalue reference variables are lvalue expressions.
- `std::forward<T>(x)` preserves lvalue/rvalue.
- `std::move(x)` always casts to rvalue.
- `const T&&` is not a forwarding reference.
- In class templates, `T&&` member parameters are not forwarding unless `T` is deduced by that function.
