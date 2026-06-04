#include <cstdlib>
#include <iostream>
#include <memory>
#include <new>
#include <string>
using namespace std;

/*
 * Topic:
 * Low-level memory:
 * malloc/free, new/delete, operator new, placement new.
 *
 * Compile:
 * g++ -std=c++20 low_level_memory_new_malloc_placement_new.cpp -o low_level_memory_new_malloc_placement_new
 *
 * Run:
 * ./low_level_memory_new_malloc_placement_new
 */

class Tracker {
private:
    string name;

public:
    explicit Tracker(string n)
        : name(std::move(n)) {
        cout << "Tracker constructor: " << name << endl;
    }

    ~Tracker() {
        cout << "Tracker destructor: " << name << endl;
    }

    void hello() const {
        cout << "hello from " << name << endl;
    }
};

void mallocFreeDemo() {
    cout << "\n=== malloc/free demo ===" << endl;

    void* mem = malloc(sizeof(Tracker));

    if (!mem) {
        throw bad_alloc();
    }

    cout << "malloc allocated raw memory only." << endl;
    cout << "No Tracker constructor was called." << endl;

    free(mem);

    cout << "free released raw memory only." << endl;
}

void newDeleteDemo() {
    cout << "\n=== new/delete demo ===" << endl;

    Tracker* p = new Tracker("new_delete");

    p->hello();

    delete p;
}

void operatorNewPlacementNewDemo() {
    cout << "\n=== operator new + placement new demo ===" << endl;

    void* mem = ::operator new(sizeof(Tracker));

    cout << "operator new allocated raw memory only." << endl;

    Tracker* p = new (mem) Tracker("placement_new");

    p->hello();

    p->~Tracker();

    cout << "manual destructor called." << endl;

    ::operator delete(mem);

    cout << "operator delete released raw memory." << endl;
}

void mallocPlacementNewDemo() {
    cout << "\n=== malloc + placement new demo ===" << endl;

    void* mem = malloc(sizeof(Tracker));

    if (!mem) {
        throw bad_alloc();
    }

    Tracker* p = new (mem) Tracker("malloc_storage");

    p->hello();

    p->~Tracker();

    free(mem);
}

void constructAtDestroyAtDemo() {
    cout << "\n=== construct_at / destroy_at demo ===" << endl;

    allocator<Tracker> alloc;

    Tracker* p = alloc.allocate(1);

    cout << "allocator allocated raw storage." << endl;

    construct_at(p, "construct_at");

    p->hello();

    destroy_at(p);

    alloc.deallocate(p, 1);
}

class CustomNew {
public:
    static void* operator new(size_t size) {
        cout << "CustomNew::operator new, size = " << size << endl;
        return ::operator new(size);
    }

    static void operator delete(void* p) noexcept {
        cout << "CustomNew::operator delete" << endl;
        ::operator delete(p);
    }

    CustomNew() {
        cout << "CustomNew constructor" << endl;
    }

    ~CustomNew() {
        cout << "CustomNew destructor" << endl;
    }
};

void customOperatorNewDemo() {
    cout << "\n=== custom operator new demo ===" << endl;

    CustomNew* p = new CustomNew();

    delete p;
}

void rawMemoryAssignmentTrapDemo() {
    cout << "\n=== raw memory assignment trap demo ===" << endl;

    void* mem = ::operator new(sizeof(string));

    cout << "Raw memory allocated for string." << endl;

    cout << "Do NOT do this:" << endl;
    cout << "string* s = static_cast<string*>(mem);" << endl;
    cout << "*s = \"hello\"; // wrong, no string object exists yet" << endl;

    string* s = new (mem) string("constructed correctly");

    cout << "*s = " << *s << endl;

    s->~basic_string();

    ::operator delete(mem);
}

void arrayNewDeleteDemo() {
    cout << "\n=== new[] / delete[] demo ===" << endl;

    Tracker* arr = new Tracker[2]{
        Tracker("arr0"),
        Tracker("arr1")
    };

    arr[0].hello();
    arr[1].hello();

    delete[] arr;
}

int main() {
    mallocFreeDemo();

    newDeleteDemo();

    operatorNewPlacementNewDemo();

    mallocPlacementNewDemo();

    constructAtDestroyAtDemo();

    customOperatorNewDemo();

    rawMemoryAssignmentTrapDemo();

    arrayNewDeleteDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}