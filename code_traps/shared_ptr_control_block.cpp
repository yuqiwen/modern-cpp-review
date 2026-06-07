#include <cstdio>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
using namespace std;

/*
 * Topic:
 * shared_ptr control block, make_shared, enable_shared_from_this,
 * aliasing constructor, custom deleter.
 *
 * Compile:
 * g++ -std=c++17 shared_ptr_control_block.cpp -o shared_ptr_control_block
 *
 * Run:
 * ./shared_ptr_control_block
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
};

void controlBlockCountDemo() {
    cout << "\n=== control block count demo ===" << endl;

    auto p1 = make_shared<Resource>("shared_object");

    cout << "p1.use_count() = " << p1.use_count() << endl;

    {
        auto p2 = p1;
        cout << "after copy, p1.use_count() = " << p1.use_count() << endl;
        cout << "p2.use_count() = " << p2.use_count() << endl;
    }

    cout << "after p2 destroyed, p1.use_count() = " << p1.use_count() << endl;
}

void weakPtrControlBlockDemo() {
    cout << "\n=== weak_ptr control block demo ===" << endl;

    weak_ptr<Resource> w;

    {
        auto p = make_shared<Resource>("weak_observed");
        w = p;

        cout << "inside scope, p.use_count() = " << p.use_count() << endl;
        cout << "w.expired() = " << boolalpha << w.expired() << endl;

        if (auto locked = w.lock()) {
            cout << "lock succeeded, locked.use_count() = "
                 << locked.use_count() << endl;
        }
    }

    cout << "after shared_ptr destroyed, w.expired() = "
         << boolalpha << w.expired() << endl;

    if (auto locked = w.lock()) {
        locked->use();
    } else {
        cout << "lock failed because object expired" << endl;
    }
}

void twoSharedPtrSameRawBadDemo() {
    cout << "\n=== two shared_ptr from same raw pointer bad demo ===" << endl;

    cout << "Bad code, do not execute:" << endl;
    cout << "Resource* raw = new Resource(\"bad\");" << endl;
    cout << "shared_ptr<Resource> p1(raw);" << endl;
    cout << "shared_ptr<Resource> p2(raw); // separate control block, double delete risk" << endl;
}

class BadSelf {
public:
    shared_ptr<BadSelf> getSelfBad() {
        return shared_ptr<BadSelf>(this);
    }
};

void sharedPtrThisBadDemo() {
    cout << "\n=== shared_ptr(this) bad demo ===" << endl;

    cout << "Bad pattern, do not execute:" << endl;
    cout << "class method returns shared_ptr<T>(this), creating a second control block." << endl;
}

class Session : public enable_shared_from_this<Session> {
public:
    Session() {
        cout << "Session constructor" << endl;
    }

    ~Session() {
        cout << "Session destructor" << endl;
    }

    shared_ptr<Session> getSelf() {
        return shared_from_this();
    }
};

void enableSharedFromThisDemo() {
    cout << "\n=== enable_shared_from_this demo ===" << endl;

    auto s = make_shared<Session>();

    cout << "s.use_count() before getSelf = " << s.use_count() << endl;

    auto self = s->getSelf();

    cout << "s.use_count() after getSelf = " << s.use_count() << endl;
    cout << "self.use_count() = " << self.use_count() << endl;
}

void badWeakPtrExceptionDemo() {
    cout << "\n=== shared_from_this without shared_ptr ownership demo ===" << endl;

    cout << "If an object is not owned by shared_ptr, shared_from_this throws bad_weak_ptr." << endl;

    Session stackSession;

    try {
        auto self = stackSession.getSelf();
        (void)self;
    } catch (const bad_weak_ptr&) {
        cout << "caught bad_weak_ptr" << endl;
    }
}

struct User {
    string name;
    int age;

    User(string n, int a)
        : name(std::move(n)), age(a) {
        cout << "User constructor\n";
    }

    ~User() {
        cout << "User destructor\n";
    }
};

void aliasingConstructorDemo() {
    cout << "\n=== aliasing constructor demo ===" << endl;

    auto user = make_shared<User>("Yuqi", 25);

    shared_ptr<string> namePtr(user, &user->name);

    cout << "user.use_count() = " << user.use_count() << endl;
    cout << "namePtr.use_count() = " << namePtr.use_count() << endl;
    cout << "*namePtr = " << *namePtr << endl;

    user.reset();

    cout << "after user.reset(), namePtr still keeps User alive" << endl;
    cout << "*namePtr = " << *namePtr << endl;
}

void customDeleterDemo() {
    cout << "\n=== shared_ptr custom deleter demo ===" << endl;

    shared_ptr<FILE> file(
        tmpfile(),
        [](FILE* f) {
            cout << "custom deleter for FILE*\n";
            if (f) {
                fclose(f);
            }
        }
    );

    if (file) {
        cout << "temporary FILE opened" << endl;
    }
}

void passingSharedPtrDemo() {
    cout << "\n=== passing shared_ptr demo ===" << endl;

    auto p = make_shared<Resource>("pass_demo");

    auto keep = [](shared_ptr<Resource> q) {
        cout << "inside keep by value, use_count = " << q.use_count() << endl;
    };

    auto observePtr = [](const shared_ptr<Resource>& q) {
        cout << "inside observe shared_ptr by const ref, use_count = "
             << q.use_count() << endl;
    };

    auto observeObject = [](const Resource& r) {
        cout << "inside observe object reference: ";
        r.use();
    };

    cout << "before calls, use_count = " << p.use_count() << endl;
    keep(p);
    cout << "after keep, use_count = " << p.use_count() << endl;

    observePtr(p);
    observeObject(*p);
}

int main() {
    controlBlockCountDemo();

    weakPtrControlBlockDemo();

    twoSharedPtrSameRawBadDemo();

    sharedPtrThisBadDemo();

    enableSharedFromThisDemo();

    badWeakPtrExceptionDemo();

    aliasingConstructorDemo();

    customDeleterDemo();

    passingSharedPtrDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}