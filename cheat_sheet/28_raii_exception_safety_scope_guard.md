# 28. RAII, Exception Safety, and Scope Guard

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Why RAII Matters](#2-why-raii-matters)
- [3. Stack Unwinding](#3-stack-unwinding)
- [4. Destructor During Exception](#4-destructor-during-exception)
- [5. unique_ptr as RAII](#5-unique_ptr-as-raii)
- [6. lock_guard as RAII](#6-lock_guard-as-raii)
- [7. fstream as RAII](#7-fstream-as-raii)
- [8. Basic RAII Wrapper](#8-basic-raii-wrapper)
- [9. Why Delete Copy?](#9-why-delete-copy)
- [10. Why Move Transfers Ownership?](#10-why-move-transfers-ownership)
- [11. Scope-based Cleanup](#11-scope-based-cleanup)
- [12. Simple ScopeGuard](#12-simple-scopeguard)
- [13. Exception Safety Guarantees](#13-exception-safety-guarantees)
- [14. Copy-and-Swap and Strong Guarantee](#14-copy-and-swap-and-strong-guarantee)
- [15. RAII and Generic Containers](#15-raii-and-generic-containers)
- [16. RAII vs finally](#16-raii-vs-finally)
- [17. Common Bad Pattern: Manual Cleanup](#17-common-bad-pattern-manual-cleanup)
- [18. Common Bad Pattern: Returning Raw Owning Pointer](#18-common-bad-pattern-returning-raw-owning-pointer)
- [19. Common Bad Pattern: Lock/Unlock Manually](#19-common-bad-pattern-lockunlock-manually)
- [20. Common Interview Questions](#20-common-interview-questions)
- [21. Key Takeaways](#21-key-takeaways)

## Related Code Trap

- [RAII / Exception Safety Demo](../code_traps/raii_exception_safety.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

RAII stands for:

```text
Resource Acquisition Is Initialization
```

The idea:

```text
constructor acquires resource
destructor releases resource
```

Resource can mean:

- heap memory
- file handle
- mutex lock
- socket
- database connection
- GPU buffer
- temporary state change

RAII makes resource cleanup automatic, especially when exceptions happen.

---

## 2. Why RAII Matters

Without RAII:

```cpp
void f() {
    Resource* r = acquire();

    doSomething();

    release(r);
}
```

Problem:

```cpp
doSomething();
```

may throw.

Then:

```cpp
release(r);
```

is skipped.

Resource leaks.

With RAII:

```cpp
void f() {
    ResourceOwner r;

    doSomething();
}
```

If `doSomething()` throws, stack unwinding destroys `r`, and its destructor releases the resource.

---

## 3. Stack Unwinding

When an exception is thrown, C++ unwinds the stack.

That means:

```text
local objects whose lifetime has started are destroyed in reverse order
```

Example:

```cpp
void f() {
    A a;
    B b;

    throw std::runtime_error("error");
}
```

When exception leaves `f`:

```text
b destructor runs
a destructor runs
```

This is why RAII works.

---

## 4. Destructor During Exception

Destructors are called during stack unwinding.

Therefore destructors should generally not throw.

Bad:

```cpp
class Bad {
public:
    ~Bad() {
        throw std::runtime_error("destructor error");
    }
};
```

If another exception is already being processed and destructor throws, the program may call:

```cpp
std::terminate()
```

Rule:

```text
destructors should be noexcept or should catch their own exceptions
```

---

## 5. unique_ptr as RAII

```cpp
void f() {
    auto p = std::make_unique<int>(42);

    mayThrow();
}
```

If `mayThrow()` throws:

```text
p destructor runs
owned int is deleted
```

No memory leak.

This is why smart pointers are safer than raw `new/delete`.

---

## 6. lock_guard as RAII

Manual locking is dangerous:

```cpp
mutex.lock();
mayThrow();
mutex.unlock(); // skipped if exception thrown
```

RAII version:

```cpp
std::lock_guard<std::mutex> lock(mutex);
mayThrow();
```

When scope exits, `lock` destructor unlocks the mutex.

Even if exception happens.

---

## 7. fstream as RAII

```cpp
std::ifstream file("data.txt");

if (!file) {
    throw std::runtime_error("open failed");
}
```

When `file` goes out of scope, file handle closes automatically.

No manual `close()` needed in most cases.

---

## 8. Basic RAII Wrapper

Example: wrapper around C `FILE*`.

```cpp
class File {
private:
    FILE* fp = nullptr;

public:
    explicit File(const char* path)
        : fp(std::fopen(path, "r")) {
        if (!fp) {
            throw std::runtime_error("failed to open file");
        }
    }

    ~File() {
        if (fp) {
            std::fclose(fp);
        }
    }

    File(const File&) = delete;
    File& operator=(const File&) = delete;

    File(File&& other) noexcept
        : fp(other.fp) {
        other.fp = nullptr;
    }

    File& operator=(File&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        if (fp) {
            std::fclose(fp);
        }

        fp = other.fp;
        other.fp = nullptr;

        return *this;
    }

    FILE* get() const {
        return fp;
    }
};
```

This class owns a `FILE*`.

It is move-only, like `unique_ptr`.

---

## 9. Why Delete Copy?

For resource owners:

```cpp
File(const File&) = delete;
File& operator=(const File&) = delete;
```

Why?

If two `File` objects copied the same raw `FILE*`, both destructors would call `fclose`.

Double close bug.

So ownership should be either:

```text
unique ownership -> delete copy, allow move
shared ownership -> use reference counting/shared ownership design
```

For file handles, unique ownership is usually best.

---

## 10. Why Move Transfers Ownership?

Move constructor:

```cpp
File(File&& other) noexcept
    : fp(other.fp) {
    other.fp = nullptr;
}
```

Meaning:

```text
new object takes the resource
old object becomes empty
```

Then old object's destructor is safe because `fp == nullptr`.

---

## 11. Scope-based Cleanup

Sometimes you need cleanup code at scope exit.

Example:

```cpp
beginTransaction();

try {
    doWork();
    commit();
} catch (...) {
    rollback();
    throw;
}
```

A scope guard can make this RAII.

Conceptually:

```cpp
auto guard = makeScopeGuard([] {
    rollback();
});

doWork();

commit();
guard.dismiss();
```

If exception happens before `dismiss`, rollback runs automatically.

---

## 12. Simple ScopeGuard

```cpp
template <typename F>
class ScopeGuard {
private:
    F func;
    bool active = true;

public:
    explicit ScopeGuard(F f)
        : func(std::move(f)) {}

    ~ScopeGuard() {
        if (active) {
            func();
        }
    }

    void dismiss() {
        active = false;
    }

    ScopeGuard(const ScopeGuard&) = delete;
    ScopeGuard& operator=(const ScopeGuard&) = delete;

    ScopeGuard(ScopeGuard&& other) noexcept
        : func(std::move(other.func)),
          active(other.active) {
        other.active = false;
    }
};
```

Usage:

```cpp
auto guard = ScopeGuard([&] {
    rollback();
});

doWork();

commit();
guard.dismiss();
```

---

## 13. Exception Safety Guarantees

C++ often discusses three levels.

### No-throw guarantee

Operation never throws.

Example:

```cpp
~ResourceOwner() noexcept;
swap(a, b) noexcept;
move constructor noexcept;
```

---

### Strong guarantee

Operation either:

```text
succeeds completely
or leaves object unchanged
```

Example:

```cpp
copy-and-swap assignment
vector reallocation when copy/move transfer succeeds safely
```

---

### Basic guarantee

If operation throws:

```text
object remains valid and destructible
but value may have changed
```

This is weaker than strong guarantee.

---

## 14. Copy-and-Swap and Strong Guarantee

```cpp
MyClass& operator=(const MyClass& other) {
    MyClass temp(other);
    swap(temp);
    return *this;
}
```

Why strong guarantee?

```text
1. temp copy happens first
2. if copy throws, *this unchanged
3. if copy succeeds, swap is noexcept
4. temp destructor cleans old resource
```

---

## 15. RAII and Generic Containers

A vector-like container uses RAII internally.

It owns:

```text
raw storage
constructed elements
```

Its destructor releases everything.

If reallocation throws, it must clean up partially constructed new storage.

This is exception safety.

RAII is the broader principle behind that cleanup.

---

## 16. RAII vs finally

Some languages use `finally`.

C++ uses destructors.

Example:

```cpp
{
    std::lock_guard<std::mutex> lock(m);
    criticalSection();
} // unlock automatically here
```

The destructor is the cleanup hook.

---

## 17. Common Bad Pattern: Manual Cleanup

Bad:

```cpp
void f() {
    T* p = new T();

    mayThrow();

    delete p;
}
```

If `mayThrow()` throws, `delete p` is skipped.

Good:

```cpp
void f() {
    auto p = std::make_unique<T>();

    mayThrow();
}
```

---

## 18. Common Bad Pattern: Returning Raw Owning Pointer

Bad:

```cpp
T* makeT() {
    return new T();
}
```

Caller must remember to delete.

Good:

```cpp
std::unique_ptr<T> makeT() {
    return std::make_unique<T>();
}
```

Ownership is explicit.

---

## 19. Common Bad Pattern: Lock/Unlock Manually

Bad:

```cpp
m.lock();

if (condition) {
    return; // unlock skipped
}

m.unlock();
```

Good:

```cpp
std::lock_guard<std::mutex> lock(m);

if (condition) {
    return; // destructor unlocks
}
```

---

## 20. Common Interview Questions

### Q1. What is RAII?

RAII means resource acquisition is tied to object initialization, and resource release is tied to object destruction.

The constructor acquires the resource, and the destructor releases it.

---

### Q2. Why is RAII useful with exceptions?

Because when an exception is thrown, stack unwinding destroys local objects.

RAII destructors run automatically and release resources.

---

### Q3. Why should destructors not throw?

Destructors may run during stack unwinding.

If a destructor throws while another exception is active, the program may terminate.

---

### Q4. Why is lock_guard RAII?

`lock_guard` locks a mutex in its constructor and unlocks it in its destructor.

This guarantees the mutex is unlocked when scope exits.

---

### Q5. Why delete copy for a resource owner?

Copying a raw resource owner could create two owners for the same resource, leading to double free or double close.

Use move semantics for unique ownership.

---

### Q6. What is strong exception guarantee?

An operation either succeeds completely or leaves the object unchanged.

---

## 21. Key Takeaways

- RAII ties resource lifetime to object lifetime.
- Constructors acquire; destructors release.
- Stack unwinding calls destructors.
- Destructors should not throw.
- Use smart pointers for memory RAII.
- Use lock_guard/unique_lock for mutex RAII.
- Use fstream or wrappers for file RAII.
- Resource owners usually delete copy and support move.
- Exception safety levels: no-throw, strong, basic.
- Prefer RAII over manual cleanup.
