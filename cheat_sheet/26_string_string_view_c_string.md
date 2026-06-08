# 26. std::string, std::string_view, and C string

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. std::string](#2-stdstring)
- [3. string Copy](#3-string-copy)
- [4. string Move](#4-string-move)
- [5. Passing string to Function](#5-passing-string-to-function)
- [6. C String: const char*](#6-c-string-const-char)
- [7. C String Does Not Own Data](#7-c-string-does-not-own-data)
- [8. string::c_str()](#8-stringc_str)
- [9. c_str Pointer Invalidation](#9-c_str-pointer-invalidation)
- [10. std::string_view](#10-stdstring_view)
- [11. string_view Is Cheap](#11-string_view-is-cheap)
- [12. string_view Does Not Own](#12-string_view-does-not-own)
- [13. Dangerous Temporary string_view](#13-dangerous-temporary-string_view)
- [14. string_view May Not Be Null-Terminated](#14-string_view-may-not-be-null-terminated)
- [15. string_view substr](#15-string_view-substr)
- [16. string vs string_view](#16-string-vs-string_view)
- [17. Function Parameter Guidelines](#17-function-parameter-guidelines)
- [18. Small String Optimization](#18-small-string-optimization)
- [19. Common Interview Questions](#19-common-interview-questions)
- [20. Key Takeaways](#20-key-takeaways)

## Related Code Trap

- [string / string_view / C string Demo](../code_traps/string_string_view_c_string.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

C++ commonly has three string-like things:

```cpp
std::string
std::string_view
const char*
```

They have very different ownership/lifetime meanings.

```text
std::string      owns characters
std::string_view does not own characters
const char*      points to null-terminated character data
```

---

## 2. std::string

`std::string` owns its character buffer.

Example:

```cpp
std::string s = "hello";
```

`s` manages the memory for characters.

When `s` is destroyed, its character storage is released automatically.

Important properties:

- owns data
- mutable
- size is stored
- may allocate heap memory
- safe to return by value
- works well with RAII

---

## 3. string Copy

```cpp
std::string a = "hello";
std::string b = a;
```

This makes a copy of the string content.

Now:

```text
a and b are separate strings
```

Modifying `b` does not modify `a`.

```cpp
b[0] = 'H';
```

Now:

```text
a = "hello"
b = "Hello"
```

---

## 4. string Move

```cpp
std::string a = "hello";
std::string b = std::move(a);
```

This moves resources from `a` to `b` if possible.

After move:

```text
b contains the original content
a is valid but unspecified
```

Do not assume `a` is empty.

You can still assign to `a` later:

```cpp
a = "new value";
```

---

## 5. Passing string to Function

### Read-only, no ownership needed

Prefer:

```cpp
void print(const std::string& s);
```

This avoids copying.

### Need to store a copy

Either:

```cpp
void setName(const std::string& s) {
    name = s;
}
```

or pass by value and move:

```cpp
void setName(std::string s) {
    name = std::move(s);
}
```

The by-value pattern is useful when caller may pass either lvalue or rvalue.

---

## 6. C String: const char*

C string is a pointer to null-terminated character data.

Example:

```cpp
const char* p = "hello";
```

Memory layout:

```text
'h' 'e' 'l' 'l' 'o' '\0'
```

The `'\0'` marks the end.

C string functions rely on this terminator:

```cpp
std::strlen(p);
```

---

## 7. C String Does Not Own Data

```cpp
const char* p = "hello";
```

`p` points to a string literal.

`p` itself does not own the characters.

Do not modify string literal:

```cpp
char* p = "hello"; // bad in modern C++
p[0] = 'H';        // undefined behavior
```

Use:

```cpp
const char* p = "hello";
```

or:

```cpp
std::string s = "hello";
```

---

## 8. string::c_str()

```cpp
std::string s = "hello";
const char* p = s.c_str();
```

`p` points to null-terminated data owned by `s`.

Important:

```text
p is valid only while s is alive and not modified in a way that invalidates the buffer
```

Example dangerous:

```cpp
const char* bad() {
    std::string s = "hello";
    return s.c_str(); // dangling
}
```

`s` is destroyed when function returns, so returned pointer is invalid.

---

## 9. c_str Pointer Invalidation

```cpp
std::string s = "hello";
const char* p = s.c_str();

s += " world";

// p may be invalid now
```

The string may reallocate its internal buffer.

So old `p` may dangle.

Rule:

```text
Do not store c_str() pointer long-term unless string lifetime and mutation are controlled.
```

---

## 10. std::string_view

`std::string_view` is a non-owning view of characters.

Header:

```cpp
#include <string_view>
```

Example:

```cpp
std::string_view sv = "hello";
```

A `string_view` usually stores:

```text
pointer + length
```

It does not own characters.

It does not require null termination.

---

## 11. string_view Is Cheap

Copying `string_view` is cheap:

```cpp
std::string_view a = "hello";
std::string_view b = a;
```

This copies only:

```text
pointer + length
```

It does not copy the characters.

Good for read-only function parameters:

```cpp
void print(std::string_view sv);
```

This can accept:

```cpp
print("hello");
print(std::string("hello"));
print(existingString);
```

But lifetime matters.

---

## 12. string_view Does Not Own

This is dangerous:

```cpp
std::string_view bad() {
    std::string s = "hello";
    return std::string_view(s); // dangling
}
```

Because `s` is destroyed when function returns.

The returned `string_view` points to destroyed memory.

---

## 13. Dangerous Temporary string_view

Very common bug:

```cpp
std::string_view sv = std::string("hello");
```

This compiles, but is dangerous.

The temporary `std::string` is destroyed at the end of the full expression.

Then `sv` dangles.

Safer:

```cpp
std::string s = "hello";
std::string_view sv = s;
```

Now `sv` is valid as long as `s` is alive and not modified in a dangerous way.

---

## 14. string_view May Not Be Null-Terminated

`string_view` stores length.

It may view a substring:

```cpp
std::string s = "hello world";
std::string_view sv(s.data(), 5); // "hello"
```

This view is not necessarily null-terminated after `"hello"`.

So this is dangerous:

```cpp
printf("%s", sv.data()); // wrong if not null-terminated
```

Use:

```cpp
std::cout << sv;
```

or:

```cpp
printf("%.*s", static_cast<int>(sv.size()), sv.data());
```

---

## 15. string_view substr

```cpp
std::string_view sv = "hello world";
auto sub = sv.substr(0, 5);
```

`sub` is another view.

It does not copy characters.

This is cheap.

But both views depend on the original character data being alive.

---

## 16. string vs string_view

### std::string

```text
owns data
safe to store
can modify
can guarantee null-terminated c_str()
```

### std::string_view

```text
does not own data
cheap to pass
read-only view
dangerous to store if source lifetime is unclear
may not be null-terminated
```

Use `string_view` mostly for function parameters where the function does not store it.

---

## 17. Function Parameter Guidelines

### Function only reads string and does not store

Good:

```cpp
void log(std::string_view msg);
```

This accepts string literals, strings, and substrings cheaply.

---

### Function stores the string

Use `std::string`:

```cpp
class User {
private:
    std::string name;

public:
    void setName(std::string n) {
        name = std::move(n);
    }
};
```

Do not store `string_view` unless you can guarantee external lifetime.

---

### Function needs C API null-terminated string

Use:

```cpp
void openFile(const std::string& path) {
    c_api_open(path.c_str());
}
```

or:

```cpp
void openFile(const char* path);
```

Be careful with `string_view` because it may not be null-terminated.

---

## 18. Small String Optimization

Many `std::string` implementations use Small String Optimization, SSO.

This means short strings may be stored inside the `std::string` object itself without heap allocation.

Example:

```cpp
std::string s = "hello";
```

May not allocate heap memory.

This is an implementation optimization, not something to rely on for correctness.

---

## 19. Common Interview Questions

### Q1. Difference between std::string and string_view?

`std::string` owns its characters.

`std::string_view` is a non-owning pointer-length view.

`string_view` is cheap but can dangle if the underlying data does not outlive it.

---

### Q2. Is string_view null-terminated?

Not necessarily.

`string_view` stores pointer and length, so it may refer to a substring that is not null-terminated.

---

### Q3. Why is returning string_view to a local string bad?

Because the local string is destroyed when the function returns.

The returned `string_view` points to invalid memory.

---

### Q4. When should a function take string_view?

When it only needs read-only access and does not store the view.

---

### Q5. When should a function take std::string?

When it needs ownership or stores the value.

---

### Q6. Is returning std::string by value okay?

Yes.

Modern C++ return-by-value is efficient due to copy elision and move semantics.

---

## 20. Key Takeaways

- `std::string` owns characters.
- `std::string_view` does not own characters.
- `const char*` is a pointer to null-terminated data.
- `string::c_str()` pointer depends on the string's lifetime and stability.
- `string_view` can dangle.
- `string_view` may not be null-terminated.
- Use `string_view` for read-only non-storing parameters.
- Use `std::string` when storing or owning.
- Returning `std::string` by value is safe.
- Returning `string_view` to local string is dangerous.
