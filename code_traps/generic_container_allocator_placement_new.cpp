#include <iostream>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
using namespace std;

/*
 * Topic:
 * Generic container, allocator, placement new, object lifetime.
 *
 * Compile:
 * g++ -std=c++20 generic_container_allocator_placement_new.cpp -o generic_container_allocator_placement_new
 *
 * Run:
 * ./generic_container_allocator_placement_new
 *
 * Note:
 * This file uses std::construct_at and std::destroy_at, which are C++20.
 */

class NoDefault {
private:
    int value;

public:
    explicit NoDefault(int v)
        : value(v) {
        cout << "NoDefault constructor: " << value << endl;
    }

    ~NoDefault() {
        cout << "NoDefault destructor: " << value << endl;
    }

    int get() const {
        return value;
    }
};

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

    const string& getName() const {
        return name;
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

    SimpleVector(const SimpleVector&) = delete;
    SimpleVector& operator=(const SimpleVector&) = delete;

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

void rawStorageDemo() {
    cout << "\n=== raw storage demo ===" << endl;

    allocator<NoDefault> alloc;

    NoDefault* p = alloc.allocate(2);

    cout << "Allocated raw storage for 2 NoDefault objects." << endl;
    cout << "No objects exist yet." << endl;

    construct_at(p, 10);
    construct_at(p + 1, 20);

    cout << "p[0] = " << p[0].get() << endl;
    cout << "p[1] = " << p[1].get() << endl;

    destroy_at(p);
    destroy_at(p + 1);

    alloc.deallocate(p, 2);
}

void simpleVectorNoDefaultDemo() {
    cout << "\n=== SimpleVector<NoDefault> demo ===" << endl;

    SimpleVector<NoDefault> v;

    v.emplace_back(10);
    v.emplace_back(20);
    v.emplace_back(30);

    cout << "size = " << v.size()
         << ", capacity = " << v.capacity() << endl;

    cout << "v[1] = " << v[1].get() << endl;
}

void simpleVectorTrackerDemo() {
    cout << "\n=== SimpleVector<Tracker> demo ===" << endl;

    SimpleVector<Tracker> v;

    v.emplace_back("A");
    v.emplace_back("B");
    v.emplace_back("C");

    cout << "size = " << v.size()
         << ", capacity = " << v.capacity() << endl;

    cout << "v[0] = " << v[0].getName() << endl;
}

void moveSimpleVectorDemo() {
    cout << "\n=== move SimpleVector demo ===" << endl;

    SimpleVector<Tracker> v;
    v.emplace_back("move_demo_A");
    v.emplace_back("move_demo_B");

    SimpleVector<Tracker> moved = std::move(v);

    cout << "moved size = " << moved.size() << endl;
    cout << "old v size = " << v.size() << endl;
}

int main() {
    rawStorageDemo();

    simpleVectorNoDefaultDemo();

    simpleVectorTrackerDemo();

    moveSimpleVectorDemo();

    cout << "\n=== end of main ===" << endl;

    return 0;
}