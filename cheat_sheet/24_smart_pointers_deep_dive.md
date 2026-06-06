# 24. Smart Pointers Deep Dive

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. unique_ptr](#2-unique_ptr)
- [3. unique_ptr Is Move-only](#3-unique_ptr-is-move-only)
- [4. Passing unique_ptr to Functions](#4-passing-unique_ptr-to-functions)
- [5. Returning unique_ptr](#5-returning-unique_ptr)
- [6. make_unique](#6-make_unique)
- [7. unique_ptr with Polymorphism](#7-unique_ptr-with-polymorphism)
- [8. unique_ptr Custom Deleter](#8-unique_ptr-custom-deleter)
- [9. shared_ptr](#9-shared_ptr)
- [10. shared_ptr Control Block](#10-shared_ptr-control-block)
- [11. make_shared](#11-make_shared)
- [12. shared_ptr Copy Cost](#12-shared_ptr-copy-cost)
- [13. Passing shared_ptr to Functions](#13-passing-shared_ptr-to-functions)
- [14. weak_ptr](#14-weak_ptr)
- [15. shared_ptr Cycle Problem](#15-shared_ptr-cycle-problem)
- [16. Fix Cycle with weak_ptr](#16-fix-cycle-with-weak_ptr)
- [17. enable_shared_from_this](#17-enable_shared_from_this)
- [18. Do Not Create Two shared_ptr from Same Raw Pointer](#18-do-not-create-two-shared_ptr-from-same-raw-pointer)
- [19. Aliasing Constructor](#19-aliasing-constructor)
- [20. Raw Pointer vs unique_ptr vs shared_ptr](#20-raw-pointer-vs-unique_ptr-vs-shared_ptr)
- [21. Common Interview Questions](#21-common-interview-questions)
- [22. Key Takeaways](#22-key-takeaways)

## Related Code Trap

- [Smart Pointers Deep Dive Demo](../code_traps/smart_pointers_deep_dive.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

Smart pointers manage object lifetime automatically.

They help express ownership:

```cpp
std::unique_ptr<T>  // exclusive ownership
std::shared_ptr<T>  // shared ownership
std::weak_ptr<T>    // non-owning observer
```

Raw pointer:

```cpp
T*
```

should usually mean:

```text
non-owning pointer
nullable access
```

Reference:

```cpp
T&
```

should usually mean:

```text
non-owning access
object must exist
```

---

## 2. unique_ptr

`std::unique_ptr<T>` represents exclusive ownership.

Only one `unique_ptr` owns the object at a time.

Example:

```cpp
std::unique_ptr<int> p = std::make_unique<int>(42);
```

When `p` is destroyed, it automatically deletes the owned object.

---

## 3. unique_ptr Is Move-only

This is not allowed:

```cpp
std::unique_ptr<int> p1 = std::make_unique<int>(42);
std::unique_ptr<int> p2 = p1; // error
```

Because copying would create two owners of the same object.

Correct:

```cpp
std::unique_ptr<int> p2 = std::move(p1);
```

After move:

```text
p2 owns the object
p1 becomes nullptr
```

---

## 4. Passing unique_ptr to Functions

### Transfer ownership

```cpp
void take(std::unique_ptr<Resource> r);
```

Call:

```cpp
take(std::move(p));
```

Meaning:

```text
caller gives ownership to callee
```

After call:

```cpp
p == nullptr
```

---

### Borrow object

If the function only uses the object and does not take ownership:

```cpp
void use(Resource& r);
void inspect(const Resource& r);
```

Call:

```cpp
use(*p);
inspect(*p);
```

If `p` may be null, check first:

```cpp
if (p) {
    inspect(*p);
}
```

---

### Nullable non-owning access

If the function should accept null:

```cpp
void maybeUse(Resource* r);
```

Call:

```cpp
maybeUse(p.get());
```

`get()` does not transfer ownership.

---

## 5. Returning unique_ptr

Factory functions often return `unique_ptr` by value:

```cpp
std::unique_ptr<Resource> makeResource() {
    return std::make_unique<Resource>();
}
```

This is safe and efficient.

Ownership is transferred by move or copy elision.

Do not return:

```cpp
return p.get(); // bad if p is local
```

because the local `unique_ptr` will destroy the object when the function ends.

---

## 6. make_unique

Prefer:

```cpp
auto p = std::make_unique<T>(args...);
```

over:

```cpp
std::unique_ptr<T> p(new T(args...));
```

Reasons:

```text
clearer
exception-safe
less repetition
```

Example:

```cpp
auto user = std::make_unique<User>("Yuqi", 25);
```

---

## 7. unique_ptr with Polymorphism

Common pattern:

```cpp
class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;
};

class Circle : public Shape {
public:
    double area() const override {
        return 3.14;
    }
};
```

Use:

```cpp
std::unique_ptr<Shape> p = std::make_unique<Circle>();
```

This works because `Circle` is-a `Shape`.

Important:

```text
Base destructor must be virtual
```

Otherwise deleting derived object through base pointer is unsafe.

---

## 8. unique_ptr Custom Deleter

`unique_ptr` can use custom deleters.

Example for `FILE*`:

```cpp
struct FileCloser {
    void operator()(FILE* f) const {
        if (f) {
            std::fclose(f);
        }
    }
};

std::unique_ptr<FILE, FileCloser> file(std::fopen("data.txt", "r"));
```

When `file` is destroyed, it calls `FileCloser`.

This is RAII for C resources.

---

## 9. shared_ptr

`std::shared_ptr<T>` represents shared ownership.

Multiple `shared_ptr`s can own the same object.

The object is destroyed when the last owning `shared_ptr` is destroyed.

Example:

```cpp
auto p1 = std::make_shared<int>(42);
auto p2 = p1;
```

Now both own the same int.

```cpp
p1.use_count(); // usually 2
```

---

## 10. shared_ptr Control Block

`shared_ptr` uses a control block.

Conceptually:

```text
shared_ptr object:
  raw pointer to T
  pointer to control block

control block:
  strong count
  weak count
  deleter
  allocator info
```

When a `shared_ptr` is copied:

```text
strong count increases
```

When a `shared_ptr` is destroyed:

```text
strong count decreases
```

When strong count reaches 0:

```text
T object is destroyed
```

The control block may stay alive while weak_ptr exists.

---

## 11. make_shared

Prefer:

```cpp
auto p = std::make_shared<T>(args...);
```

over:

```cpp
std::shared_ptr<T> p(new T(args...));
```

Reasons:

```text
usually one allocation for object + control block
exception-safe
clearer
```

`make_shared` often allocates object and control block together.

This improves performance and cache locality.

---

## 12. shared_ptr Copy Cost

Copying a `shared_ptr` is more expensive than copying a raw pointer or unique_ptr move.

Why?

```text
it must update reference count
usually atomic increment/decrement
```

So do not use `shared_ptr` everywhere by default.

Use it only when shared ownership is truly needed.

---

## 13. Passing shared_ptr to Functions

### Function shares ownership

```cpp
void keep(std::shared_ptr<Resource> r);
```

Call:

```cpp
keep(p);
```

This increments use count.

Meaning:

```text
callee may store a copy and extend lifetime
```

---

### Function only observes

Prefer:

```cpp
void inspect(const Resource& r);
```

Call:

```cpp
inspect(*p);
```

Or nullable:

```cpp
void maybeInspect(const Resource* r);
maybeInspect(p.get());
```

Do not pass `shared_ptr` by value if the function does not need ownership.

---

### Read shared_ptr object without incrementing count

Sometimes:

```cpp
void observe(const std::shared_ptr<Resource>& p);
```

This avoids incrementing count.

But semantically, it still exposes that caller uses shared ownership.

Often better API:

```cpp
void observe(const Resource& r);
```

if ownership is irrelevant.

---

## 14. weak_ptr

`std::weak_ptr<T>` is a non-owning observer of an object managed by `shared_ptr`.

It does not increase strong count.

Use it to avoid cycles.

Example:

```cpp
std::weak_ptr<T> w = p;
```

To use it:

```cpp
if (auto sp = w.lock()) {
    // object is still alive
}
```

If object was destroyed:

```cpp
w.lock() // returns empty shared_ptr
```

---

## 15. shared_ptr Cycle Problem

Bad:

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::shared_ptr<Node> prev;
};
```

If two nodes point to each other:

```cpp
auto a = std::make_shared<Node>();
auto b = std::make_shared<Node>();

a->next = b;
b->prev = a;
```

Now:

```text
a owns b
b owns a
```

Even if external pointers are destroyed, reference counts never reach zero.

Memory leak.

---

## 16. Fix Cycle with weak_ptr

Use `weak_ptr` for back references / parent pointers / observers.

```cpp
struct Node {
    std::shared_ptr<Node> next;
    std::weak_ptr<Node> prev;
};
```

Now:

```text
next owns forward node
prev observes previous node
```

No ownership cycle.

---

## 17. enable_shared_from_this

Sometimes an object needs to create a `shared_ptr` to itself.

Wrong:

```cpp
std::shared_ptr<T>(this)
```

This creates a separate control block and can cause double delete.

Correct:

```cpp
class Session : public std::enable_shared_from_this<Session> {
public:
    std::shared_ptr<Session> getSelf() {
        return shared_from_this();
    }
};
```

Object must already be owned by a `shared_ptr` before calling `shared_from_this()`.

---

## 18. Do Not Create Two shared_ptr from Same Raw Pointer

Bad:

```cpp
T* raw = new T();

std::shared_ptr<T> p1(raw);
std::shared_ptr<T> p2(raw); // bad
```

This creates two separate control blocks.

Both think they own the object.

Result:

```text
double delete
```

Correct:

```cpp
auto p1 = std::make_shared<T>();
auto p2 = p1;
```

---

## 19. Aliasing Constructor

Advanced but useful to know.

A `shared_ptr` can own one object but point to a subobject.

Example:

```cpp
auto owner = std::make_shared<User>();
std::shared_ptr<std::string> namePtr(owner, &owner->name);
```

This `namePtr` keeps `owner` alive but points to `owner->name`.

Used in advanced APIs.

---

## 20. Raw Pointer vs unique_ptr vs shared_ptr

### Raw pointer

```cpp
T*
```

Usually means:

```text
non-owning
nullable
do not delete unless explicitly documented
```

---

### unique_ptr

```cpp
std::unique_ptr<T>
```

Means:

```text
exclusive ownership
automatic deletion
move-only
```

---

### shared_ptr

```cpp
std::shared_ptr<T>
```

Means:

```text
shared ownership
reference counted
object dies when last owner dies
```

---

### weak_ptr

```cpp
std::weak_ptr<T>
```

Means:

```text
non-owning observer of shared_ptr-managed object
can check whether object still exists
```

---

## 21. Common Interview Questions

### Q1. Difference between unique_ptr and shared_ptr?

`unique_ptr` represents exclusive ownership and is move-only.

`shared_ptr` represents shared ownership using reference counting and can be copied.

---

### Q2. When should you use weak_ptr?

Use `weak_ptr` to observe an object managed by `shared_ptr` without extending its lifetime.

It is commonly used to break shared_ptr cycles.

---

### Q3. Why prefer make_unique / make_shared?

They are clearer and exception-safe.

`make_shared` can also allocate the object and control block in one allocation.

---

### Q4. Why is creating two shared_ptr from the same raw pointer dangerous?

Because each shared_ptr creates its own control block.

Both will try to delete the same object, causing double delete.

---

### Q5. What does shared_ptr copy do?

It copies the pointer and increments the strong reference count in the control block.

When the last shared_ptr is destroyed, the object is deleted.

---

### Q6. How should a function accept smart pointers?

If it takes ownership, accept `unique_ptr<T>` by value.

If it shares ownership, accept `shared_ptr<T>` by value.

If it only observes, accept `T&`, `const T&`, or `T*` depending on nullability.

---

## 22. Key Takeaways

- Smart pointers express ownership.
- `unique_ptr` is default choice for owning heap objects.
- `shared_ptr` only when ownership is genuinely shared.
- `weak_ptr` observes shared_ptr-managed objects without owning.
- Use `make_unique` and `make_shared`.
- Do not return `get()` from local unique_ptr.
- Do not create multiple shared_ptr from the same raw pointer.
- Use raw pointer/reference for non-owning access.
- Avoid shared_ptr cycles by using weak_ptr for back references.
