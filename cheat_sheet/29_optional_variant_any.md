# 29. optional, variant, and any

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. std::optional](#2-stdoptional)
- [3. optional Basic Usage](#3-optional-basic-usage)
- [4. optional Dereference](#4-optional-dereference)
- [5. optional::value()](#5-optionalvalue)
- [6. value_or](#6-value_or)
- [7. optional and Object Lifetime](#7-optional-and-object-lifetime)
- [8. emplace](#8-emplace)
- [9. optional as Return Type](#9-optional-as-return-type)
- [10. optional<T&>?](#10-optionalt)
- [11. optional vs pointer](#11-optional-vs-pointer)
- [12. std::variant](#12-stdvariant)
- [13. variant Access](#13-variant-access)
- [14. holds_alternative](#14-holds_alternative)
- [15. visit](#15-visit)
- [16. variant Example: Result Type](#16-variant-example-result-type)
- [17. variant vs optional](#17-variant-vs-optional)
- [18. variant vs union](#18-variant-vs-union)
- [19. monostate](#19-monostate)
- [20. std::any](#20-stdany)
- [21. any Type Erasure](#21-any-type-erasure)
- [22. any vs variant](#22-any-vs-variant)
- [23. any vs void*](#23-any-vs-void)
- [24. Common Parameter/Return Guidelines](#24-common-parameterreturn-guidelines)
- [25. Common Interview Questions](#25-common-interview-questions)
- [26. Key Takeaways](#26-key-takeaways)

## Related Code Trap

- [optional / variant / any Demo](../code_traps/optional_variant_any.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

Modern C++ provides type-safe ways to represent flexible values.

```cpp
std::optional<T>
std::variant<Ts...>
std::any
```

Meanings:

```text
optional<T>      zero or one T
variant<A,B,C>   exactly one active alternative among known types
any              one value of almost any copyable type
```

Headers:

```cpp
#include <optional>
#include <variant>
#include <any>
```

---

## 2. std::optional

`std::optional<T>` represents a value that may or may not exist.

Example:

```cpp
std::optional<int> findIndex();
```

This is better than returning:

```cpp
-1
```

when `-1` is just a sentinel value.

---

## 3. optional Basic Usage

```cpp
std::optional<int> x;

if (!x) {
    std::cout << "no value\n";
}

x = 42;

if (x.has_value()) {
    std::cout << *x << std::endl;
}
```

Ways to access:

```cpp
*x
x.value()
x.value_or(defaultValue)
```

---

## 4. optional Dereference

```cpp
std::optional<int> x = 42;

std::cout << *x << std::endl;
```

`*x` gives access to the contained value.

But if optional is empty:

```cpp
std::optional<int> x;
*x; // wrong, undefined behavior
```

So check first:

```cpp
if (x) {
    std::cout << *x << std::endl;
}
```

---

## 5. optional::value()

```cpp
std::optional<int> x;

x.value();
```

If empty, throws:

```cpp
std::bad_optional_access
```

So:

```cpp
*x
```

is unchecked.

```cpp
.value()
```

is checked and throws if no value.

---

## 6. value_or

```cpp
std::optional<int> x;

int y = x.value_or(100);
```

If `x` has value:

```text
y = contained value
```

If empty:

```text
y = 100
```

Example:

```cpp
std::optional<std::string> nickname;
std::string name = nickname.value_or("unknown");
```

---

## 7. optional and Object Lifetime

`optional<T>` contains storage for a `T`, but the `T` object may or may not be alive.

```cpp
std::optional<T> opt;
```

No `T` object is constructed.

```cpp
opt.emplace(args...);
```

Constructs `T` inside optional.

```cpp
opt.reset();
```

Destroys contained `T` if present.

So optional is like:

```text
raw storage + engaged flag
```

This connects to placement-new/object-lifetime ideas.

---

## 8. emplace

```cpp
std::optional<std::string> opt;

opt.emplace("hello");
```

Constructs the `std::string` directly inside optional.

If optional already had a value, old value is destroyed first.

---

## 9. optional as Return Type

Example:

```cpp
std::optional<int> parseInt(std::string_view s);
```

Usage:

```cpp
auto result = parseInt("123");

if (result) {
    std::cout << *result << std::endl;
} else {
    std::cout << "parse failed\n";
}
```

This makes failure explicit without exceptions.

---

## 10. optional<T&>?

`std::optional<T&>` is not supported by standard `optional`.

If you need optional reference-like behavior:

```cpp
T* ptr; // nullable non-owning pointer
```

or:

```cpp
std::optional<std::reference_wrapper<T>>
```

But often a raw pointer is simpler for optional reference:

```cpp
User* findUser();
```

Meaning:

```text
nullptr means not found
non-null means found
```

---

## 11. optional vs pointer

Use `optional<T>` when:

```text
the object is small/moderate and you want value semantics
```

Use pointer when:

```text
you want non-owning optional reference
or polymorphism
or avoiding object copy/move
```

Example:

```cpp
std::optional<User> maybeCreateUser();
```

means it may return an owned `User` value.

```cpp
User* findExistingUser();
```

means it may return a non-owning pointer to existing user.

---

## 12. std::variant

`std::variant<A, B, C>` stores exactly one active alternative.

Example:

```cpp
std::variant<int, std::string> v;

v = 42;
v = std::string("hello");
```

At any time, it contains either:

```text
int
or std::string
```

This is type-safe union.

---

## 13. variant Access

```cpp
std::variant<int, std::string> v = 42;

std::get<int>(v);      // OK
std::get<0>(v);        // OK, index 0 is int
std::get<std::string>(v); // throws bad_variant_access if active type is not string
```

Checked pointer-style access:

```cpp
if (auto p = std::get_if<int>(&v)) {
    std::cout << *p << std::endl;
}
```

`get_if` returns pointer or `nullptr`.

---

## 14. holds_alternative

```cpp
if (std::holds_alternative<int>(v)) {
    std::cout << std::get<int>(v);
}
```

This checks current active type.

---

## 15. visit

`std::visit` is the idiomatic way to handle variant.

```cpp
std::variant<int, std::string> v = "hello";

std::visit([](const auto& value) {
    std::cout << value << std::endl;
}, v);
```

The lambda is called with the active value.

---

## 16. variant Example: Result Type

```cpp
struct ParseError {
    std::string message;
};

using ParseResult = std::variant<int, ParseError>;
```

Usage:

```cpp
ParseResult r = parse("123");

if (auto p = std::get_if<int>(&r)) {
    std::cout << "value = " << *p << std::endl;
} else if (auto e = std::get_if<ParseError>(&r)) {
    std::cout << "error = " << e->message << std::endl;
}
```

This is useful when failure needs details.

---

## 17. variant vs optional

```cpp
std::optional<T>
```

means:

```text
T or nothing
```

```cpp
std::variant<T, Error>
```

means:

```text
T or Error
```

Use optional when absence is enough.

Use variant when you need different meaningful alternatives.

---

## 18. variant vs union

C union:

```cpp
union U {
    int i;
    double d;
};
```

Problems:

```text
you must manually track active member
non-trivial types are harder
less type-safe
```

`std::variant`:

```text
tracks active alternative
works with non-trivial types
throws/uses get_if on wrong access
type-safe
```

---

## 19. monostate

A variant must always hold something.

If you want an "empty" alternative:

```cpp
std::variant<std::monostate, int, std::string> v;
```

`std::monostate` is a dummy empty type.

This is like:

```text
nothing or int or string
```

---

## 20. std::any

`std::any` can hold a value of almost any copyable type.

Example:

```cpp
std::any a;

a = 42;
a = std::string("hello");
```

Access:

```cpp
std::any_cast<int>(a);
```

If wrong type:

```cpp
std::bad_any_cast
```

Pointer form:

```cpp
if (auto p = std::any_cast<std::string>(&a)) {
    std::cout << *p << std::endl;
}
```

---

## 21. any Type Erasure

`std::any` uses type erasure.

The code using `any` does not know the concrete type at compile time.

But the actual value still has a runtime type stored inside.

This is flexible but less type-safe and more runtime-oriented than `variant`.

---

## 22. any vs variant

### variant

```text
allowed types are known at compile time
type-safe alternatives
compiler can check exhaustiveness patterns more easily
better for closed set of types
```

### any

```text
can hold many possible types
type checked at runtime through any_cast
less explicit
more flexible but easier to misuse
```

Use `variant` when possible.

Use `any` only when the type set is truly open or plugin-like.

---

## 23. any vs void*

Both can store arbitrary-ish values, but:

```cpp
std::any
```

is safer because it remembers the actual type.

```cpp
void*
```

does not remember type.

With `void*`, caller must manually cast correctly.

Wrong cast is undefined behavior.

With `std::any`, wrong `any_cast` throws or returns nullptr.

---

## 24. Common Parameter/Return Guidelines

Return optional:

```cpp
std::optional<User> findUserValue();
```

when result may be absent and returning a value makes sense.

Return pointer:

```cpp
User* findUser();
```

when returning non-owning reference to existing object.

Return variant:

```cpp
std::variant<User, Error> loadUser();
```

when you need success or detailed error.

Use any:

```cpp
std::any metadata;
```

when you truly need open-ended type-erased storage.

---

## 25. Common Interview Questions

### Q1. What is std::optional?

`std::optional<T>` represents an optional value: either it contains a `T` or it contains no value.

---

### Q2. Difference between optional and pointer?

`optional<T>` owns a value if present.

A pointer usually refers to an object elsewhere and can be null.

---

### Q3. What is std::variant?

`std::variant` is a type-safe union that holds exactly one value from a fixed set of alternative types.

---

### Q4. How do you safely access variant?

Use `std::get_if<T>(&v)` or `std::visit`.

`std::get<T>(v)` throws if the active alternative is not `T`.

---

### Q5. Difference between variant and any?

`variant` holds one of a fixed set of known types.

`any` can hold many possible copyable types and requires runtime `any_cast`.

Prefer `variant` when the possible types are known.

---

### Q6. Why is any safer than void*?

`any` stores runtime type information and checks casts.

`void*` stores only an address and does not know the actual type.

---

## 26. Key Takeaways

- `optional<T>` means value or no value.
- `optional` owns the contained value.
- Check optional before dereferencing.
- `variant` means one active alternative from known types.
- Use `get_if` or `visit` for variant.
- `monostate` can represent empty variant state.
- `any` is type-erased runtime storage.
- Prefer optional/variant over sentinel values and unsafe unions.
- Prefer variant over any when possible.
