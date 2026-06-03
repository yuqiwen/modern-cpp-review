#include <iostream>
#include <memory>
#include <string>
#include <utility>
using namespace std;

/*
 * Topic:
 * Generic container Rule of Five and exception safety.
 *
 * Compile:
 * g++ -std=c++20 generic_container_rule_of_five.cpp -o generic_container_rule_of_five
 *
 * Run:
 * ./generic_container_rule_of_five
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

    Tracker(const Tracker& other)
        : name(other.name + "_copy") {
        cout << "Tracker copy constructor from " << other.name << endl;
    }

    Tracker(Tracker&& other) noexcept
        : name(std::move(other.name)) {
        cout << "Tracker move constructor" << endl;
        other.name = "moved_from";
    }

    Tracker& operator=(const Tracker& other) {
        cout << "Tracker copy assignment from " << other.name << endl;
        name = other.name + "_copy_assigned";
        return *this;
    }

    Tracker& operator=(Tracker&& other) noexcept {
        cout << "Tracker move assignment" << endl;
        name = std::move(other.name);
        other.name = "moved_from";
        return *this;
    }

    const string& getName() const {
        return name;
    }
};

class ThrowOnCopy {
private:
    int value;

public:
    static inline int copyCount = 0;
    static inline int throwAfter = 2;

    explicit ThrowOnCopy(int v)
        : value(v) {
        cout << "ThrowOnCopy constructor: " << value << endl;
    }

    ~ThrowOnCopy() {
        cout << "ThrowOnCopy destructor: " << value << endl;
    }

    ThrowOnCopy(const ThrowOnCopy& other)
        : value(other.value) {
        ++copyCount;
        cout << "ThrowOnCopy copy constructor: " << value
             << ", copyCount = " << copyCount << endl;

        if (copyCount >= throwAfter) {
            cout << "ThrowOnCopy throws during copy\n";
            throw runtime_error("copy failed");
        }
    }

    ThrowOnCopy(ThrowOnCopy&& other) noexcept
        : value(other.value) {
        cout << "ThrowOnCopy move constructor: " << value << endl;
        other.value = 0;
    }

    int get() const {
        return value;
    }
};

template <typename T>
class SimpleVector {
private:
    T* data_ = nullptr;
    size_t size_ = 0;
    size_t capacity_ = 0;
    allocator<T> alloc_;

public:
    SimpleVector() = default;

    ~SimpleVector() {
        clear();

        if (data_) {
            alloc_.deallocate(data_, capacity_);
        }
    }

    SimpleVector(const SimpleVector& other)
        : data_(nullptr), size_(0), capacity_(0) {
        if (other.size_ == 0) {
            return;
        }

        data_ = alloc_.allocate(other.size_);
        capacity_ = other.size_;

        size_t constructed = 0;

        try {
            for (; constructed < other.size_; ++constructed) {
                construct_at(data_ + constructed, other.data_[constructed]);
            }
        } catch (...) {
            for (size_t i = 0; i < constructed; ++i) {
                destroy_at(data_ + i);
            }

            alloc_.deallocate(data_, capacity_);

            data_ = nullptr;
            capacity_ = 0;

            throw;
        }

        size_ = other.size_;
    }

    SimpleVector& operator=(const SimpleVector& other) {
        if (this == &other) {
            return *this;
        }

        SimpleVector temp(other);
        swap(temp);

        return *this;
    }

    SimpleVector(SimpleVector&& other) noexcept
        : data_(other.data_),
          size_(other.size_),
          capacity_(other.capacity_) {
        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    SimpleVector& operator=(SimpleVector&& other) noexcept {
        if (this == &other) {
            return *this;
        }

        clear();

        if (data_) {
            alloc_.deallocate(data_, capacity_);
        }

        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;

        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;

        return *this;
    }

    void swap(SimpleVector& other) noexcept {
        using std::swap;
        swap(data_, other.data_);
        swap(size_, other.size_);
        swap(capacity_, other.capacity_);
    }

    void push_back(const T& value) {
        if (size_ == capacity_) {
            grow();
        }

        construct_at(data_ + size_, value);
        ++size_;
    }

    void push_back(T&& value) {
        if (size_ == capacity_) {
            grow();
        }

        construct_at(data_ + size_, std::move(value));
        ++size_;
    }

    template <typename... Args>
    T& emplace_back(Args&&... args) {
        if (size_ == capacity_) {
            grow();
        }

        construct_at(data_ + size_, std::forward<Args>(args)...);
        ++size_;

        return data_[size_ - 1];
    }

    void clear() {
        for (size_t i = 0; i < size_; ++i) {
            destroy_at(data_ + i);
        }

        size_ = 0;
    }

    size_t size() const {
        return size_;
    }

    size_t capacity() const {
        return capacity_;
    }

    T& operator[](size_t i) {
        return data_[i];
    }

    const T& operator[](size_t i) const {
        return data_[i];
    }

private:
    void grow() {
        size_t newCapacity = capacity_ == 0 ? 1 : capacity_ * 2;
        T* newData = alloc_.allocate(newCapacity);

        size_t constructed = 0;

        try {
            for (; constructed < size_; ++constructed) {
                construct_at(
                    newData + constructed,
                    std::move_if_noexcept(data_[constructed])
                );
            }
        } catch (...) {
            for (size_t i = 0; i < constructed; ++i) {
                destroy_at(newData + i);
            }

            alloc_.deallocate(newData, newCapacity);
            throw;
        }

        for (size_t i = 0; i < size_; ++i) {
            destroy_at(data_ + i);
        }

        if (data_) {
            alloc_.deallocate(data_, capacity_);
        }

        data_ = newData;
        capacity_ = newCapacity;
    }
};

void copyConstructorDemo() {
    cout << "\n=== copy constructor demo ===" << endl;

    SimpleVector<Tracker> a;
    a.emplace_back("A");
    a.emplace_back("B");

    cout << "\nCopy construct b from a:\n";
    SimpleVector<Tracker> b = a;

    cout << "a size = " << a.size() << endl;
    cout << "b size = " << b.size() << endl;
}

void copyAssignmentDemo() {
    cout << "\n=== copy assignment demo ===" << endl;

    SimpleVector<Tracker> a;
    a.emplace_back("A1");
    a.emplace_back("A2");

    SimpleVector<Tracker> b;
    b.emplace_back("B1");

    cout << "\nAssign b = a:\n";
    b = a;

    cout << "a size = " << a.size() << endl;
    cout << "b size = " << b.size() << endl;
}

void moveConstructorDemo() {
    cout << "\n=== move constructor demo ===" << endl;

    SimpleVector<Tracker> a;
    a.emplace_back("M1");
    a.emplace_back("M2");

    cout << "\nMove construct b from a:\n";
    SimpleVector<Tracker> b = std::move(a);

    cout << "a size after move = " << a.size() << endl;
    cout << "b size after move = " << b.size() << endl;
}

void moveAssignmentDemo() {
    cout << "\n=== move assignment demo ===" << endl;

    SimpleVector<Tracker> a;
    a.emplace_back("MA1");
    a.emplace_back("MA2");

    SimpleVector<Tracker> b;
    b.emplace_back("old_B");

    cout << "\nMove assign b = std::move(a):\n";
    b = std::move(a);

    cout << "a size after move = " << a.size() << endl;
    cout << "b size after move = " << b.size() << endl;
}

void exceptionSafetyCopyDemo() {
    cout << "\n=== exception safety copy demo ===" << endl;

    ThrowOnCopy::copyCount = 0;
    ThrowOnCopy::throwAfter = 2;

    SimpleVector<ThrowOnCopy> a;
    a.emplace_back(1);
    a.emplace_back(2);
    a.emplace_back(3);

    try {
        cout << "\nTry copy construct b from a:\n";
        SimpleVector<ThrowOnCopy> b = a;
    } catch (const exception& e) {
        cout << "Caught exception: " << e.what() << endl;
    }

    cout << "Original vector a is still valid, size = " << a.size() << endl;
}

int main() {
    copyConstructorDemo();

    copyAssignmentDemo();

    moveConstructorDemo();

    moveAssignmentDemo();

    exceptionSafetyCopyDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}