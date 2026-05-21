#include <iostream>
#include <memory>
#include <string>
using namespace std;

/*
 * Topic:
 * Inheritance, virtual functions, vtable, and virtual destructor.
 *
 * Compile:
 * g++ -std=c++17 inheritance_virtual_vtable.cpp -o inheritance_virtual_vtable
 *
 * Run:
 * ./inheritance_virtual_vtable
 */

class NonVirtualBase {
public:
    void speak() const {
        cout << "NonVirtualBase::speak\n";
    }
};

class NonVirtualDerived : public NonVirtualBase {
public:
    void speak() const {
        cout << "NonVirtualDerived::speak\n";
    }
};

void nonVirtualDemo() {
    cout << "\n=== non-virtual function demo ===" << endl;

    NonVirtualDerived d;
    NonVirtualBase* p = &d;

    p->speak(); // static type is NonVirtualBase*, so calls NonVirtualBase::speak
}

class VirtualBase {
public:
    virtual ~VirtualBase() = default;

    virtual void speak() const {
        cout << "VirtualBase::speak\n";
    }
};

class VirtualDerived : public VirtualBase {
public:
    void speak() const override {
        cout << "VirtualDerived::speak\n";
    }
};

void virtualDemo() {
    cout << "\n=== virtual function demo ===" << endl;

    VirtualDerived d;
    VirtualBase* p = &d;
    VirtualBase& r = d;

    p->speak(); // dynamic dispatch
    r.speak();  // dynamic dispatch
}

class BadBaseDestructor {
public:
    ~BadBaseDestructor() {
        cout << "BadBaseDestructor destructor\n";
    }
};

class BadDerivedDestructor : public BadBaseDestructor {
private:
    int* data;

public:
    BadDerivedDestructor()
        : data(new int[10]) {
        cout << "BadDerivedDestructor constructor\n";
    }

    ~BadDerivedDestructor() {
        cout << "BadDerivedDestructor destructor, deleting data\n";
        delete[] data;
    }
};

void badVirtualDestructorDemo() {
    cout << "\n=== bad virtual destructor demo ===" << endl;

    cout << "This is dangerous and commented out." << endl;

    // BadBaseDestructor* p = new BadDerivedDestructor();
    // delete p;
    //
    // Undefined behavior:
    // base destructor is not virtual.
    // Derived destructor may not run.
}

class GoodBaseDestructor {
public:
    virtual ~GoodBaseDestructor() {
        cout << "GoodBaseDestructor destructor\n";
    }
};

class GoodDerivedDestructor : public GoodBaseDestructor {
private:
    int* data;

public:
    GoodDerivedDestructor()
        : data(new int[10]) {
        cout << "GoodDerivedDestructor constructor\n";
    }

    ~GoodDerivedDestructor() override {
        cout << "GoodDerivedDestructor destructor, deleting data\n";
        delete[] data;
    }
};

void goodVirtualDestructorDemo() {
    cout << "\n=== good virtual destructor demo ===" << endl;

    GoodBaseDestructor* p = new GoodDerivedDestructor();
    delete p;

    cout << "Derived destructor ran before base destructor." << endl;
}

class Shape {
public:
    virtual ~Shape() = default;

    virtual double area() const = 0;
    virtual string name() const = 0;
};

class Circle : public Shape {
private:
    double radius;

public:
    explicit Circle(double r)
        : radius(r) {}

    double area() const override {
        return 3.14159 * radius * radius;
    }

    string name() const override {
        return "Circle";
    }
};

class Rectangle : public Shape {
private:
    double width;
    double height;

public:
    Rectangle(double w, double h)
        : width(w), height(h) {}

    double area() const override {
        return width * height;
    }

    string name() const override {
        return "Rectangle";
    }
};

void printShape(const Shape& shape) {
    cout << shape.name() << " area = " << shape.area() << endl;
}

void abstractClassDemo() {
    cout << "\n=== abstract class / interface demo ===" << endl;

    Circle c(2.0);
    Rectangle r(3.0, 4.0);

    printShape(c);
    printShape(r);

    unique_ptr<Shape> p = make_unique<Circle>(5.0);
    printShape(*p);
}

class SliceBase {
public:
    virtual ~SliceBase() = default;

    virtual void speak() const {
        cout << "SliceBase::speak\n";
    }
};

class SliceDerived : public SliceBase {
public:
    int extra = 42;

    void speak() const override {
        cout << "SliceDerived::speak, extra = " << extra << "\n";
    }
};

void slicingDemo() {
    cout << "\n=== object slicing demo ===" << endl;

    SliceDerived d;

    SliceBase b = d; // slicing: derived part is lost
    b.speak();       // SliceBase::speak

    SliceBase& ref = d;
    ref.speak();     // SliceDerived::speak
}

class ConstructorVirtualBase {
public:
    ConstructorVirtualBase() {
        cout << "ConstructorVirtualBase constructor calls speak(): ";
        speak();
    }

    virtual ~ConstructorVirtualBase() {
        cout << "ConstructorVirtualBase destructor calls speak(): ";
        speak();
    }

    virtual void speak() const {
        cout << "ConstructorVirtualBase::speak\n";
    }
};

class ConstructorVirtualDerived : public ConstructorVirtualBase {
public:
    ConstructorVirtualDerived() {
        cout << "ConstructorVirtualDerived constructor body\n";
    }

    ~ConstructorVirtualDerived() override {
        cout << "ConstructorVirtualDerived destructor body\n";
    }

    void speak() const override {
        cout << "ConstructorVirtualDerived::speak\n";
    }
};

void constructorVirtualCallDemo() {
    cout << "\n=== virtual call in constructor/destructor demo ===" << endl;

    ConstructorVirtualDerived d;

    cout << "Leaving function now." << endl;
}

class Engine {
public:
    void start() const {
        cout << "Engine starts\n";
    }
};

class Car {
private:
    Engine engine;

public:
    void start() const {
        engine.start();
        cout << "Car starts\n";
    }
};

void compositionDemo() {
    cout << "\n=== composition demo ===" << endl;

    Car car;
    car.start();

    cout << "Car has an Engine, so composition is better than inheritance here." << endl;
}

int main() {
    nonVirtualDemo();

    virtualDemo();

    badVirtualDestructorDemo();

    goodVirtualDestructorDemo();

    abstractClassDemo();

    slicingDemo();

    constructorVirtualCallDemo();

    compositionDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}