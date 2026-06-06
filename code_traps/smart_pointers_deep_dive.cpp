#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
using namespace std;

/*
 * Topic:
 * Smart pointers deep dive.
 *
 * Compile:
 * g++ -std=c++17 smart_pointers_deep_dive.cpp -o smart_pointers_deep_dive
 *
 * Run:
 * ./smart_pointers_deep_dive
 */

class Resource {
private:
    string name;

public:
    explicit Resource(string n)
        : name(std::move(n)) {
        cout << "Resource constructor: " << name << endl;
    }

    ~Resource() {
        cout << "Resource destructor: " << name << endl;
    }

    void use() const {
        cout << "using Resource: " << name << endl;
    }

    const string& getName() const {
        return name;
    }
};

void inspect(const Resource& r) {
    cout << "inspect: ";
    r.use();
}

void maybeInspect(const Resource* r) {
    if (r) {
        cout << "maybeInspect: ";
        r->use();
    } else {
        cout << "maybeInspect: null\n";
    }
}

void takeUnique(unique_ptr<Resource> r) {
    cout << "takeUnique owns: ";
    r->use();
}

unique_ptr<Resource> makeResource() {
    return make_unique<Resource>("factory_resource");
}

void uniquePtrDemo() {
    cout << "\n=== unique_ptr demo ===" << endl;

    auto p = make_unique<Resource>("unique_A");

    inspect(*p);
    maybeInspect(p.get());

    auto q = std::move(p);

    if (!p) {
        cout << "p is null after move\n";
    }

    q->use();

    takeUnique(std::move(q));

    if (!q) {
        cout << "q is null after ownership transfer\n";
    }

    auto fromFactory = makeResource();
    fromFactory->use();
}

class Shape {
public:
    virtual ~Shape() = default;
    virtual double area() const = 0;
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
};

void polymorphicUniquePtrDemo() {
    cout << "\n=== polymorphic unique_ptr demo ===" << endl;

    unique_ptr<Shape> shape = make_unique<Circle>(2.0);

    cout << "area = " << shape->area() << endl;
}

struct FileCloser {
    void operator()(FILE* f) const {
        if (f) {
            cout << "closing FILE*\n";
            fclose(f);
        }
    }
};

void customDeleterDemo() {
    cout << "\n=== unique_ptr custom deleter demo ===" << endl;

    unique_ptr<FILE, FileCloser> file(tmpfile());

    if (file) {
        cout << "temporary file opened\n";
    }
}

void keepShared(shared_ptr<Resource> r) {
    cout << "inside keepShared, use_count = " << r.use_count() << endl;
    r->use();
}

void observeSharedByConstRef(const shared_ptr<Resource>& r) {
    cout << "inside observeSharedByConstRef, use_count = " << r.use_count() << endl;

    if (r) {
        r->use();
    }
}

void sharedPtrDemo() {
    cout << "\n=== shared_ptr demo ===" << endl;

    auto p1 = make_shared<Resource>("shared_A");

    cout << "after make_shared, use_count = " << p1.use_count() << endl;

    {
        auto p2 = p1;

        cout << "after copy to p2, use_count = " << p1.use_count() << endl;

        keepShared(p1);

        observeSharedByConstRef(p1);

        inspect(*p1);
    }

    cout << "after p2 scope, use_count = " << p1.use_count() << endl;
}

struct BadNode {
    string name;
    shared_ptr<BadNode> other;

    explicit BadNode(string n)
        : name(std::move(n)) {
        cout << "BadNode constructor: " << name << endl;
    }

    ~BadNode() {
        cout << "BadNode destructor: " << name << endl;
    }
};

void sharedPtrCycleDemo() {
    cout << "\n=== shared_ptr cycle demo ===" << endl;

    auto a = make_shared<BadNode>("A");
    auto b = make_shared<BadNode>("B");

    a->other = b;
    b->other = a;

    cout << "a use_count = " << a.use_count() << endl;
    cout << "b use_count = " << b.use_count() << endl;

    cout << "When function exits, destructors will NOT run due to cycle." << endl;
}

struct GoodNode {
    string name;
    shared_ptr<GoodNode> next;
    weak_ptr<GoodNode> prev;

    explicit GoodNode(string n)
        : name(std::move(n)) {
        cout << "GoodNode constructor: " << name << endl;
    }

    ~GoodNode() {
        cout << "GoodNode destructor: " << name << endl;
    }
};

void weakPtrDemo() {
    cout << "\n=== weak_ptr demo ===" << endl;

    weak_ptr<Resource> weak;

    {
        auto p = make_shared<Resource>("weak_target");
        weak = p;

        cout << "inside scope, expired = " << boolalpha << weak.expired() << endl;

        if (auto locked = weak.lock()) {
            locked->use();
            cout << "locked use_count = " << locked.use_count() << endl;
        }
    }

    cout << "after shared_ptr destroyed, expired = " << boolalpha << weak.expired() << endl;

    if (auto locked = weak.lock()) {
        locked->use();
    } else {
        cout << "weak.lock() returned empty shared_ptr\n";
    }
}

void weakBreakCycleDemo() {
    cout << "\n=== weak_ptr breaks cycle demo ===" << endl;

    auto a = make_shared<GoodNode>("A");
    auto b = make_shared<GoodNode>("B");

    a->next = b;
    b->prev = a;

    cout << "a use_count = " << a.use_count() << endl;
    cout << "b use_count = " << b.use_count() << endl;

    cout << "When function exits, destructors will run." << endl;
}

class Session : public enable_shared_from_this<Session> {
public:
    Session() {
        cout << "Session constructor\n";
    }

    ~Session() {
        cout << "Session destructor\n";
    }

    shared_ptr<Session> getSelf() {
        return shared_from_this();
    }
};

void enableSharedFromThisDemo() {
    cout << "\n=== enable_shared_from_this demo ===" << endl;

    auto session = make_shared<Session>();

    auto self = session->getSelf();

    cout << "session use_count = " << session.use_count() << endl;
}

void doubleSharedPtrBadDemo() {
    cout << "\n=== double shared_ptr from same raw pointer demo ===" << endl;

    cout << "Bad pattern, do not run:" << endl;
    cout << "Resource* raw = new Resource(\"bad\");" << endl;
    cout << "shared_ptr<Resource> p1(raw);" << endl;
    cout << "shared_ptr<Resource> p2(raw); // double delete risk" << endl;
}

int main() {
    uniquePtrDemo();

    polymorphicUniquePtrDemo();

    customDeleterDemo();

    sharedPtrDemo();

    sharedPtrCycleDemo();

    weakPtrDemo();

    weakBreakCycleDemo();

    enableSharedFromThisDemo();

    doubleSharedPtrBadDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}