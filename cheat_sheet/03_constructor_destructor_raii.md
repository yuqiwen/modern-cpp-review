# 03. Constructor, Destructor, and RAII

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Constructor](#2-constructor)
- [3. Constructor with Parameters](#3-constructor-with-parameters)
- [4. Member Initializer List](#4-member-initializer-list)
- [5. Initialization Order](#5-initialization-order)
- [6. Destructor](#6-destructor)
- [7. Destructor Should Not Throw](#7-destructor-should-not-throw)
- [8. RAII](#8-raii)
- [9. RAII and Exception Safety](#9-raii-and-exception-safety)
- [10. Stack Unwinding](#10-stack-unwinding)
- [11. Classic RAII Example: File Wrapper](#11-classic-raii-example-file-wrapper)
- [12. Classic RAII Example: Mutex Lock](#12-classic-raii-example-mutex-lock)
- [13. Constructor Failure](#13-constructor-failure)
- [14. Default Constructor](#14-default-constructor)
- [15. Copy Constructor](#15-copy-constructor)
- [16. Move Constructor](#16-move-constructor)
- [17. Common Interview Questions](#17-common-interview-questions)
- [18. Key Takeaways](#18-key-takeaways)

## Related Code Trap

- [Constructor / Destructor / RAII Demo](../code_traps/constructor_destructor_raii.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

In C++, object lifetime is directly connected to resource management.

A constructor is responsible for creating a valid object state.

A destructor is responsible for cleaning up resources when the object lifetime ends.

RAII stands for:

```text
Resource Acquisition Is Initialization
```

The core idea is:

- Acquire a resource in the constructor.
- Release the resource in the destructor.
- Let object lifetime control resource lifetime.

This is one of the most important ideas in modern C++.

---

## 2. Constructor

A constructor is called when an object is created.

Example:

```cpp
class User {
public:
    User() {
        // constructor body
    }
};
```

Usage:

```cpp
User u;
```

The constructor initializes the object.

---

## 3. Constructor with Parameters

```cpp
class User {
private:
    std::string name;
    int age;

public:
    User(const std::string& n, int a)
        : name(n), age(a) {}
};
```

The part after `:` is the member initializer list.

```cpp
: name(n), age(a)
```

This initializes members before the constructor body runs.

---

## 4. Member Initializer List

Prefer this:

```cpp
class User {
private:
    std::string name;

public:
    User(const std::string& n)
        : name(n) {}
};
```

Instead of this:

```cpp
class User {
private:
    std::string name;

public:
    User(const std::string& n) {
        name = n;
    }
};
```

Why?

The first version directly initializes `name`.

The second version default-constructs `name` first, then assigns to it.

For class members, initializer lists are usually preferred.

---

## 5. Initialization Order

Members are initialized in the order they are declared in the class, not the order in the initializer list.

Example:

```cpp
class Example {
private:
    int a;
    int b;

public:
    Example() : b(2), a(b + 1) {}
};
```

This is dangerous.

Even though `b(2)` appears first in the initializer list, `a` is initialized first because `a` is declared first.

So `a(b + 1)` uses `b` before `b` is initialized.

Correct version:

```cpp
class Example {
private:
    int b;
    int a;

public:
    Example() : b(2), a(b + 1) {}
};
```

Or simpler:

```cpp
class Example {
private:
    int a;
    int b;

public:
    Example() : a(3), b(2) {}
};
```

Interview reminder:

```text
Member initialization order follows declaration order, not initializer-list order.
```

---

## 6. Destructor

A destructor is called when an object is destroyed.

```cpp
class User {
public:
    ~User() {
        // cleanup
    }
};
```

For automatic objects:

```cpp
void f() {
    User u;
} // destructor runs here
```

For dynamic objects:

```cpp
User* p = new User();
delete p; // destructor runs here
```

For smart pointers:

```cpp
auto p = std::make_unique<User>();
```

When `p` leaves scope, `unique_ptr` destroys the managed `User`.

---

## 7. Destructor Should Not Throw

A destructor should generally not throw exceptions.

Bad:

```cpp
class Bad {
public:
    ~Bad() {
        throw std::runtime_error("error");
    }
};
```

Why dangerous?

If an exception is already being handled and another exception escapes from a destructor, the program may call `std::terminate`.

Interview answer:

```text
Destructors should not throw because they are often called during stack unwinding. If a destructor throws while another exception is active, the program can terminate.
```

---

## 8. RAII

RAII means binding resource lifetime to object lifetime.

Example resource types:

- heap memory
- file handles
- mutex locks
- sockets
- database connections
- GPU buffers
- thread handles

Without RAII:

```cpp
void f() {
    int* p = new int(10);

    if (someCondition()) {
        return; // memory leak
    }

    delete p;
}
```

With RAII:

```cpp
void f() {
    auto p = std::make_unique<int>(10);

    if (someCondition()) {
        return; // safe
    }
}
```

When `p` leaves scope, the memory is released automatically.

---

## 9. RAII and Exception Safety

Without RAII:

```cpp
void f() {
    Resource* r = new Resource();

    doSomethingThatMayThrow();

    delete r;
}
```

If `doSomethingThatMayThrow()` throws, `delete r` is skipped.

With RAII:

```cpp
void f() {
    ResourceWrapper r;

    doSomethingThatMayThrow();
}
```

Even if an exception is thrown, `r`'s destructor runs during stack unwinding.

So the resource is still released.

---

## 10. Stack Unwinding

When an exception is thrown, C++ destroys local objects as it exits scopes.

Example:

```cpp
void f() {
    std::string s = "hello";
    throw std::runtime_error("error");
}
```

Before leaving `f`, the destructor of `s` runs.

This process is called stack unwinding.

RAII depends on this behavior.

---

## 11. Classic RAII Example: File Wrapper

```cpp
class File {
private:
    FILE* file;

public:
    File(const char* path, const char* mode) {
        file = std::fopen(path, mode);
        if (!file) {
            throw std::runtime_error("failed to open file");
        }
    }

    ~File() {
        if (file) {
            std::fclose(file);
        }
    }
};
```

Now the file is automatically closed when the `File` object is destroyed.

---

## 12. Classic RAII Example: Mutex Lock

Without RAII:

```cpp
mutex.lock();
// critical section
mutex.unlock();
```

Problem:

If an exception happens before `unlock`, the mutex stays locked.

With RAII:

```cpp
std::lock_guard<std::mutex> lock(mutex);
// critical section
```

When `lock` leaves scope, its destructor unlocks the mutex automatically.

---

## 13. Constructor Failure

If a constructor throws, the object is considered not fully constructed.

Its destructor will not run.

However, fully constructed member objects will be destroyed.

Example:

```cpp
class A {
public:
    A() {
        std::cout << "A constructor\n";
    }

    ~A() {
        std::cout << "A destructor\n";
    }
};

class B {
private:
    A a;

public:
    B() {
        throw std::runtime_error("B constructor failed");
    }

    ~B() {
        std::cout << "B destructor\n";
    }
};
```

If `B` constructor throws:

- `A` destructor runs because `a` was fully constructed.
- `B` destructor does not run because `B` was not fully constructed.

Interview reminder:

```text
If a constructor throws, the destructor of that object is not called, but destructors for fully constructed members and base classes are called.
```

---

## 14. Default Constructor

A default constructor can be called without arguments.

```cpp
class User {
public:
    User() {}
};
```

Usage:

```cpp
User u;
```

If no constructor is defined, C++ may generate a default constructor automatically.

But if you define a parameterized constructor, the compiler will not automatically generate a default constructor.

Example:

```cpp
class User {
public:
    User(const std::string& name) {}
};

User u; // error: no default constructor
```

Fix:

```cpp
class User {
public:
    User() = default;
    User(const std::string& name) {}
};
```

---

## 15. Copy Constructor

A copy constructor creates a new object from an existing object.

```cpp
class User {
public:
    User(const User& other) {
        // copy from other
    }
};
```

Usage:

```cpp
User a;
User b = a;
```

This will be studied in detail in copy/move semantics.

---

## 16. Move Constructor

A move constructor creates a new object by taking resources from a temporary or movable object.

```cpp
class User {
public:
    User(User&& other) noexcept {
        // move from other
    }
};
```

Usage:

```cpp
User b = std::move(a);
```

This will be studied in detail later.

---

## 17. Common Interview Questions

### Q1. What is RAII?

RAII means Resource Acquisition Is Initialization.

It binds resource lifetime to object lifetime.

A resource is acquired in the constructor and released in the destructor, so cleanup happens automatically when the object leaves scope.

---

### Q2. Why is RAII useful?

RAII prevents resource leaks and makes code exception-safe.

Even if a function returns early or throws an exception, destructors of local objects still run, so resources are released automatically.

---

### Q3. Why should destructors usually not throw?

Destructors may run during stack unwinding.

If a destructor throws while another exception is already active, the program can terminate.

So destructors should generally handle errors internally and not allow exceptions to escape.

---

### Q4. What happens if a constructor throws?

The object is not fully constructed, so its destructor is not called.

However, destructors for fully constructed members and base classes are called.

---

### Q5. Why prefer member initializer lists?

Member initializer lists directly initialize members.

Assignment inside the constructor body first default-constructs the member, then assigns to it.

Initializer lists are also required for `const` members, reference members, and members without default constructors.

---

## 18. Key Takeaways

- Constructor creates a valid object state.
- Destructor cleans up when object lifetime ends.
- RAII binds resource lifetime to object lifetime.
- Prefer member initializer lists.
- Member initialization order follows declaration order.
- Destructors should generally not throw.
- RAII makes early return and exception paths safe.
- If a constructor throws, the object destructor does not run, but fully constructed members are destroyed.
