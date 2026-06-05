# 23. Copy Elision, RVO, NRVO, and Return by Value

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Return by Value](#2-return-by-value)
- [3. RVO: Return Value Optimization](#3-rvo-return-value-optimization)
- [4. NRVO: Named Return Value Optimization](#4-nrvo-named-return-value-optimization)
- [5. RVO vs NRVO](#5-rvo-vs-nrvo)
- [6. What If NRVO Does Not Happen?](#6-what-if-nrvo-does-not-happen)
- [7. Do Not Write `return std::move(local)` for NRVO](#7-do-not-write-return-stdmovelocal-for-nrvo)
- [8. Returning `unique_ptr` by Value](#8-returning-unique_ptr-by-value)
- [9. Returning Local unique_ptr](#9-returning-local-unique_ptr)
- [10. Dangerous: Returning Raw Pointer from Local unique_ptr](#10-dangerous-returning-raw-pointer-from-local-unique_ptr)
- [11. Dangerous: Returning Reference to Local Object](#11-dangerous-returning-reference-to-local-object)
- [12. Return by Value vs Output Parameter](#12-return-by-value-vs-output-parameter)
- [13. Copy Elision and Constructors](#13-copy-elision-and-constructors)
- [14. But NRVO Still May Need Move/Copy Available](#14-but-nrvo-still-may-need-movecopy-available)
- [15. Multiple Return Paths Can Block NRVO](#15-multiple-return-paths-can-block-nrvo)
- [16. Copy Elision vs std::move](#16-copy-elision-vs-stdmove)
- [17. When Should You Use std::move in Return?](#17-when-should-you-use-stdmove-in-return)
- [18. Returning Function Parameter](#18-returning-function-parameter)
- [19. Common Interview Questions](#19-common-interview-questions)
- [20. Key Takeaways](#20-key-takeaways)

## Related Code Trap

- [Copy Elision / RVO Demo](../code_traps/copy_elision_rvo.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

Copy elision means the compiler eliminates an unnecessary copy or move.

Instead of:

```text
construct local object
move/copy it to return value
destroy local object
```

the compiler may directly construct the result in the caller's storage.

This is why returning objects by value is often efficient in modern C++.

---

## 2. Return by Value

Example:

```cpp
std::string makeName() {
    return std::string("Yuqi");
}
```

Caller:

```cpp
std::string name = makeName();
```

Modern C++ can construct the returned `std::string` directly as `name`.

No extra copy is needed.

---

## 3. RVO: Return Value Optimization

RVO usually refers to returning a temporary directly:

```cpp
Widget makeWidget() {
    return Widget("hello");
}
```

The returned object can be constructed directly in the caller.

Conceptually:

```text
caller provides storage for return value
function constructs Widget directly there
```

In C++17, this kind of copy elision for prvalues is guaranteed in many cases.

---

## 4. NRVO: Named Return Value Optimization

NRVO refers to returning a named local variable:

```cpp
Widget makeWidget() {
    Widget w("hello");
    return w;
}
```

The compiler may construct `w` directly in the caller's return storage.

This avoids copy/move.

Important:

```text
NRVO is allowed but not always guaranteed.
```

Most modern compilers do it in simple cases.

---

## 5. RVO vs NRVO

### RVO

```cpp
return Widget("hello");
```

Returning a temporary/prvalue.

Often guaranteed since C++17.

### NRVO

```cpp
Widget w("hello");
return w;
```

Returning a named local variable.

Allowed optimization, very common, but not guaranteed in all cases.

---

## 6. What If NRVO Does Not Happen?

If NRVO does not happen, the local object is usually moved if possible.

Example:

```cpp
Widget makeWidget() {
    Widget w("hello");
    return w;
}
```

If NRVO is not performed:

```text
move constructor may be called
```

If move is not available, copy may be used if available.

---

## 7. Do Not Write `return std::move(local)` for NRVO

Bad:

```cpp
Widget makeWidget() {
    Widget w("hello");
    return std::move(w);
}
```

Why bad?

Because `std::move(w)` turns `w` into an xvalue expression, which can prevent NRVO.

Better:

```cpp
Widget makeWidget() {
    Widget w("hello");
    return w;
}
```

Let the compiler apply NRVO. If NRVO does not happen, the compiler can still move.

Rule:

```text
Do not std::move a local variable in a return statement when returning by value.
```

---

## 8. Returning `unique_ptr` by Value

This is safe and common:

```cpp
std::unique_ptr<User> makeUser() {
    return std::make_unique<User>("Yuqi");
}
```

`unique_ptr` is not copyable, but it is movable.

The return is valid because:

```text
copy elision may happen
or move transfers ownership
```

Caller:

```cpp
auto p = makeUser();
```

Now caller owns the object.

---

## 9. Returning Local unique_ptr

Also valid:

```cpp
std::unique_ptr<User> makeUser() {
    auto p = std::make_unique<User>("Yuqi");
    return p;
}
```

Do not write:

```cpp
return std::move(p);
```

Usually just write:

```cpp
return p;
```

The compiler can apply NRVO or move automatically.

---

## 10. Dangerous: Returning Raw Pointer from Local unique_ptr

Bad:

```cpp
User* makeUserBad() {
    auto p = std::make_unique<User>("Yuqi");
    return p.get();
}
```

The returned raw pointer dangles.

Why?

```text
p is destroyed when the function ends
p deletes the User
caller receives pointer to deleted object
```

Correct:

```cpp
std::unique_ptr<User> makeUser() {
    return std::make_unique<User>("Yuqi");
}
```

---

## 11. Dangerous: Returning Reference to Local Object

Bad:

```cpp
std::string& bad() {
    std::string s = "hello";
    return s;
}
```

` s ` is destroyed when function returns.

The returned reference dangles.

Also bad:

```cpp
const std::string& bad() {
    return std::string("hello");
}
```

The temporary does not live long enough for caller.

Rule:

```text
Never return reference or pointer to local objects.
```

---

## 12. Return by Value vs Output Parameter

Old style:

```cpp
void makeVector(std::vector<int>& out) {
    out.push_back(1);
    out.push_back(2);
}
```

Modern style:

```cpp
std::vector<int> makeVector() {
    std::vector<int> v;
    v.push_back(1);
    v.push_back(2);
    return v;
}
```

Modern return-by-value is usually clearer and efficient due to RVO/NRVO/move.

---

## 13. Copy Elision and Constructors

Suppose:

```cpp
Widget makeWidget() {
    return Widget("hello");
}
```

With guaranteed copy elision, the compiler may not need move/copy constructor at all.

This means some code can compile even if copy/move constructors are deleted.

Example:

```cpp
struct NonMovable {
    NonMovable() = default;
    NonMovable(const NonMovable&) = delete;
    NonMovable(NonMovable&&) = delete;
};

NonMovable make() {
    return NonMovable();
}
```

In C++17, this can work because the object is constructed directly in the caller.

---

## 14. But NRVO Still May Need Move/Copy Available

This may be different:

```cpp
NonMovable make() {
    NonMovable x;
    return x;
}
```

NRVO is not guaranteed in every case.

If the compiler does not perform NRVO, it would need move/copy.

Since move/copy are deleted, this may fail depending on standard rules and situation.

So for non-movable types, prefer direct prvalue return:

```cpp
return NonMovable();
```

---

## 15. Multiple Return Paths Can Block NRVO

Example:

```cpp
Widget make(bool flag) {
    Widget a("A");
    Widget b("B");

    if (flag) {
        return a;
    } else {
        return b;
    }
}
```

NRVO may not apply because there are multiple named local return objects.

If no NRVO, move may happen.

A better design sometimes returns prvalues:

```cpp
Widget make(bool flag) {
    if (flag) {
        return Widget("A");
    } else {
        return Widget("B");
    }
}
```

This is more friendly to guaranteed copy elision.

---

## 16. Copy Elision vs std::move

`std::move` does not move by itself.

It only casts to rvalue.

Example:

```cpp
return std::move(w);
```

This tells compiler:

```text
treat w as rvalue
```

Then move constructor may be called.

But this may prevent NRVO.

So:

```cpp
return w;
```

is usually better for local return objects.

---

## 17. When Should You Use std::move in Return?

Usually not for local variables returned by value.

Do not:

```cpp
return std::move(local);
```

But sometimes returning a member by value may need `std::move` if you intentionally move from the object:

```cpp
class Holder {
private:
    std::string value;

public:
    std::string take() {
        return std::move(value);
    }
};
```

Here `value` is a member, not a local return object.

NRVO does not apply to returning a member.

This intentionally leaves `value` moved-from.

---

## 18. Returning Function Parameter

Example:

```cpp
Widget f(Widget w) {
    return w;
}
```

`w` is a function parameter.

NRVO does not apply to parameters.

But C++ can move from `w` on return.

Usually write:

```cpp
return w;
```

not necessarily:

```cpp
return std::move(w);
```

Modern rules can treat it as move-eligible.

For clarity, many codebases still avoid explicit `std::move` unless needed.

---

## 19. Common Interview Questions

### Q1. What is copy elision?

Copy elision is a compiler optimization that eliminates unnecessary copy or move construction by constructing the object directly in its final destination.

---

### Q2. What is RVO?

Return Value Optimization happens when a returned temporary object is constructed directly in the caller's storage.

Example:

```cpp
return Widget();
```

---

### Q3. What is NRVO?

Named Return Value Optimization happens when a named local variable is returned by value and the compiler constructs it directly in the caller's return storage.

Example:

```cpp
Widget w;
return w;
```

---

### Q4. Should we write `return std::move(local)`?

Usually no.

It can prevent NRVO. Prefer:

```cpp
return local;
```

If NRVO does not happen, the compiler can still move.

---

### Q5. Is returning `unique_ptr` by value safe?

Yes.

Returning `std::unique_ptr<T>` by value transfers ownership through move or copy elision.

---

### Q6. Why is returning `p.get()` from a local `unique_ptr` dangerous?

Because the local `unique_ptr` is destroyed at the end of the function and deletes the object.

The returned raw pointer becomes dangling.

---

## 20. Key Takeaways

- Return by value is efficient in modern C++.
- RVO returns a temporary directly.
- NRVO returns a named local object directly when optimized.
- C++17 guarantees many prvalue copy elision cases.
- Do not write `return std::move(local)` for local return objects.
- Returning `unique_ptr` by value is safe.
- Returning pointer/reference to local object is dangerous.
- `std::move` may prevent NRVO.
- For factories, prefer returning by value.
