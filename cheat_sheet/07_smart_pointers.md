# 07. Smart Pointers

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Why Smart Pointers?](#2-why-smart-pointers)
- [3. unique_ptr](#3-unique_ptr)
- [4. unique_ptr Is Move-only](#4-unique_ptr-is-move-only)
- [5. unique_ptr and Rule of Zero](#5-unique_ptr-and-rule-of-zero)
- [6. Passing unique_ptr to Functions](#6-passing-unique_ptr-to-functions)
- [7. make_unique](#7-make_unique)
- [8. unique_ptr for Arrays](#8-unique_ptr-for-arrays)
- [9. release vs reset vs get](#9-release-vs-reset-vs-get)
- [10. shared_ptr](#10-shared_ptr)
- [11. make_shared](#11-make_shared)
- [12. shared_ptr Control Block](#12-shared_ptr-control-block)
- [13. shared_ptr Copy vs Move](#13-shared_ptr-copy-vs-move)
- [14. Passing shared_ptr to Functions](#14-passing-shared_ptr-to-functions)
- [15. weak_ptr](#15-weak_ptr)
- [16. Why weak_ptr Exists: Cyclic Reference](#16-why-weak_ptr-exists-cyclic-reference)
- [17. enable_shared_from_this](#17-enable_shared_from_this)
- [18. Common Mistake: shared_ptr from Raw this](#18-common-mistake-shared_ptr-from-raw-this)
- [19. Raw Pointers Are Still Useful](#19-raw-pointers-are-still-useful)
- [20. Choosing the Right Pointer](#20-choosing-the-right-pointer)
- [21. Common Interview Questions](#21-common-interview-questions)
- [22. Key Takeaways](#22-key-takeaways)

## Related Code Trap

- [Smart Pointers Demo](../code_traps/smart_pointers.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

Smart pointers are RAII wrappers for dynamic memory ownership.

They automatically release resources in their destructors.

The three most important smart pointers are:

```cpp
std::unique_ptr<T>
std::shared_ptr<T>
std::weak_ptr<T>
```

Ownership meanings:

```text
unique_ptr: exclusive ownership
shared_ptr: shared ownership
weak_ptr: non-owning observer of shared ownership
```

---

## 2. Why Smart Pointers?

Raw pointer:

```cpp
T* p = new T();
delete p;
```

Problems:

- easy to forget `delete`
- double delete risk
- unclear ownership
- not exception-safe

Smart pointer:

```cpp
auto p = std::make_unique<T>();
```

When `p` leaves scope, the object is automatically deleted.

This follows RAII.

---

## 3. unique_ptr

`std::unique_ptr<T>` means exclusive ownership.

Only one `unique_ptr` owns the object at a time.

Example:

```cpp
std::unique_ptr<int> p = std::make_unique<int>(10);
```

When `p` is destroyed, it deletes the managed `int`.

---

## 4. unique_ptr Is Move-only

This is not allowed:

```cpp
std::unique_ptr<int> p1 = std::make_unique<int>(10);
std::unique_ptr<int> p2 = p1; // error
```

Why?

Because copying would create two owners of the same object, leading to double delete.

This is allowed:

```cpp
std::unique_ptr<int> p1 = std::make_unique<int>(10);
std::unique_ptr<int> p2 = std::move(p1);
```

After move:

```text
p2 owns the object.
p1 becomes nullptr.
```

---

## 5. unique_ptr and Rule of Zero

If a class uses `unique_ptr` as a member, it often does not need a manual destructor.

Example:

```cpp
class Owner {
private:
    std::unique_ptr<Resource> resource;
};
```

The compiler-generated destructor will destroy `resource`, and `resource` will delete the managed object.

However, because `unique_ptr` is not copyable, the class will not be copyable by default.

It can still be movable if appropriate.

---

## 6. Passing unique_ptr to Functions

### Pass by value: transfer ownership

```cpp
void takeOwnership(std::unique_ptr<Resource> p);
```

Call:

```cpp
auto p = std::make_unique<Resource>();
takeOwnership(std::move(p));
```

This means the function takes ownership.

After the call:

```text
p == nullptr
```

---

### Pass by reference: do not transfer ownership

```cpp
void useResource(Resource& r);
```

or:

```cpp
void useResource(const Resource& r);
```

If the function only uses the object, prefer reference.

Do not pass `unique_ptr` just because the object is stored in a `unique_ptr`.

Example:

```cpp
void print(const Resource& r);
print(*p);
```

---

### Pass unique_ptr by const reference?

```cpp
void inspectPtr(const std::unique_ptr<Resource>& p);
```

This means:

- the function does not take ownership
- the function may inspect whether the pointer is null
- the function can access the resource

But if nullability is not important, prefer:

```cpp
void inspect(const Resource& r);
```

---

## 7. make_unique

Prefer:

```cpp
auto p = std::make_unique<T>(args);
```

over:

```cpp
std::unique_ptr<T> p(new T(args));
```

Why?

- cleaner
- exception-safe
- avoids raw `new`
- less repetitive

---

## 8. unique_ptr for Arrays

Use:

```cpp
std::unique_ptr<int[]> arr = std::make_unique<int[]>(10);
```

Access:

```cpp
arr[0] = 42;
```

But for dynamic arrays, usually prefer:

```cpp
std::vector<int> arr;
```

because vector stores size and supports copy/move behavior better.

---

## 9. release vs reset vs get

### get()

```cpp
T* raw = p.get();
```

Returns the raw pointer but does not release ownership.

`p` still owns the object.

Do not delete `raw`.

---

### release()

```cpp
T* raw = p.release();
```

Returns the raw pointer and releases ownership.

Now `p` becomes nullptr.

Someone else must delete `raw`.

This is dangerous and should be used carefully.

---

### reset()

```cpp
p.reset();
```

Deletes the currently owned object and makes `p` null.

```cpp
p.reset(new T());
```

Deletes old object and takes ownership of the new raw pointer.

Prefer `make_unique` where possible.

---

## 10. shared_ptr

`std::shared_ptr<T>` means shared ownership.

Multiple `shared_ptr`s can own the same object.

The object is destroyed when the last owning `shared_ptr` is destroyed.

Example:

```cpp
auto p1 = std::make_shared<int>(10);
auto p2 = p1;
```

Now both share ownership.

There is a reference count:

```text
use_count == 2
```

When both go away, the `int` is deleted.

---

## 11. make_shared

Prefer:

```cpp
auto p = std::make_shared<T>(args);
```

over:

```cpp
std::shared_ptr<T> p(new T(args));
```

Why?

`make_shared` usually performs one allocation for both:

- the object
- the control block

This is more efficient.

It is also cleaner and safer.

---

## 12. shared_ptr Control Block

A `shared_ptr` has two major parts:

```text
1. pointer to the managed object
2. pointer to a control block
```

The control block stores:

- strong reference count
- weak reference count
- deleter
- allocator information

When the strong count reaches zero, the managed object is destroyed.

When both strong and weak counts reach zero, the control block is destroyed.

---

## 13. shared_ptr Copy vs Move

Copying a shared_ptr increases the reference count:

```cpp
auto p1 = std::make_shared<int>(10);
auto p2 = p1; // use_count increases
```

Moving a shared_ptr transfers one ownership handle:

```cpp
auto p3 = std::move(p1);
```

After move:

```text
p3 owns the handle.
p1 is empty.
reference count does not increase.
```

---

## 14. Passing shared_ptr to Functions

### Pass by value

```cpp
void f(std::shared_ptr<T> p);
```

This increases the reference count.

Use this when the function needs to share ownership.

---

### Pass by const reference

```cpp
void f(const std::shared_ptr<T>& p);
```

This does not increase the reference count.

Use this when the function needs to inspect the smart pointer itself but does not need to share ownership.

---

### Prefer raw reference for observation

If the function only uses the object and does not care about ownership:

```cpp
void f(const T& obj);
```

Call:

```cpp
f(*p);
```

This is often best.

Important principle:

```text
Pass ownership only when ownership is part of the function's contract.
```

---

## 15. weak_ptr

`std::weak_ptr<T>` is a non-owning observer of an object managed by `shared_ptr`.

It does not increase the strong reference count.

Use it to avoid reference cycles.

Example:

```cpp
std::weak_ptr<T> w = shared;
```

To access the object, call:

```cpp
if (auto p = w.lock()) {
    // p is a shared_ptr<T>
}
```

If the object is already destroyed, `lock()` returns an empty `shared_ptr`.

---

## 16. Why weak_ptr Exists: Cyclic Reference

Bad example with shared_ptr cycle:

```cpp
class Node {
public:
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> prev;
};

auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();

a->next = b;
b->prev = a;
```

Even after `a` and `b` go out of scope, the objects may not be destroyed because they keep each other alive.

Fix:

```cpp
class Node {
public:
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;
};
```

Now `prev` observes without owning.

---

## 17. enable_shared_from_this

Sometimes an object managed by `shared_ptr` needs to safely create a `shared_ptr` to itself.

Use:

```cpp
class Session : public std::enable_shared_from_this<Session> {
public:
    std::shared_ptr<Session> getPtr() {
        return shared_from_this();
    }
};
```

Do not do this:

```cpp
std::shared_ptr<Session>(this);
```

Why?

Because it creates a separate control block, leading to double delete.

`enable_shared_from_this` lets the object reuse the existing control block.

---

## 18. Common Mistake: shared_ptr from Raw this

Bad:

```cpp
class Bad {
public:
    std::shared_ptr<Bad> getPtr() {
        return std::shared_ptr<Bad>(this);
    }
};
```

If the object is already managed by a `shared_ptr`, this creates a second control block.

Then two independent `shared_ptr` groups think they own the same object.

Result:

```text
double delete
undefined behavior
```

Correct:

```cpp
class Good : public std::enable_shared_from_this<Good> {
public:
    std::shared_ptr<Good> getPtr() {
        return shared_from_this();
    }
};
```

---

## 19. Raw Pointers Are Still Useful

Raw pointers are not banned.

They are okay for non-owning observation:

```cpp
void f(T* p);
```

This can mean:

```text
nullable non-owning reference
```

But raw owning pointers should usually be avoided.

Ownership should be represented by:

```cpp
std::unique_ptr<T>
std::shared_ptr<T>
```

---

## 20. Choosing the Right Pointer

```cpp
std::unique_ptr<T>
```

Use when there is one clear owner.

```cpp
std::shared_ptr<T>
```

Use when ownership is truly shared.

```cpp
std::weak_ptr<T>
```

Use to observe a shared object without extending its lifetime.

```cpp
T*
```

Use for nullable non-owning access.

```cpp
T&
```

Use for non-null non-owning access.

---

## 21. Common Interview Questions

### Q1. What is the difference between unique_ptr and shared_ptr?

`unique_ptr` represents exclusive ownership. It cannot be copied, only moved.

`shared_ptr` represents shared ownership. It can be copied, and the object is destroyed when the last shared owner is destroyed.

---

### Q2. Why is unique_ptr move-only?

Because copying a `unique_ptr` would create two owners of the same object, which could cause double delete.

Moving transfers ownership safely.

---

### Q3. When should you pass unique_ptr by value?

When the function should take ownership of the object.

Example:

```cpp
void setResource(std::unique_ptr<Resource> r);
```

The caller must use:

```cpp
setResource(std::move(ptr));
```

---

### Q4. When should you use weak_ptr?

Use `weak_ptr` to observe an object managed by `shared_ptr` without increasing the strong reference count.

It is especially useful for breaking cyclic references.

---

### Q5. What is the shared_ptr control block?

The control block stores the reference counts and deleter information.

A `shared_ptr` points to both the managed object and the control block.

The object is destroyed when the strong count reaches zero.

---

### Q6. Why should we avoid constructing shared_ptr from raw this?

Because it may create a separate control block.

If another shared_ptr already owns the object, this causes two independent ownership groups and may lead to double delete.

Use `enable_shared_from_this` instead.

---

## 22. Key Takeaways

- Smart pointers express ownership.
- `unique_ptr` means exclusive ownership.
- `shared_ptr` means shared ownership.
- `weak_ptr` means non-owning observer of shared ownership.
- Prefer `make_unique` and `make_shared`.
- Use `release()` carefully; it gives up ownership and returns raw pointer.
- Use `get()` only for non-owning access.
- Avoid raw owning pointers.
- Avoid unnecessary `shared_ptr`; use it only when ownership is truly shared.
- Use `weak_ptr` to break shared_ptr cycles.
- Do not create `shared_ptr<T>(this)` from an already managed object.
