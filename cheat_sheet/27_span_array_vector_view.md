# 27. std::span, Array, Vector, and Views

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. C-style Array](#2-c-style-array)
- [3. Array Decay](#3-array-decay)
- [4. std::array](#4-stdarray)
- [5. std::vector](#5-stdvector)
- [6. std::span](#6-stdspan)
- [7. span Is a View](#7-span-is-a-view)
- [8. span vs string_view](#8-span-vs-string_view)
- [9. span Lifetime Danger](#9-span-lifetime-danger)
- [10. span and vector Reallocation](#10-span-and-vector-reallocation)
- [11. Passing Sequence to Function](#11-passing-sequence-to-function)
- [12. span<const T> vs const span<T>](#12-spanconst-t-vs-const-spant)
- [13. Static Extent span](#13-static-extent-span)
- [14. subspan](#14-subspan)
- [15. data() and size()](#15-data-and-size)
- [16. span Requires Contiguous Memory](#16-span-requires-contiguous-memory)
- [17. When to Use span](#17-when-to-use-span)
- [18. When Not to Store span](#18-when-not-to-store-span)
- [19. span vs vector Parameter](#19-span-vs-vector-parameter)
- [20. Common Interview Questions](#20-common-interview-questions)
- [21. Key Takeaways](#21-key-takeaways)

## Related Code Trap

- [span / array / vector view Demo](../code_traps/span_array_vector_view.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

C++ has several ways to represent a sequence of elements:

```cpp
T arr[N];              // C-style array
std::array<T, N>       // fixed-size owning array
std::vector<T>         // dynamic owning array
std::span<T>           // non-owning view over contiguous elements
```

Ownership meaning:

```text
T arr[N]        owns N elements in its scope
std::array      owns N elements
std::vector     owns dynamic elements
std::span       owns nothing
```

`std::span` is like `std::string_view`, but for arbitrary contiguous types.

---

## 2. C-style Array

Example:

```cpp
int arr[3] = {1, 2, 3};
```

Properties:

```text
fixed size
contiguous memory
size information is easily lost when passed to functions
cannot be reassigned
```

Inside the same scope:

```cpp
sizeof(arr) / sizeof(arr[0]) // 3
```

But when passed to function:

```cpp
void f(int arr[]);
```

This actually becomes:

```cpp
void f(int* arr);
```

So size information is lost.

---

## 3. Array Decay

C-style arrays often decay to pointer.

Example:

```cpp
void print(int* p);
```

Call:

```cpp
int arr[3] = {1, 2, 3};
print(arr);
```

`arr` decays to pointer to first element:

```cpp
int*
```

The function receives only the starting address, not the length.

That is why C APIs often need:

```cpp
void f(int* data, size_t size);
```

---

## 4. std::array

```cpp
std::array<int, 3> arr = {1, 2, 3};
```

Properties:

```text
fixed size known at compile time
owns elements
contiguous memory
has size()
works with STL algorithms
does not decay automatically like C array
```

Example:

```cpp
arr.size();   // 3
arr.data();   // pointer to first element
```

Use `std::array<T, N>` instead of raw C array when possible.

---

## 5. std::vector

```cpp
std::vector<int> v = {1, 2, 3};
```

Properties:

```text
dynamic size
owns elements
contiguous memory
size and capacity
can reallocate
```

Use `std::vector<T>` when size changes dynamically.

---

## 6. std::span

Header:

```cpp
#include <span>
```

C++20.

Example:

```cpp
void print(std::span<int> s) {
    for (int& x : s) {
        std::cout << x << " ";
    }
}
```

Can accept:

```cpp
std::vector<int> v = {1, 2, 3};
std::array<int, 3> a = {4, 5, 6};
int raw[3] = {7, 8, 9};

print(v);
print(a);
print(raw);
```

`span` stores:

```text
pointer + size
```

It does not own elements.

---

## 7. span Is a View

```cpp
std::vector<int> v = {1, 2, 3};

std::span<int> s = v;
```

`s` views `v`'s data.

If we modify through `s`:

```cpp
s[0] = 10;
```

Then:

```cpp
v[0] == 10
```

So `span<int>` is mutable view.

If read-only:

```cpp
std::span<const int> s = v;
```

Then:

```cpp
s[0] = 10; // error
```

---

## 8. span vs string_view

Similar idea:

```text
string_view = non-owning view of characters
span<T>     = non-owning view of contiguous T elements
```

Both store:

```text
pointer + length
```

Both can dangle if underlying data dies.

Difference:

```text
string_view is read-only character view
span<T> can be mutable if T is non-const
```

---

## 9. span Lifetime Danger

Dangerous:

```cpp
std::span<int> bad() {
    std::vector<int> v = {1, 2, 3};
    return std::span<int>(v); // dangling
}
```

`v` is destroyed when function returns.

Returned span points to invalid memory.

Correct:

```cpp
std::vector<int> makeVector() {
    return {1, 2, 3};
}
```

or pass span only as a parameter for temporary view usage.

---

## 10. span and vector Reallocation

```cpp
std::vector<int> v = {1, 2, 3};
std::span<int> s = v;

v.push_back(4); // may reallocate

s[0]; // may be dangling
```

If vector reallocates, the old data pointer is invalid.

Since span stores old pointer + length, it becomes dangling.

Same issue as raw pointer / iterator / string_view.

---

## 11. Passing Sequence to Function

Old style:

```cpp
void process(int* data, size_t size);
```

Modern C++20 style:

```cpp
void process(std::span<int> data);
```

Read-only:

```cpp
void inspect(std::span<const int> data);
```

Benefits:

```text
data pointer and size travel together
works for vector, array, raw array
clear non-owning semantics
```

---

## 12. span<const T> vs const span<T>

This is important.

```cpp
std::span<const int> s;
```

Means:

```text
view of const int
cannot modify elements through s
```

```cpp
const std::span<int> s;
```

Means:

```text
span object itself is const
cannot reseat/change the span
but elements may still be mutable
```

Example:

```cpp
std::vector<int> v = {1, 2, 3};

const std::span<int> s = v;
s[0] = 10; // OK, element is int
// s = anotherSpan; // error, span object is const
```

But:

```cpp
std::span<const int> s = v;
s[0] = 10; // error
```

Same idea as:

```cpp
const T* p      // pointer to const T
T* const p      // const pointer to mutable T
```

---

## 13. Static Extent span

`std::span` can have dynamic extent:

```cpp
std::span<int> s;
```

or static extent:

```cpp
std::span<int, 3> s;
```

Static extent means size is part of the type.

Example:

```cpp
void f(std::span<int, 3> s);
```

This only accepts spans of exactly 3 elements.

Most common usage is dynamic extent:

```cpp
std::span<T>
```

---

## 14. subspan

Like `string_view::substr`, span has:

```cpp
s.subspan(offset, count)
```

Example:

```cpp
std::vector<int> v = {1, 2, 3, 4, 5};
std::span<int> s = v;

auto mid = s.subspan(1, 3); // views 2, 3, 4
```

This does not copy elements.

It creates another view.

Lifetime still depends on original data.

---

## 15. data() and size()

Both vector/array/span expose:

```cpp
.data()
.size()
```

Example:

```cpp
std::span<int> s = v;

int* p = s.data();
size_t n = s.size();
```

This is useful for C APIs:

```cpp
c_api(s.data(), s.size());
```

Unlike C string APIs, here the size is passed explicitly.

---

## 16. span Requires Contiguous Memory

`std::span` can view:

```cpp
std::vector<T>
std::array<T, N>
T[N]
contiguous buffer
```

It cannot view:

```cpp
std::list<T>
std::map<K, V>
std::set<T>
```

Because those are not contiguous.

Example:

```cpp
std::list<int> lst = {1, 2, 3};
std::span<int> s = lst; // error
```

---

## 17. When to Use span

Use `span` for function parameters when:

```text
function does not own data
function needs contiguous sequence
function may modify elements
or function needs size + pointer together
```

Examples:

```cpp
void normalize(std::span<float> values);
void print(std::span<const int> values);
void fillZeros(std::span<int> values);
```

---

## 18. When Not to Store span

Usually do not store `span` as a long-lived member unless you can guarantee lifetime.

Dangerous:

```cpp
class Processor {
private:
    std::span<int> data;
};
```

This is okay only if the owner of the data definitely outlives `Processor`.

Otherwise, dangling risk.

Same warning as `string_view`.

---

## 19. span vs vector Parameter

Bad if function only observes:

```cpp
void print(const std::vector<int>& v);
```

This only accepts vector.

Better:

```cpp
void print(std::span<const int> values);
```

This accepts vector, array, raw array, and subspan.

But if function needs ownership or storage, use vector.

---

## 20. Common Interview Questions

### Q1. What is std::span?

`std::span` is a non-owning view over a contiguous sequence of elements.

It stores pointer and size.

---

### Q2. Does span own data?

No.

It only views data owned by something else.

---

### Q3. Can span dangle?

Yes.

If the underlying data is destroyed or reallocated, the span becomes invalid.

---

### Q4. Difference between span and vector?

`vector` owns dynamic storage.

`span` does not own anything; it only views existing contiguous storage.

---

### Q5. Difference between span and string_view?

Both are non-owning pointer-length views.

`string_view` is a read-only view of characters.

`span<T>` is a view of contiguous `T` elements and can be mutable if `T` is non-const.

---

### Q6. Why use span instead of pointer + size?

`span` packages pointer and size together, making APIs clearer and less error-prone.

---

## 21. Key Takeaways

- C arrays decay to pointers and lose size.
- `std::array` owns fixed-size contiguous data.
- `std::vector` owns dynamic contiguous data.
- `std::span` is a non-owning pointer-size view.
- `span` can view vector, array, and C array.
- `span` cannot view list/map/set because they are not contiguous.
- `span` can dangle if source data dies or reallocates.
- Use `span<const T>` for read-only sequence parameters.
- Use `span<T>` for mutable sequence parameters.
- Do not store long-lived span unless lifetime is guaranteed.
