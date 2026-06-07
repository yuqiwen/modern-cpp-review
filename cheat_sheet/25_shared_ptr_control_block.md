# 25. shared_ptr Control Block

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. shared_ptr Copy](#2-shared_ptr-copy)
- [3. weak_ptr and Control Block](#3-weak_ptr-and-control-block)
- [4. Object Lifetime vs Control Block Lifetime](#4-object-lifetime-vs-control-block-lifetime)
- [5. make_shared](#5-make_shared)
- [6. make_shared Tradeoff](#6-make_shared-tradeoff)
- [7. shared_ptr<T>(new T)](#7-shared_ptrtnew-t)
- [8. Two shared_ptr from Same Raw Pointer](#8-two-shared_ptr-from-same-raw-pointer)
- [9. shared_ptr from this Is Dangerous](#9-shared_ptr-from-this-is-dangerous)
- [10. enable_shared_from_this](#10-enable_shared_from_this)
- [11. Important Rule for shared_from_this](#11-important-rule-for-shared_from_this)
- [12. Why enable_shared_from_this Works](#12-why-enable_shared_from_this-works)
- [13. Aliasing Constructor](#13-aliasing-constructor)
- [14. Why Aliasing Constructor Matters](#14-why-aliasing-constructor-matters)
- [15. shared_ptr and Custom Deleter](#15-shared_ptr-and-custom-deleter)
- [16. shared_ptr Cost](#16-shared_ptr-cost)
- [17. Passing shared_ptr](#17-passing-shared_ptr)
- [18. owner_before](#18-owner_before)
- [19. Common Interview Questions](#19-common-interview-questions)
- [20. Key Takeaways](#20-key-takeaways)

## Related Code Trap

- [shared_ptr Control Block Demo](../code_traps/shared_ptr_control_block.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

`std::shared_ptr<T>` manages shared ownership through a control block.

Conceptually, a `shared_ptr` contains:

```text
1. pointer to object
2. pointer to control block
```

Control block stores:

```text
strong count
weak count
deleter
allocator information
```

When strong count becomes 0, the managed object is destroyed.

When both strong count and weak count become 0, the control block is released.

---

## 2. shared_ptr Copy

Example:

```cpp
auto p1 = std::make_shared<int>(42);
auto p2 = p1;
```

Now both `p1` and `p2` share the same control block.

```text
strong count = 2
```

When one `shared_ptr` is destroyed:

```text
strong count decreases
```

When strong count reaches 0:

```text
object is destroyed
```

---

## 3. weak_ptr and Control Block

`weak_ptr` observes the same control block but does not increase strong count.

Example:

```cpp
std::weak_ptr<int> w = p1;
```

This increases weak count, not strong count.

To access object:

```cpp
if (auto sp = w.lock()) {
    // object still alive
}
```

If strong count is 0:

```cpp
w.lock()
```

returns empty `shared_ptr`.

---

## 4. Object Lifetime vs Control Block Lifetime

Important:

```text
strong count > 0:
    object alive

strong count == 0:
    object destroyed

weak count > 0:
    control block may still exist
```

So after object destruction, `weak_ptr` can still know that the object expired because the control block remains.

---

## 5. make_shared

Prefer:

```cpp
auto p = std::make_shared<T>(args...);
```

Usually it performs one allocation for:

```text
control block + T object
```

This is more efficient than:

```cpp
std::shared_ptr<T> p(new T(args...));
```

which often does:

```text
one allocation for T object
one allocation for control block
```

---

## 6. make_shared Tradeoff

`make_shared` usually combines object and control block into one allocation.

Good:

```text
fewer allocations
better locality
exception safe
clearer
```

Tradeoff:

```text
if weak_ptrs remain after all shared_ptrs are gone,
the combined allocation may not be freed until weak_ptrs are gone
```

The object destructor runs when strong count becomes 0.

But the memory block containing object + control block may stay allocated while weak_ptr exists.

Usually not a problem, but worth knowing.

---

## 7. shared_ptr<T>(new T)

Example:

```cpp
std::shared_ptr<T> p(new T(args...));
```

This is valid but less preferred.

It usually allocates:

```text
T object separately
control block separately
```

When strong count reaches 0:

```text
T object is destroyed and its memory can be freed
```

Control block may remain if weak_ptr exists.

This can be useful in rare cases with very large objects and long-lived weak_ptrs, but normally `make_shared` is preferred.

---

## 8. Two shared_ptr from Same Raw Pointer

Bad:

```cpp
T* raw = new T();

std::shared_ptr<T> p1(raw);
std::shared_ptr<T> p2(raw); // wrong
```

This creates two separate control blocks.

Conceptually:

```text
p1 control block thinks it owns raw
p2 control block also thinks it owns raw
```

When both die:

```text
delete raw twice
```

Undefined behavior.

Correct:

```cpp
auto p1 = std::make_shared<T>();
auto p2 = p1;
```

or:

```cpp
std::shared_ptr<T> p1(new T());
std::shared_ptr<T> p2 = p1;
```

---

## 9. shared_ptr from this Is Dangerous

Bad:

```cpp
class Session {
public:
    std::shared_ptr<Session> getSelf() {
        return std::shared_ptr<Session>(this); // wrong
    }
};
```

Why?

If the object is already managed by a `shared_ptr`, this creates a second control block.

That can cause double delete.

---

## 10. enable_shared_from_this

Correct way:

```cpp
class Session : public std::enable_shared_from_this<Session> {
public:
    std::shared_ptr<Session> getSelf() {
        return shared_from_this();
    }
};
```

Usage:

```cpp
auto s = std::make_shared<Session>();
auto self = s->getSelf();
```

Now `self` shares the same control block as `s`.

---

## 11. Important Rule for shared_from_this

This is wrong:

```cpp
Session s;
s.shared_from_this(); // bad
```

or:

```cpp
auto raw = new Session();
raw->shared_from_this(); // bad unless already owned by shared_ptr
```

The object must already be owned by a `shared_ptr`.

Correct:

```cpp
auto s = std::make_shared<Session>();
auto self = s->shared_from_this();
```

If not owned by `shared_ptr`, `shared_from_this()` throws `std::bad_weak_ptr`.

---

## 12. Why enable_shared_from_this Works

`enable_shared_from_this<T>` internally stores a weak reference to the control block.

When a `shared_ptr<T>` first owns the object, it initializes that internal weak pointer.

Then:

```cpp
shared_from_this()
```

locks that internal weak pointer and returns another `shared_ptr` sharing the same control block.

---

## 13. Aliasing Constructor

Advanced but useful.

Syntax:

```cpp
std::shared_ptr<U> q(p, rawSubPointer);
```

This means:

```text
q shares ownership with p's control block
but q.get() points to rawSubPointer
```

Example:

```cpp
struct User {
    std::string name;
};

auto user = std::make_shared<User>();
std::shared_ptr<std::string> namePtr(user, &user->name);
```

`namePtr` points to `user->name`, but keeps the whole `User` alive.

---

## 14. Why Aliasing Constructor Matters

Without aliasing constructor, this is dangerous:

```cpp
std::shared_ptr<std::string> bad(&user->name);
```

This tries to delete `&user->name` as if it were separately allocated.

Wrong.

Correct:

```cpp
std::shared_ptr<std::string> good(user, &user->name);
```

This says:

```text
own User through user's control block
point to name subobject
```

---

## 15. shared_ptr and Custom Deleter

You can specify custom deleter:

```cpp
std::shared_ptr<FILE> file(
    std::fopen("data.txt", "r"),
    [](FILE* f) {
        if (f) std::fclose(f);
    }
);
```

The deleter is stored in the control block.

When strong count reaches zero, the deleter is called.

---

## 16. shared_ptr Cost

A `shared_ptr` copy usually does atomic reference count operations.

This has cost:

```text
more expensive than raw pointer
more expensive than unique_ptr move
potential contention in multithreaded code
```

So:

```text
do not use shared_ptr by default
use it only when shared ownership is needed
```

---

## 17. Passing shared_ptr

### Pass by value

```cpp
void keep(std::shared_ptr<T> p);
```

Means:

```text
function shares ownership
may store it
increments strong count
```

### Pass by const reference

```cpp
void observePtr(const std::shared_ptr<T>& p);
```

Avoids incrementing count, but still exposes ownership type.

### Prefer reference for non-owning access

```cpp
void use(const T& obj);
void maybeUse(const T* obj);
```

If function does not need ownership, prefer object reference/pointer.

---

## 18. owner_before

Two `shared_ptr`s can point to different subobjects but share the same control block due to aliasing.

To compare ownership/control block identity, there is:

```cpp
owner_before
```

Usually rare in interviews, but good to know conceptually:

```text
get() compares raw pointed address
owner_before relates to control block ownership ordering
```

---

## 19. Common Interview Questions

### Q1. What is the shared_ptr control block?

It is the heap-allocated metadata shared by all `shared_ptr`s owning the same object.

It stores strong count, weak count, deleter, and allocator information.

---

### Q2. Why is creating two shared_ptr from the same raw pointer bad?

Because each shared_ptr creates a separate control block.

Both control blocks think they own the same object, causing double delete.

---

### Q3. Why use make_shared?

It is clearer, exception-safe, and usually allocates object and control block together in one allocation.

---

### Q4. What is enable_shared_from_this?

It allows an object already owned by a shared_ptr to safely create another shared_ptr to itself using the same control block.

---

### Q5. What is the aliasing constructor?

It creates a shared_ptr that shares ownership with another shared_ptr but points to a different raw pointer, often a subobject.

---

## 20. Key Takeaways

- shared_ptr ownership is managed through a control block.
- Copying shared_ptr increments strong count.
- weak_ptr observes the control block without keeping object alive.
- make_shared usually combines object and control block allocation.
- Never create two shared_ptrs from the same raw pointer.
- Never use shared_ptr<T>(this) for self ownership.
- Use enable_shared_from_this for safe self shared_ptr.
- Aliasing constructor can own one object but point to a subobject.
- shared_ptr has real runtime cost; do not use it everywhere by default.
