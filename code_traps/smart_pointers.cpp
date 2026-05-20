#include <iostream>
#include <memory>
#include <string>
using namespace std;

/*
 * Topic:
 * Smart pointers: unique_ptr, shared_ptr, weak_ptr.
 *
 * Compile:
 * g++ -std=c++17 smart_pointers.cpp -o smart_pointers
 *
 * Run:
 * ./smart_pointers
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
        cout << "Using resource: " << name << endl;
    }

    const string& getName() const {
        return name;
    }
};

void useByReference(const Resource& r) {
    cout << "useByReference: ";
    r.use();
}

void takeUniqueOwnership(unique_ptr<Resource> r) {
    cout << "takeUniqueOwnership owns: " << r->getName() << endl;
}

void inspectUniquePtr(const unique_ptr<Resource>& r) {
    if (r) {
        cout << "inspectUniquePtr sees: " << r->getName() << endl;
    } else {
        cout << "inspectUniquePtr sees nullptr" << endl;
    }
}

void shareOwnership(shared_ptr<Resource> r) {
    cout << "shareOwnership use_count = " << r.use_count() << endl;
    r->use();
}

void inspectSharedPtr(const shared_ptr<Resource>& r) {
    cout << "inspectSharedPtr use_count = " << r.use_count() << endl;
    if (r) {
        r->use();
    }
}

void uniquePtrDemo() {
    cout << "\n=== unique_ptr demo ===" << endl;

    auto p1 = make_unique<Resource>("unique_resource");

    p1->use();

    // unique_ptr cannot be copied:
    // auto p2 = p1; // error

    auto p2 = std::move(p1);

    if (!p1) {
        cout << "p1 is nullptr after move" << endl;
    }

    p2->use();

    useByReference(*p2);
    inspectUniquePtr(p2);

    takeUniqueOwnership(std::move(p2));

    if (!p2) {
        cout << "p2 is nullptr after ownership transfer" << endl;
    }
}

void releaseResetGetDemo() {
    cout << "\n=== release / reset / get demo ===" << endl;

    auto p = make_unique<Resource>("release_demo");

    Resource* raw = p.get();
    cout << "raw from get(): ";
    raw->use();

    cout << "p still owns the resource after get()" << endl;

    Resource* released = p.release();

    if (!p) {
        cout << "p is nullptr after release()" << endl;
    }

    cout << "released raw pointer must be deleted manually now" << endl;
    delete released;

    p = make_unique<Resource>("reset_demo");
    p.reset(); // deletes reset_demo resource
    cout << "p.reset() deleted the resource and made p null" << endl;
}

void sharedPtrDemo() {
    cout << "\n=== shared_ptr demo ===" << endl;

    auto p1 = make_shared<Resource>("shared_resource");
    cout << "p1 use_count = " << p1.use_count() << endl;

    {
        auto p2 = p1;
        cout << "after copy, p1 use_count = " << p1.use_count() << endl;
        cout << "p2 use_count = " << p2.use_count() << endl;

        shareOwnership(p2);
        cout << "after shareOwnership, p1 use_count = " << p1.use_count() << endl;

        inspectSharedPtr(p2);
        cout << "after inspectSharedPtr, p1 use_count = " << p1.use_count() << endl;
    }

    cout << "after p2 scope ends, p1 use_count = " << p1.use_count() << endl;
}

void weakPtrDemo() {
    cout << "\n=== weak_ptr demo ===" << endl;

    weak_ptr<Resource> weak;

    {
        auto shared = make_shared<Resource>("weak_observed_resource");
        weak = shared;

        cout << "shared use_count = " << shared.use_count() << endl;

        if (auto locked = weak.lock()) {
            cout << "weak.lock() succeeded: ";
            locked->use();
            cout << "use_count while locked = " << locked.use_count() << endl;
        }
    }

    cout << "shared_ptr is gone" << endl;

    if (auto locked = weak.lock()) {
        locked->use();
    } else {
        cout << "weak_ptr expired; object was destroyed" << endl;
    }
}

class NodeWithCycle {
public:
    string name;
    shared_ptr<NodeWithCycle> next;
    shared_ptr<NodeWithCycle> prev;

    explicit NodeWithCycle(string n)
        : name(std::move(n)) {
        cout << "NodeWithCycle constructor: " << name << endl;
    }

    ~NodeWithCycle() {
        cout << "NodeWithCycle destructor: " << name << endl;
    }
};

class NodeWithoutCycle {
public:
    string name;
    shared_ptr<NodeWithoutCycle> next;
    weak_ptr<NodeWithoutCycle> prev;

    explicit NodeWithoutCycle(string n)
        : name(std::move(n)) {
        cout << "NodeWithoutCycle constructor: " << name << endl;
    }

    ~NodeWithoutCycle() {
        cout << "NodeWithoutCycle destructor: " << name << endl;
    }
};

void sharedPtrCycleDemo() {
    cout << "\n=== shared_ptr cycle demo ===" << endl;

    {
        auto a = make_shared<NodeWithCycle>("A");
        auto b = make_shared<NodeWithCycle>("B");

        a->next = b;
        b->prev = a;

        cout << "Leaving scope with shared_ptr cycle." << endl;
    }

    cout << "If destructors above did not run, the cycle kept objects alive." << endl;
}

void weakPtrBreakCycleDemo() {
    cout << "\n=== weak_ptr breaks cycle demo ===" << endl;

    {
        auto a = make_shared<NodeWithoutCycle>("A");
        auto b = make_shared<NodeWithoutCycle>("B");

        a->next = b;
        b->prev = a;

        cout << "Leaving scope without ownership cycle." << endl;
    }

    cout << "Destructors should have run because weak_ptr did not keep A alive." << endl;
}

class GoodSelfShared : public enable_shared_from_this<GoodSelfShared> {
public:
    GoodSelfShared() {
        cout << "GoodSelfShared constructor" << endl;
    }

    ~GoodSelfShared() {
        cout << "GoodSelfShared destructor" << endl;
    }

    shared_ptr<GoodSelfShared> getPtr() {
        return shared_from_this();
    }
};

void enableSharedFromThisDemo() {
    cout << "\n=== enable_shared_from_this demo ===" << endl;

    auto p1 = make_shared<GoodSelfShared>();
    auto p2 = p1->getPtr();

    cout << "p1 use_count = " << p1.use_count() << endl;
    cout << "p2 use_count = " << p2.use_count() << endl;
}

int main() {
    uniquePtrDemo();

    releaseResetGetDemo();

    sharedPtrDemo();

    weakPtrDemo();

    sharedPtrCycleDemo();

    weakPtrBreakCycleDemo();

    enableSharedFromThisDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}