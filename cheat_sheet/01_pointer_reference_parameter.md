# 01. Pointer, Reference, and Parameter Passing

## 1. Core Idea

In C++, a variable name is used to access an object.

```cpp
int x = 10;
```

Here:

- `x` is the variable name.
- The `int` object stores the value `10`.
- `x` gives us access to that object.

A pointer is an object that stores an address.

A reference is an alias bound to an existing object.

Pointers can be null and reassigned.  
References must be initialized and cannot be reseated.

---

## 2. Pointer

```cpp
int x = 10;
int* p = &x;
```

`p` is a pointer object. It stores the address of `x`.

```cpp
*p = 20;
```

This modifies the object pointed to by `p`, so `x` becomes `20`.

### Key Properties

- A pointer stores an address.
- A pointer itself is also an object.
- A pointer can be `nullptr`.
- A pointer can be reassigned to point to another object.
- Use a pointer when nullability or address-level semantics matter.

Example:

```cpp
int a = 10;
int b = 20;

int* p = &a;
p = &b; // OK, p now points to b
```

---

## 3. Reference

```cpp
int x = 10;
int& r = x;
```

`r` is an alias of `x`.

```cpp
r = 20;
```

This modifies `x`.

### Key Properties

- A reference is an alias to an existing object.
- A reference must be initialized.
- A reference cannot be reseated.
- A reference can still dangle if the original object is destroyed.

---

## 4. Common Trap: Reference Assignment

```cpp
int a = 10;
int b = 20;

int& r = a;
r = b;
```

This does **not** make `r` refer to `b`.

It means:

```cpp
a = b;
```

After this:

```cpp
a == 20;
b == 20;
```

`r` is still an alias of `a`.

### Interview Reminder

A pointer can be reseated.  
A reference cannot be reseated.

```cpp
int* p = &a;
p = &b; // reseat pointer, OK

int& r = a;
r = b;  // assign b's value to a, not reseat
```

---

## 5. Pointer vs Reference Interview Answer

### Q: What is the difference between pointer and reference?

A pointer is an object that stores an address. It can be null and can be reassigned to point to another object.

A reference is an alias to an existing object. It must be initialized and cannot be reseated after binding.

In normal usage, a reference means the object must exist, while a pointer can represent an optional or nullable object.

---

## 6. Parameter Passing

C++ function parameters usually express one of these meanings:

```cpp
void f(T x);                  // copy
void f(const T& x);            // read-only input, avoid copy
void f(T& x);                  // modify caller's object
void f(T* x);                  // nullable or address-level parameter
void f(T&& x);                 // move or forwarding
void f(std::unique_ptr<T> x);  // transfer ownership
void f(std::shared_ptr<T> x);  // share ownership
```

---

## 7. Pass by Value

```cpp
void f(T x);
```

Use pass by value when:

- `T` is small, such as `int`, `double`, pointer, enum.
- The function needs its own copy.
- Copying is cheap or intentional.

Example:

```cpp
void changeByValue(int x) {
    x = 100;
}

int a = 10;
changeByValue(a);
```

After the call:

```cpp
a == 10;
```

The caller's object is not modified because `x` is a copy.

### Important Note

For small trivially copyable types, pass by value is usually preferred.

So this is usually fine:

```cpp
void f(int x);
void f(double x);
void f(bool flag);
```

This is usually unnecessary:

```cpp
void f(const int& x);
```

---

## 8. Pass by Const Reference

```cpp
void f(const T& x);
```

Use pass by const reference when:

- `T` is large.
- The function only reads the object.
- You want to avoid copying.

Example:

```cpp
void print(const std::vector<int>& nums);
void logMessage(const std::string& msg);
```

This means:

- Do not copy the object.
- Do not modify the object.

### Interview Answer

For large read-only objects, I prefer `const T&` because it avoids copying while guaranteeing that the function will not modify the object.

---

## 9. Pass by Non-const Reference

```cpp
void f(T& x);
```

Use pass by non-const reference when:

- The function needs to modify the caller's object.
- The object must exist.

Example:

```cpp
void normalize(std::vector<int>& nums) {
    for (int& x : nums) {
        if (x < 0) {
            x = -x;
        }
    }
}
```

A non-const reference parameter usually means the function may modify the caller's object.

---

## 10. Pass by Pointer

```cpp
void f(T* x);
```

Use pointer parameters when:

- The argument may be `nullptr`.
- You want to express address-level semantics.
- The function may need to check whether the object exists.

Example:

```cpp
void update(User* user) {
    if (user) {
        // modify user
    }
}
```

### Reference vs Pointer Parameter

```cpp
void f(User& user); // user must exist
void f(User* user); // user may be nullptr
```

---

## 11. Pass by Rvalue Reference

```cpp
void f(T&& x);
```

Use rvalue reference parameters when:

- The function accepts temporary objects.
- The function may move from the argument.
- You are implementing move semantics or perfect forwarding.

Example:

```cpp
void setName(std::string&& name);
```

This topic will be reviewed in detail later when studying move semantics.

---

## 12. Ownership Parameters

### Transfer Ownership

```cpp
void f(std::unique_ptr<T> p);
```

This means the function takes ownership.

The caller usually passes it with `std::move`:

