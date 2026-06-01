# 19. Advanced Templates Lite

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Variadic Templates](#2-variadic-templates)
- [3. Parameter Pack](#3-parameter-pack)
- [4. Pack Expansion](#4-pack-expansion)
- [5. Variadic Function Example](#5-variadic-function-example)
- [6. Fold Expressions](#6-fold-expressions)
- [7. Fold Expression with Initial Value](#7-fold-expression-with-initial-value)
- [8. Fold with Logical Operators](#8-fold-with-logical-operators)
- [9. Perfect Forwarding with Variadic Templates](#9-perfect-forwarding-with-variadic-templates)
- [10. Type Traits](#10-type-traits)
- [11. Type Trait Example](#11-type-trait-example)
- [12. if constexpr](#12-if-constexpr)
- [13. Why if constexpr Matters](#13-why-if-constexpr-matters)
- [14. enable_if](#14-enable_if)
- [15. enable_if_t](#15-enable_if_t)
- [16. SFINAE](#16-sfinae)
- [17. enable_if with Return Type](#17-enable_if-with-return-type)
- [18. enable_if vs if constexpr](#18-enable_if-vs-if-constexpr)
- [19. C++20 Concepts Preview](#19-c20-concepts-preview)
- [20. Why This Matters for Generic Containers](#20-why-this-matters-for-generic-containers)
- [21. Common Interview Questions](#21-common-interview-questions)
- [22. Key Takeaways](#22-key-takeaways)

## Related Code Trap

- [Advanced Templates Lite Demo](../code_traps/advanced_templates_lite.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

Templates can do more than just accept one type parameter.

Modern C++ templates can:

- accept any number of arguments
- inspect type properties at compile time
- enable or disable functions based on type
- forward arguments perfectly
- generate efficient type-specific code

Important tools:

```cpp
template <typename... Args>
fold expressions
std::is_integral_v<T>
std::enable_if_t<condition, Type>
```

---

## 2. Variadic Templates

A variadic template accepts any number of template parameters.

Example:

```cpp
template <typename... Args>
void printAll(Args... args) {
    // args is a parameter pack
}
```

Here:

```text
Args... = template parameter pack
args... = function parameter pack
```

Usage:

```cpp
printAll(1, 2.5, "hello");
```

The compiler deduces:

```text
Args = int, double, const char*
```

---

## 3. Parameter Pack

A parameter pack represents zero or more parameters.

```cpp
template <typename... Args>
void f(Args... args);
```

If called as:

```cpp
f(1, "hi", 3.14);
```

Then conceptually:

```text
Args... = int, const char*, double
args... = 1, "hi", 3.14
```

---

## 4. Pack Expansion

Use `...` to expand a pack.

Example:

```cpp
template <typename... Args>
void callTarget(Args&&... args) {
    target(std::forward<Args>(args)...);
}
```

This expands into:

```cpp
target(std::forward<Arg1>(arg1),
       std::forward<Arg2>(arg2),
       std::forward<Arg3>(arg3));
```

This is the foundation of perfect forwarding for multiple arguments.

---

## 5. Variadic Function Example

Before C++17 fold expressions, printing all arguments could be done recursively.

Modern C++17 version:

```cpp
template <typename... Args>
void printAll(const Args&... args) {
    ((std::cout << args << " "), ...);
    std::cout << std::endl;
}
```

Usage:

```cpp
printAll(1, "hello", 3.14);
```

Output:

```text
1 hello 3.14
```

---

## 6. Fold Expressions

A fold expression expands a parameter pack with an operator.

Example:

```cpp
template <typename... Args>
auto sum(Args... args) {
    return (args + ...);
}
```

Usage:

```cpp
sum(1, 2, 3); // 6
```

Conceptually:

```cpp
1 + (2 + 3)
```

Another form:

```cpp
(args + ...)
```

or:

```cpp
(... + args)
```

For many operations, both are fine, but order can matter for non-associative operations.

---

## 7. Fold Expression with Initial Value

This handles empty pack safely:

```cpp
template <typename... Args>
auto sumSafe(Args... args) {
    return (0 + ... + args);
}
```

Now:

```cpp
sumSafe(); // 0
```

Without initial value:

```cpp
(args + ...)
```

empty pack may be invalid.

---

## 8. Fold with Logical Operators

Check if all conditions are true:

```cpp
template <typename... Args>
bool allTrue(Args... args) {
    return (args && ...);
}
```

Check if any condition is true:

```cpp
template <typename... Args>
bool anyTrue(Args... args) {
    return (args || ...);
}
```

---

## 9. Perfect Forwarding with Variadic Templates

This is how many factory functions work.

```cpp
template <typename T, typename... Args>
T makeObject(Args&&... args) {
    return T(std::forward<Args>(args)...);
}
```

Usage:

```cpp
auto user = makeObject<User>("Yuqi", 25);
```

Conceptually constructs:

```cpp
User("Yuqi", 25)
```

The function preserves whether each argument is an lvalue or rvalue.

---

## 10. Type Traits

Type traits inspect type properties at compile time.

Header:

```cpp
#include <type_traits>
```

Examples:

```cpp
std::is_integral_v<int>        // true
std::is_integral_v<double>     // false
std::is_pointer_v<int*>        // true
std::is_same_v<int, int>       // true
std::is_same_v<int, double>    // false
std::is_move_constructible_v<T>
std::is_nothrow_move_constructible_v<T>
```

They are compile-time constants.

---

## 11. Type Trait Example

```cpp
template <typename T>
void inspectType() {
    if constexpr (std::is_integral_v<T>) {
        std::cout << "integral type\n";
    } else {
        std::cout << "non-integral type\n";
    }
}
```

Usage:

```cpp
inspectType<int>();     // integral
inspectType<double>();  // non-integral
```

---

## 12. if constexpr

`if constexpr` is a compile-time branch.

Example:

```cpp
template <typename T>
void printTypeCategory(const T& value) {
    if constexpr (std::is_integral_v<T>) {
        std::cout << "integral\n";
    } else {
        std::cout << "non-integral\n";
    }
}
```

Only the selected branch is compiled for each `T`.

This is different from normal `if`.

---

## 13. Why if constexpr Matters

Without `if constexpr`, both branches must be valid C++.

With `if constexpr`, the unused branch can contain code that would be invalid for that type.

Example:

```cpp
template <typename T>
void f(T value) {
    if constexpr (std::is_pointer_v<T>) {
        std::cout << *value << std::endl;
    } else {
        std::cout << value << std::endl;
    }
}
```

For `T = int`, the pointer branch is discarded, so `*value` is not compiled.

---

## 14. enable_if

`std::enable_if` can enable or disable a template based on a compile-time condition.

Header:

```cpp
#include <type_traits>
```

Common form:

```cpp
template <typename T,
          typename = std::enable_if_t<std::is_integral_v<T>>>
void f(T x) {
    std::cout << "integral only\n";
}
```

This function only participates in overload resolution if `T` is integral.

---

## 15. enable_if_t

`std::enable_if_t<condition, Type>` means:

```text
If condition is true:
    becomes Type

If condition is false:
    substitution fails
```

Default `Type` is `void`.

So:

```cpp
std::enable_if_t<std::is_integral_v<T>>
```

means:

```text
void if T is integral
invalid if T is not integral
```

---

## 16. SFINAE

SFINAE stands for:

```text
Substitution Failure Is Not An Error
```

Meaning:

If template substitution fails during overload resolution, the compiler removes that overload from consideration instead of immediately producing a hard error.

Example:

```cpp
template <typename T,
          typename = std::enable_if_t<std::is_integral_v<T>>>
void onlyIntegral(T x);
```

For:

```cpp
onlyIntegral(10);
```

Works.

For:

```cpp
onlyIntegral(3.14);
```

This overload is removed because `T = double` is not integral.

If no other overload matches, then compilation fails.

---

## 17. enable_if with Return Type

Another style:

```cpp
template <typename T>
std::enable_if_t<std::is_integral_v<T>, void>
onlyIntegral(T x) {
    std::cout << "integral\n";
}
```

This means return type is `void` only if `T` is integral.

If not, this function is removed from overload resolution.

---

## 18. enable_if vs if constexpr

Use `if constexpr` when one function can handle all types with compile-time branches.

Use `enable_if` when you want to control whether a function exists for a type.

Example:

```cpp
template <typename T>
void print(T x) {
    if constexpr (std::is_pointer_v<T>) {
        std::cout << *x << std::endl;
    } else {
        std::cout << x << std::endl;
    }
}
```

One function handles both.

But:

```cpp
template <typename T,
          typename = std::enable_if_t<std::is_integral_v<T>>>
void onlyIntegral(T x);
```

This function exists only for integral types.

---

## 19. C++20 Concepts Preview

In C++20, concepts make constraints cleaner.

Instead of:

```cpp
template <typename T,
          typename = std::enable_if_t<std::is_integral_v<T>>>
void f(T x);
```

We can write:

```cpp
template <std::integral T>
void f(T x);
```

Or:

```cpp
template <typename T>
requires std::integral<T>
void f(T x);
```

But for many interviews and older codebases, understanding `enable_if` is still useful.

---

## 20. Why This Matters for Generic Containers

A generic container may need to ask:

```text
Is T move constructible?
Is T noexcept move constructible?
Is T copy constructible?
Is T destructible?
```

Example:

```cpp
if constexpr (std::is_nothrow_move_constructible_v<T> ||
              !std::is_copy_constructible_v<T>) {
    // move elements
} else {
    // copy elements
}
```

This is the logic behind ideas like:

```cpp
std::move_if_noexcept
```

---

## 21. Common Interview Questions

### Q1. What is a variadic template?

A variadic template is a template that can accept a variable number of template arguments.

Example:

```cpp
template <typename... Args>
void f(Args... args);
```

---

### Q2. What is a fold expression?

A fold expression expands a parameter pack using an operator.

Example:

```cpp
(args + ...)
```

for summing all arguments.

---

### Q3. What are type traits?

Type traits are compile-time utilities that inspect or transform types.

Examples include:

```cpp
std::is_integral_v<T>
std::is_pointer_v<T>
std::is_same_v<T, U>
```

---

### Q4. What does `if constexpr` do?

`if constexpr` selects a branch at compile time.

The non-selected branch is discarded and does not need to be valid for that specific type.

---

### Q5. What is SFINAE?

SFINAE means Substitution Failure Is Not An Error.

If template substitution fails during overload resolution, that overload is removed instead of causing an immediate compile error.

---

### Q6. What is `enable_if` used for?

`enable_if` enables or disables template overloads based on compile-time conditions.

It is often used to constrain templates before C++20 concepts.

---

## 22. Key Takeaways

- `typename... Args` means a template parameter pack.
- `Args&&... args` is common for perfect forwarding.
- `std::forward<Args>(args)...` expands forwarding for every argument.
- Fold expressions reduce parameter packs with operators.
- Type traits inspect types at compile time.
- `if constexpr` enables compile-time branching.
- `enable_if` controls whether a template participates in overload resolution.
- SFINAE removes invalid template overloads instead of hard failing.
- These tools are foundations for generic libraries and containers.
