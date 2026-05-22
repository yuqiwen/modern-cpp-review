#include <iostream>
#include <memory>
#include <string>
#include <typeinfo>
using namespace std;

/*
 * Topic:
 * C++ casts: static_cast, dynamic_cast, const_cast, reinterpret_cast.
 *
 * Compile:
 * g++ -std=c++17 cpp_casts.cpp -o cpp_casts
 *
 * Run:
 * ./cpp_casts
 */

class Animal {
public:
    virtual ~Animal() = default;

    virtual string type() const {
        return "Animal";
    }
};

class Dog : public Animal {
public:
    string type() const override {
        return "Dog";
    }

    void bark() const {
        cout << "Dog barks\n";
    }
};

class Cat : public Animal {
public:
    string type() const override {
        return "Cat";
    }

    void meow() const {
        cout << "Cat meows\n";
    }
};

void staticCastNumericDemo() {
    cout << "\n=== static_cast numeric demo ===" << endl;

    double d = 3.14;
    int x = static_cast<int>(d);

    cout << "d = " << d << ", x = " << x << endl;
}

void staticCastUpcastDemo() {
    cout << "\n=== static_cast upcast demo ===" << endl;

    Dog dog;

    Animal* a1 = &dog;                    // implicit upcast
    Animal* a2 = static_cast<Animal*>(&dog);

    cout << "a1 type = " << a1->type() << endl;
    cout << "a2 type = " << a2->type() << endl;
}

void staticCastDowncastDangerDemo() {
    cout << "\n=== static_cast downcast danger demo ===" << endl;

    Cat cat;
    Animal* animal = &cat;

    cout << "animal dynamic type = " << animal->type() << endl;

    cout << "Dangerous cast commented out:" << endl;
    cout << "Dog* dog = static_cast<Dog*>(animal); // compiles but wrong" << endl;
    cout << "dog->bark(); // undefined behavior if animal is actually Cat" << endl;

    // Dog* dog = static_cast<Dog*>(animal);
    // dog->bark(); // undefined behavior
}

void dynamicCastDemo() {
    cout << "\n=== dynamic_cast demo ===" << endl;

    Dog dog;
    Cat cat;

    Animal* a1 = &dog;
    Animal* a2 = &cat;

    if (Dog* d = dynamic_cast<Dog*>(a1)) {
        cout << "a1 is Dog: ";
        d->bark();
    }

    if (Dog* d = dynamic_cast<Dog*>(a2)) {
        d->bark();
    } else {
        cout << "a2 is not Dog, dynamic_cast returned nullptr" << endl;
    }
}

void dynamicCastReferenceDemo() {
    cout << "\n=== dynamic_cast reference demo ===" << endl;

    Cat cat;
    Animal& animalRef = cat;

    try {
        Dog& dogRef = dynamic_cast<Dog&>(animalRef);
        dogRef.bark();
    } catch (const bad_cast& e) {
        cout << "dynamic_cast<Dog&> failed and threw bad_cast" << endl;
    }
}

void constCastDemo() {
    cout << "\n=== const_cast demo ===" << endl;

    int x = 10;
    const int& constRef = x;

    int& mutableRef = const_cast<int&>(constRef);
    mutableRef = 20;

    cout << "x after const_cast on originally non-const object = " << x << endl;

    cout << "Dangerous example commented out:" << endl;
    cout << "const int y = 30; int* p = const_cast<int*>(&y); *p = 40; // UB" << endl;

    // const int y = 30;
    // int* p = const_cast<int*>(&y);
    // *p = 40; // undefined behavior
}

void reinterpretCastDemo() {
    cout << "\n=== reinterpret_cast demo ===" << endl;

    int x = 0x12345678;

    unsigned char* bytes = reinterpret_cast<unsigned char*>(&x);

    cout << "Memory bytes of int x: ";
    for (size_t i = 0; i < sizeof(int); ++i) {
        cout << hex << static_cast<int>(bytes[i]) << " ";
    }
    cout << dec << endl;

    cout << "reinterpret_cast views memory representation, not semantic conversion." << endl;
}

void sharedPtrCastDemo() {
    cout << "\n=== shared_ptr cast demo ===" << endl;

    shared_ptr<Animal> animal = make_shared<Dog>();

    shared_ptr<Dog> dog = dynamic_pointer_cast<Dog>(animal);
    if (dog) {
        dog->bark();
        cout << "animal use_count = " << animal.use_count() << endl;
        cout << "dog use_count = " << dog.use_count() << endl;
    }

    shared_ptr<Cat> cat = dynamic_pointer_cast<Cat>(animal);
    if (!cat) {
        cout << "dynamic_pointer_cast<Cat> failed, cat is empty" << endl;
    }
}

class InterfaceA {
public:
    virtual ~InterfaceA() = default;
    virtual void a() const = 0;
};

class InterfaceB {
public:
    virtual ~InterfaceB() = default;
    virtual void b() const = 0;
};

class Impl : public InterfaceA, public InterfaceB {
public:
    void a() const override {
        cout << "Impl::a\n";
    }

    void b() const override {
        cout << "Impl::b\n";
    }
};

void crossCastDemo() {
    cout << "\n=== cross-cast demo ===" << endl;

    Impl impl;
    InterfaceA* a = &impl;

    InterfaceB* b = dynamic_cast<InterfaceB*>(a);
    if (b) {
        b->b();
    }
}

int main() {
    staticCastNumericDemo();

    staticCastUpcastDemo();

    staticCastDowncastDangerDemo();

    dynamicCastDemo();

    dynamicCastReferenceDemo();

    constCastDemo();

    reinterpretCastDemo();

    sharedPtrCastDemo();

    crossCastDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}