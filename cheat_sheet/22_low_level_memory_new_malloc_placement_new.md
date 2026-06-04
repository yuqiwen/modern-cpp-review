# 22. Low-level Memory: malloc/free, new/delete, operator new, Placement New

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. `malloc` / `free`](#2-malloc--free)
- [3. `new` Expression](#3-new-expression)
- [4. `delete` Expression](#4-delete-expression)
- [5. `operator new`](#5-operator-new)
- [6. `operator delete`](#6-operator-delete)
- [7. Placement New](#7-placement-new)
- [8. Manual Destructor Call](#8-manual-destructor-call)
- [9. Why `malloc` Alone Is Not Enough for C++ Objects](#9-why-malloc-alone-is-not-enough-for-c-objects)
- [10. `new[]` and `delete[]`](#10-new-and-delete)
- [11. Matching Allocation and Deallocation](#11-matching-allocation-and-deallocation)
- [12. `std::allocator<T>`](#12-stdallocatort)
- [13. `std::construct_at` and Placement New](#13-stdconstruct_at-and-placement-new)
- [14. `std::destroy_at`](#14-stddestroy_at)
- [15. Object Lifetime](#15-object-lifetime)
- [16. Why Assignment Is Not Construction](#16-why-assignment-is-not-construction)
- [17. Custom `operator new` in a Class](#17-custom-operator-new-in-a-class)
- [18. `new` Expression with Custom operator new](#18-new-expression-with-custom-operator-new)
- [19. Placement New Does Not Use Normal Allocation](#19-placement-new-does-not-use-normal-allocation)
- [20. Interview Comparison: malloc vs new](#20-interview-comparison-malloc-vs-new)
- [21. Common Interview Questions](#21-common-interview-questions)
- [22. Key Takeaways](#22-key-takeaways)

## Related Code Trap

- [Low-level Memory Demo](../code_traps/low_level_memory_new_malloc_placement_new.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

In C++, memory and object lifetime are different concepts.

```text
raw memory: bytes of storage
object: a live C++ object whose lifetime has started
```

Allocation gives memory.

Construction creates an object in that memory.

Destruction ends the object's lifetime.

Deallocation releases the memory.

---

## 2. `malloc` / `free`

C-style allocation:

```cpp
void* mem = std::malloc(sizeof(T));
std::free(mem);
```

`malloc` only allocates raw memory.

It does not call constructor.

`free` only releases memory.

It does not call destructor.

Example:

```cpp
void* mem = std::malloc(sizeof(std::string));
```

This gives enough memory for a `std::string`, but no `std::string` object exists yet.

This is not valid:

```cpp
std::string* p = static_cast<std::string*>(mem);
*p = "hello"; // wrong: no string object has been constructed
```

---

## 3. `new` Expression

Normal C++ object creation:

```cpp
T* p = new T(args...);
```

Conceptually does two things:

```text
1. allocate raw memory using operator new
2. construct T object in that memory using T constructor
```

Equivalent mental model:

```cpp
void* mem = ::operator new(sizeof(T));
T* p = new (mem) T(args...);
```

Real implementation details are more complex, but this model is very useful.

---

## 4. `delete` Expression

```cpp
delete p;
```

Conceptually does two things:

```text
1. call destructor: p->~T()
2. release memory using operator delete
```

Equivalent mental model:

```cpp
p->~T();
::operator delete(p);
```

So:

```cpp
new = allocation + construction
delete = destruction + deallocation
```

---

## 5. `operator new`

`operator new` is a low-level allocation function.

```cpp
void* mem = ::operator new(sizeof(T));
```

It only allocates raw memory.

It does not construct an object.

It is more C++-style than `malloc` because it throws `std::bad_alloc` on failure by default.

Comparison:

```text
malloc failure -> returns nullptr
operator new failure -> throws std::bad_alloc
```

---

## 6. `operator delete`

```cpp
::operator delete(mem);
```

Releases raw memory allocated by `::operator new`.

It does not call destructor.

So if an object was constructed there, you must destroy it first:

```cpp
p->~T();
::operator delete(p);
```

---

## 7. Placement New

Placement new constructs an object at an existing memory address.

```cpp
T* p = new (mem) T(args...);
```

It does not allocate memory.

It only constructs.

Example:

```cpp
void* mem = ::operator new(sizeof(T));
T* p = new (mem) T(args...);
```

Here:

```text
operator new allocates memory
placement new constructs T in that memory
```

---

## 8. Manual Destructor Call

If you manually construct with placement new, you must manually destroy:

```cpp
p->~T();
```

Then release raw memory:

```cpp
::operator delete(mem);
```

Full pattern:

```cpp
void* mem = ::operator new(sizeof(T));

try {
    T* p = new (mem) T(args...);

    // use *p

    p->~T();
    ::operator delete(mem);
} catch (...) {
    ::operator delete(mem);
    throw;
}
```

---

## 9. Why `malloc` Alone Is Not Enough for C++ Objects

For trivial types like `int`, this may appear to work:

```cpp
int* p = static_cast<int*>(std::malloc(sizeof(int)));
*p = 10;
std::free(p);
```

But for non-trivial C++ types like `std::string`, this is wrong:

```cpp
std::string* s = static_cast<std::string*>(std::malloc(sizeof(std::string)));
*s = "hello"; // undefined behavior
std::free(s);
```

Because no `std::string` constructor ran.

Correct C-style raw storage + placement new:

```cpp
void* mem = std::malloc(sizeof(std::string));
std::string* s = new (mem) std::string("hello");

s->~basic_string();
std::free(mem);
```

But in real C++ code, prefer:

```cpp
auto s = std::make_unique<std::string>("hello");
```

or normal automatic object:

```cpp
std::string s = "hello";
```

---

## 10. `new[]` and `delete[]`

Array new:

```cpp
T* arr = new T[n];
```

This:

```text
1. allocates memory for n T objects
2. default-constructs n T objects
```

Array delete:

```cpp
delete[] arr;
```

This:

```text
1. calls destructor for each element
2. releases memory
```

Important:

```cpp
T* p = new T;
delete[] p; // wrong
```

and:

```cpp
T* arr = new T[10];
delete arr; // wrong
```

Must match:

```text
new    with delete
new[]  with delete[]
malloc with free
operator new with operator delete
```

---

## 11. Matching Allocation and Deallocation

Correct pairs:

```cpp
T* p = new T;
delete p;
```

```cpp
T* arr = new T[n];
delete[] arr;
```

```cpp
void* mem = std::malloc(size);
std::free(mem);
```

```cpp
void* mem = ::operator new(size);
::operator delete(mem);
```

Wrong pairs cause undefined behavior:

```cpp
T* p = new T;
std::free(p); // wrong
```

```cpp
void* mem = std::malloc(sizeof(T));
T* p = new (mem) T();
delete p; // wrong if memory came from malloc
```

For malloc + placement new:

```cpp
p->~T();
std::free(mem);
```

For operator new + placement new:

```cpp
p->~T();
::operator delete(mem);
```

---

## 12. `std::allocator<T>`

A generic container usually uses allocator:

```cpp
std::allocator<T> alloc;
T* data = alloc.allocate(n);
```

This allocates raw memory for `n` objects.

Then construct:

```cpp
std::construct_at(data + i, args...);
```

Destroy:

```cpp
std::destroy_at(data + i);
```

Deallocate:

```cpp
alloc.deallocate(data, n);
```

This is the standard container style abstraction.

---

## 13. `std::construct_at` and Placement New

C++20:

```cpp
std::construct_at(ptr, args...);
```

Equivalent idea:

```cpp
new (ptr) T(args...);
```

Use `construct_at` when possible because it is clearer and standard-library friendly.

---

## 14. `std::destroy_at`

```cpp
std::destroy_at(ptr);
```

Equivalent idea:

```cpp
ptr->~T();
```

It calls the destructor for the object at `ptr`.

Only call it if an object is alive there.

---

## 15. Object Lifetime

This is the most important part.

```cpp
void* mem = ::operator new(sizeof(T));
```

At this point:

```text
memory exists
T object does not exist
```

Then:

```cpp
T* p = new (mem) T(args...);
```

Now:

```text
T object exists
object lifetime has started
```

Then:

```cpp
p->~T();
```

Now:

```text
T object no longer exists
memory still exists
```

Then:

```cpp
::operator delete(mem);
```

Now:

```text
memory is released
```

---

## 16. Why Assignment Is Not Construction

Wrong:

```cpp
T* p = static_cast<T*>(::operator new(sizeof(T)));
*p = T(args...); // wrong, no object exists at p yet
```

Assignment requires an existing object.

Construction creates an object.

Correct:

```cpp
T* p = static_cast<T*>(::operator new(sizeof(T)));
new (p) T(args...);
```

or:

```cpp
std::construct_at(p, args...);
```

---

## 17. Custom `operator new` in a Class

A class can define its own allocation behavior:

```cpp
class MyClass {
public:
    static void* operator new(std::size_t size) {
        std::cout << "custom operator new\n";
        return ::operator new(size);
    }

    static void operator delete(void* p) noexcept {
        std::cout << "custom operator delete\n";
        ::operator delete(p);
    }
};
```

Then:

```cpp
MyClass* p = new MyClass();
delete p;
```

Uses class-specific allocation/deallocation functions.

Constructor/destructor are still separate.

---

## 18. `new` Expression with Custom operator new

For:

```cpp
MyClass* p = new MyClass();
```

Conceptually:

```text
1. MyClass::operator new(sizeof(MyClass))
2. MyClass constructor
```

For:

```cpp
delete p;
```

Conceptually:

```text
1. MyClass destructor
2. MyClass::operator delete(p)
```

This is useful for memory pools, debugging allocation, custom allocators, etc.

---

## 19. Placement New Does Not Use Normal Allocation

```cpp
T* p = new (mem) T(args...);
```

This uses placement new overload:

```cpp
void* operator new(std::size_t, void* place) noexcept {
    return place;
}
```

It simply returns the provided memory address.

It does not allocate.

---

## 20. Interview Comparison: malloc vs new

### malloc

```text
C library function
allocates raw memory
does not call constructor
returns void*
returns nullptr on failure
must use free
```

### new

```text
C++ expression
allocates memory
calls constructor
returns typed pointer
throws std::bad_alloc on failure
must use delete
```

Interview answer:

```text
malloc/free manage raw memory only.
new/delete manage both memory and object lifetime.
```

---

## 21. Common Interview Questions

### Q1. Difference between `malloc` and `new`?

`malloc` only allocates raw memory and returns `void*`.

`new` allocates memory and constructs an object.

`malloc` returns `nullptr` on failure, while `new` throws `std::bad_alloc` by default.

---

### Q2. Difference between `free` and `delete`?

`free` only releases raw memory.

`delete` calls the object's destructor and then releases memory.

---

### Q3. What is placement new?

Placement new constructs an object at a specific pre-allocated memory address.

It does not allocate memory.

---

### Q4. What is `operator new`?

`operator new` is the low-level allocation function used by the `new` expression to obtain raw memory.

It does not construct the object by itself.

---

### Q5. Why can't we assign into raw memory?

Because assignment requires an already existing object.

Raw memory is just bytes. We must construct an object first using placement new or `std::construct_at`.

---

### Q6. What happens in `T* p = new T(args...)`?

It allocates raw memory using `operator new`, then constructs `T` in that memory by calling the constructor.

---

### Q7. What happens in `delete p`?

It calls the destructor for `*p`, then releases the memory using `operator delete`.

---

## 22. Key Takeaways

- Memory allocation and object construction are separate.
- `malloc` allocates memory only.
- `new` allocates memory and constructs object.
- `free` releases memory only.
- `delete` destroys object and releases memory.
- `operator new` is raw allocation.
- placement new constructs at existing memory.
- Manual placement new requires manual destructor call.
- Assignment requires an existing object.
- Match allocation and deallocation pairs.
- Generic containers use allocator + construct/destroy to manage object lifetime.