```cpp
std::unique_ptr<T> ptr = std::make_unique<T>();
f(std::move(ptr));
```

After this, `ptr` no longer owns the object.

---

### Share Ownership

```cpp
void f(std::shared_ptr<T> p);
```

This means the function shares ownership.

But if the function only observes the object, prefer:

```cpp
void f(const T& x);
```

Do not use `shared_ptr` just to avoid copying.  
Use it only when shared lifetime ownership is part of the design.

---

## 13. Const Value Parameter

This exists:

```cpp
void f(const int x);
void f(const std::string s);
```

But for public API semantics, it is usually not important.

Why?

Because pass by value already creates a copy. The caller's object will not be modified anyway.

```cpp
void f(int x) {
    x = 100; // only modifies local copy
}
```

Adding `const` only prevents the function body from modifying its local copy:

```cpp
void f(const int x) {
    // x = 100; // error
}
```

This is mostly an implementation-style choice, not an important API-level distinction.

---

## 14. Temporary Lifetime and Const Reference

A `const T&` can bind to a temporary object.

```cpp
const std::string& s = std::string("hello");
```

Here, the temporary string's lifetime is extended to the lifetime of `s`.

Function parameters also work:

```cpp
void print(const std::string& s) {
    std::cout << s << std::endl;
}

print(std::string("hello"));
```

The temporary lives until the end of the function call.

---

## 15. Dangling Pointer and Dangling Reference

### Dangling Pointer

```cpp
int* bad() {
    int x = 10;
    return &x;
}
```

This is wrong.

`x` is a local object. Its lifetime ends when the function returns.

The returned pointer points to an object that no longer exists.

Accessing it is undefined behavior.

---

### Dangling Reference

```cpp
int& bad() {
    int x = 10;
    return x;
}
```

This is also wrong.

The returned reference refers to a destroyed local object.

---

### Returning Const Reference to Temporary

```cpp
const std::string& bad() {
    return std::string("hello");
}
```

This is wrong.

The temporary string is destroyed after the return expression.  
The returned reference is dangling.

---

## 16. Practice Example

```cpp
#include <iostream>
using namespace std;

void changeByValue(int x) {
    x = 100;
}

void changeByRef(int& x) {
    x = 200;
}

void changeByPtr(int* x) {
    *x = 300;
}

int main() {
    int a = 10;
    int b = 20;

    int& r = a;
    int* p = &a;

    r = b;
    p = &b;

    changeByValue(a);
    changeByRef(a);
    changeByPtr(p);

    cout << a << " " << b << endl;
}
```

Output:

```text
200 300
```

Explanation:

```cpp
int& r = a;
r = b;
```

This means:

```cpp
a = b;
```

So `a` becomes `20`.

Then:

```cpp
p = &b;
```

Now `p` points to `b`.

Then:

```cpp
changeByValue(a);
```

This does not modify `a`.

```cpp
changeByRef(a);
```

This modifies `a` to `200`.

```cpp
changeByPtr(p);
```

Since `p` points to `b`, this modifies `b` to `300`.

Final result:

```text
a = 200
b = 300
```

---

## 17. Parameter Selection Table

```cpp
void f(T x);                  // small object or need a copy
void f(const T& x);            // large read-only input
void f(T& x);                  // modify caller's object
void f(T* x);                  // nullable or address-level parameter
void f(T&& x);                 // move or forwarding
void f(std::unique_ptr<T> x);  // transfer ownership
void f(std::shared_ptr<T> x);  // share ownership
```

---

## 18. Common Interview Questions

### Q1. What is the difference between pointer and reference?

A pointer is an object that stores an address. It can be null and can be reassigned.

A reference is an alias to an existing object. It must be initialized and cannot be reseated.

---

### Q2. When would you use pointer instead of reference?

I would use a reference when the object must exist and the function does not need nullability.

I would use a pointer when `nullptr` is a valid state or when I want to express address-level semantics.

For ownership, I would prefer smart pointers instead of raw pointers.

---

### Q3. When should you use `const T&`?

I use `const T&` for large read-only input objects. It avoids copying and prevents accidental modification.

For small types like `int`, `double`, pointers, or enums, pass by value is usually fine.

---

### Q4. Can a reference dangle?

Yes.

A reference can dangle if it refers to an object whose lifetime has ended.

Example:

```cpp
int& bad() {
    int x = 10;
    return x;
}
```

`x` is destroyed when the function returns, so the returned reference is dangling.

---

### Q5. Does `int& r = a; r = b;` rebind `r` to `b`?

No.

It assigns the value of `b` to `a`.

The reference `r` is still bound to `a`.

---

## 19. Key Takeaways

- Pointer: address object, nullable, reseatable.
- Reference: alias, must be initialized, cannot reseat.
- `T x`: copy.
- `const T& x`: read-only input, avoids copy.
- `T& x`: modifies caller's object.
- `T* x`: nullable or address-level semantics.
- `T&& x`: move or forwarding.
- `unique_ptr<T>` parameter: transfer ownership.
- `shared_ptr<T>` parameter: share ownership.
- Returning pointer/reference to local objects causes dangling.
- `const T&` can extend temporary lifetime locally, but returning a const reference to a temporary is still wrong.