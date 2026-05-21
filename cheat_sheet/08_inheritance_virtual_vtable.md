# 08. Inheritance, Virtual Functions, VTable, and Virtual Destructor

## Table of Contents

- [Related Code Trap](#related-code-trap)
- [1. Core Idea](#1-core-idea)
- [2. Static Type vs Dynamic Type](#2-static-type-vs-dynamic-type)
- [3. Non-virtual Function](#3-non-virtual-function)
- [4. Virtual Function](#4-virtual-function)
- [5. `override`](#5-override)
- [6. Pure Virtual Function and Abstract Class](#6-pure-virtual-function-and-abstract-class)
- [7. Interface-like Base Class](#7-interface-like-base-class)
- [8. Virtual Destructor](#8-virtual-destructor)
- [9. Destructor Order](#9-destructor-order)
- [10. Why Base Destructor Should Be Virtual for Polymorphism](#10-why-base-destructor-should-be-virtual-for-polymorphism)
- [11. VTable and VPtr: Conceptual Model](#11-vtable-and-vptr-conceptual-model)
- [12. Virtual Call Cost](#12-virtual-call-cost)
- [13. Object Slicing](#13-object-slicing)
- [14. Virtual Functions in Constructors and Destructors](#14-virtual-functions-in-constructors-and-destructors)
- [15. Access Control: public / protected / private](#15-access-control-public--protected--private)
- [16. Public Inheritance Means "is-a"](#16-public-inheritance-means-is-a)
- [17. Composition vs Inheritance](#17-composition-vs-inheritance)
- [18. `final`](#18-final)
- [19. Common Interview Questions](#19-common-interview-questions)
- [20. Key Takeaways](#20-key-takeaways)

## Related Code Trap

- [Inheritance / Virtual / VTable Demo](../code_traps/inheritance_virtual_vtable.cpp)
- [Code Traps Index](../code_traps/README.md)

## 1. Core Idea

Inheritance allows a derived class to reuse and extend a base class.

Virtual functions enable runtime polymorphism.

Example:

```cpp
class Base {
public:
    virtual void speak() const {
        std::cout << "Base\n";
    }
};

class Derived : public Base {
public:
    void speak() const override {
        std::cout << "Derived\n";
    }
};
```

Usage:

```cpp
Base* p = new Derived();
p->speak(); // Derived
delete p;
```

Because `speak()` is virtual, the call is resolved based on the dynamic type of the object.

---

## 2. Static Type vs Dynamic Type

Example:

```cpp
Derived d;
Base* p = &d;
```

Here:

```text
static type of p: Base*
dynamic type of *p: Derived
```

The static type is what the compiler sees from the variable declaration.

The dynamic type is the real runtime type of the object.

Virtual dispatch uses the dynamic type.

Non-virtual function calls use the static type.

---

## 3. Non-virtual Function

```cpp
class Base {
public:
    void speak() const {
        std::cout << "Base\n";
    }
};

class Derived : public Base {
public:
    void speak() const {
        std::cout << "Derived\n";
    }
};
```

Usage:

```cpp
Derived d;
Base* p = &d;

p->speak(); // Base
```

Because `speak()` is not virtual, the call is resolved at compile time using the static type `Base*`.

This is not runtime polymorphism.

---

## 4. Virtual Function

```cpp
class Base {
public:
    virtual void speak() const {
        std::cout << "Base\n";
    }
};

class Derived : public Base {
public:
    void speak() const override {
        std::cout << "Derived\n";
    }
};
```

Usage:

```cpp
Derived d;
Base* p = &d;

p->speak(); // Derived
```

Because `speak()` is virtual, C++ uses dynamic dispatch.

The actual function is chosen based on the real object type.

---

## 5. `override`

Always use `override` when overriding virtual functions.

```cpp
class Derived : public Base {
public:
    void speak() const override {
        std::cout << "Derived\n";
    }
};
```

Why?

It lets the compiler check that this function really overrides a base virtual function.

Without `override`, a small signature mismatch may silently create a new function instead of overriding.

Bad example:

```cpp
class Base {
public:
    virtual void speak() const {}
};

class Derived : public Base {
public:
    void speak() {} // missing const, does not override
};
```

With `override`:

```cpp
void speak() override {}
```

The compiler reports an error because the signature does not match.

---

## 6. Pure Virtual Function and Abstract Class

A pure virtual function has `= 0`.

```cpp
class Shape {
public:
    virtual double area() const = 0;
};
```

A class with at least one pure virtual function is an abstract class.

You cannot instantiate it:

```cpp
Shape s; // error
```

But you can use pointers or references:

```cpp
void printArea(const Shape& shape) {
    std::cout << shape.area() << std::endl;
}
```

Derived classes must implement the pure virtual function to become concrete.

---

## 7. Interface-like Base Class

Example:

```cpp
class Shape {
public:
    virtual ~Shape() = default;

    virtual double area() const = 0;
    virtual void draw() const = 0;
};
```

This is common C++ interface style.

Important:

```cpp
virtual ~Shape() = default;
```

If a class is intended to be used polymorphically, its destructor should usually be virtual.

---

## 8. Virtual Destructor

This is one of the most important C++ interview topics.

Bad:

```cpp
class Base {
public:
    ~Base() {
        std::cout << "Base destructor\n";
    }
};

class Derived : public Base {
private:
    int* data;

public:
    Derived() : data(new int[10]) {}

    ~Derived() {
        delete[] data;
        std::cout << "Derived destructor\n";
    }
};
```

Usage:

```cpp
Base* p = new Derived();
delete p; // undefined behavior if Base destructor is not virtual
```

If `Base` destructor is not virtual, deleting a derived object through a base pointer may not call the derived destructor.

This can cause resource leaks or undefined behavior.

Correct:

```cpp
class Base {
public:
    virtual ~Base() {
        std::cout << "Base destructor\n";
    }
};
```

Then:

```cpp
Base* p = new Derived();
delete p;
```

Destructor order:

```text
Derived destructor
Base destructor
```

---

## 9. Destructor Order

For normal object destruction:

```cpp
class Base {
public:
    ~Base() {
        std::cout << "Base destructor\n";
    }
};

class Derived : public Base {
public:
    ~Derived() {
        std::cout << "Derived destructor\n";
    }
};
```

When a `Derived` object is destroyed:

```text
Derived destructor body runs first
Base destructor runs after
```

Construction order is opposite:

```text
Base constructor first
Derived constructor second
```

Destruction order:

```text
Derived destructor first
Base destructor second
```

---

## 10. Why Base Destructor Should Be Virtual for Polymorphism

If a class has virtual functions and is meant to be used through base pointers or references, the base destructor should usually be virtual.

Example:

```cpp
class Animal {
public:
    virtual ~Animal() = default;
    virtual void speak() const = 0;
};
```

This ensures:

```cpp
Animal* a = new Dog();
delete a;
```

correctly destroys the `Dog` part and then the `Animal` part.

Interview answer:

```text
If a base class is intended for polymorphic use, its destructor should be virtual so deleting through a base pointer destroys the entire derived object correctly.
```

---

## 11. VTable and VPtr: Conceptual Model

For a class with virtual functions, the compiler usually adds a hidden pointer inside each object.

This hidden pointer is often called:

```text
vptr
```

It points to a table of virtual functions:

```text
vtable
```

Conceptually:

```text
Derived object:
+----------------+
| vptr ----------+----> Derived vtable
| Base fields    |
| Derived fields |
+----------------+
```

When calling:

```cpp
p->speak();
```

C++ roughly does:

```text
look at object's vptr
find speak() in vtable
call the function pointer
```

This is why virtual calls are resolved at runtime.

---

## 12. Virtual Call Cost

Virtual calls usually have small overhead:

- one extra indirection through vptr/vtable
- harder for compiler to inline in some cases

But the cost is usually small unless in very hot performance-critical code.

For quant / systems / performance interviews, mention:

```text
Virtual functions trade some runtime overhead for runtime polymorphism and extensibility.
```

---

## 13. Object Slicing

Object slicing happens when a derived object is copied into a base object by value.

Example:

```cpp
class Base {
public:
    virtual void speak() const {
        std::cout << "Base\n";
    }
};

class Derived : public Base {
public:
    int extra = 42;

    void speak() const override {
        std::cout << "Derived\n";
    }
};

Derived d;
Base b = d; // slicing
```

The `Derived` part is sliced off.

Now `b` is only a `Base` object.

```cpp
b.speak(); // Base
```

To preserve polymorphism, use reference or pointer:

```cpp
Base& ref = d;
Base* ptr = &d;
```

---

## 14. Virtual Functions in Constructors and Destructors

This is another major interview trap.

Example:

```cpp
class Base {
public:
    Base() {
        speak();
    }

    virtual void speak() const {
        std::cout << "Base\n";
    }
};

class Derived : public Base {
public:
    void speak() const override {
        std::cout << "Derived\n";
    }
};
```

Usage:

```cpp
Derived d;
```

During `Base` constructor, the `Derived` part has not been constructed yet.

So calling `speak()` inside `Base` constructor calls:

```text
Base::speak()
```

not:

```text
Derived::speak()
```

Rule:

```text
Virtual calls in constructors/destructors do not dispatch to more-derived classes.
```

Why?

During base construction, the derived part does not exist yet.

During base destruction, the derived part has already been destroyed.

So C++ treats the object as the currently constructed/destructed class.

---

## 15. Access Control: public / protected / private

### public

```cpp
public:
```

Accessible from anywhere.

### protected

```cpp
protected:
```

Accessible inside the class and derived classes.

### private

```cpp
private:
```

Accessible only inside the class itself and friends.

Common pattern:

```cpp
class Base {
protected:
    int value;
};
```

Derived classes can access `value`, but outside users cannot.

---

## 16. Public Inheritance Means "is-a"

```cpp
class Dog : public Animal {};
```

This means:

```text
Dog is an Animal.
```

A `Dog*` can be converted to `Animal*`.

A `Dog&` can be bound to `Animal&`.

This enables polymorphism.

If the relationship is not truly “is-a”, prefer composition.

Example:

```cpp
class Car {
private:
    Engine engine;
};
```

A car has an engine, but a car is not an engine.

So use composition, not inheritance.

---

## 17. Composition vs Inheritance

Use inheritance for:

```text
is-a relationship
runtime polymorphism
interface abstraction
```

Use composition for:

```text
has-a relationship
code reuse without polymorphism
more flexible design
```

Example:

```cpp
class Engine {};

class Car {
private:
    Engine engine;
};
```

This is composition.

Interview answer:

```text
Prefer composition unless inheritance clearly models an is-a relationship or polymorphic interface.
```

---

## 18. `final`

`final` prevents further overriding or inheritance.

Prevent overriding a virtual function:

```cpp
class Derived : public Base {
public:
    void speak() const override final {}
};
```

Prevent inheritance from a class:

```cpp
class FinalClass final {};
```

Use `final` when you want to lock down behavior or help compiler optimization.

---

## 19. Common Interview Questions

### Q1. What is the difference between static type and dynamic type?

Static type is the type known at compile time from the variable declaration.

Dynamic type is the real runtime type of the object.

Virtual function calls use the dynamic type.

---

### Q2. What is a virtual function?

A virtual function is a member function that supports runtime dispatch.

When called through a base pointer or reference, the actual function is selected based on the dynamic type of the object.

---

### Q3. Why should a base class destructor be virtual?

If a derived object is deleted through a base pointer, the base destructor must be virtual to ensure the derived destructor runs.

Otherwise, destruction may be incomplete, causing resource leaks or undefined behavior.

---

### Q4. What is object slicing?

Object slicing occurs when a derived object is copied into a base object by value.

The derived part is lost.

Use base references or pointers to preserve polymorphism.

---

### Q5. What happens when calling virtual functions in constructors?

Virtual calls inside constructors do not dispatch to derived classes.

During base construction, the derived part has not been constructed yet, so the call resolves to the current class version.

---

### Q6. What is a vtable?

A vtable is a compiler-generated table of virtual function pointers.

Objects of classes with virtual functions usually contain a hidden vptr pointing to the appropriate vtable.

Virtual dispatch uses this vptr to call the correct function at runtime.

---

### Q7. When should you use inheritance vs composition?

Use inheritance for true is-a relationships or polymorphic interfaces.

Use composition for has-a relationships and general code reuse.

Prefer composition unless inheritance is clearly justified.

---

## 20. Key Takeaways

- Non-virtual calls use static type.
- Virtual calls use dynamic type.
- Use `override` for all overrides.
- Use pure virtual functions for abstract interfaces.
- Polymorphic base classes should usually have virtual destructors.
- Construction order: base first, derived second.
- Destruction order: derived first, base second.
- Virtual calls in constructors/destructors do not dispatch to more-derived classes.
- Object slicing happens when passing/copying derived objects by value into base objects.
- Prefer composition unless inheritance models a true is-a relationship.
