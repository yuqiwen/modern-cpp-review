#include <atomic>
#include <iostream>
#include <thread>
#include <vector>
using namespace std;

/*
 * Topic:
 * Lock-free basics, CAS, and ABA.
 *
 * Compile:
 * g++ -std=c++20 -O2 -pthread lock_free_cas_aba.cpp \
 *     -o lock_free_cas_aba
 *
 * Run:
 * ./lock_free_cas_aba
 */

void compareExchangeBasicDemo() {
    cout << "\n=== compare_exchange basic demo ===\n";

    atomic<int> state = 5;

    int expected = 0;

    bool success = state.compare_exchange_strong(
        expected,
        10
    );

    cout << boolalpha;
    cout << "first success = " << success << '\n';
    cout << "state = " << state.load() << '\n';
    cout << "expected after failure = "
         << expected << '\n';

    success = state.compare_exchange_strong(
        expected,
        10
    );

    cout << "second success = " << success << '\n';
    cout << "state = " << state.load() << '\n';
    cout << "expected = " << expected << '\n';
}

void casIncrementDemo() {
    cout << "\n=== CAS increment demo ===\n";

    atomic<int> counter = 0;

    auto increment = [&] {
        for (int i = 0; i < 100000; ++i) {
            int current = counter.load(
                memory_order_relaxed
            );

            while (!counter.compare_exchange_weak(
                current,
                current + 1,
                memory_order_relaxed,
                memory_order_relaxed
            )) {
                // On failure, current is replaced
                // with the actual current counter value.
            }
        }
    };

    thread t1(increment);
    thread t2(increment);

    t1.join();
    t2.join();

    cout << "counter = "
         << counter.load()
         << '\n';

    cout << "For simple increments, fetch_add is clearer.\n";
}

struct Node {
    int value;
    Node* next;
};

class DemoLockFreeStack {
private:
    atomic<Node*> head_{nullptr};

    // Educational simplification:
    // removed nodes are not immediately deleted.
    // They are retained until stack destruction to avoid
    // unsafe concurrent memory reclamation.
    vector<Node*> allocatedNodes_;

public:
    void push(int value) {
        Node* node = new Node{
            value,
            nullptr
        };

        allocatedNodes_.push_back(node);

        node->next = head_.load(
            memory_order_relaxed
        );

        while (!head_.compare_exchange_weak(
            node->next,
            node,
            memory_order_release,
            memory_order_relaxed
        )) {
        }
    }

    bool pop(int& result) {
        Node* oldHead = head_.load(
            memory_order_acquire
        );

        while (oldHead != nullptr) {
            Node* next = oldHead->next;

            if (head_.compare_exchange_weak(
                oldHead,
                next,
                memory_order_acquire,
                memory_order_relaxed
            )) {
                result = oldHead->value;

                // Do NOT delete oldHead here.
                // Another thread may still hold a pointer to it.
                return true;
            }

            // Failure updates oldHead with current head.
        }

        return false;
    }

    ~DemoLockFreeStack() {
        // Only safe because all worker threads must already
        // have stopped before the stack is destroyed.
        for (Node* node : allocatedNodes_) {
            delete node;
        }
    }
};

void lockFreeStackDemo() {
    cout << "\n=== educational lock-free stack demo ===\n";

    DemoLockFreeStack stack;

    thread producer1([&] {
        for (int i = 0; i < 5; ++i) {
            stack.push(i);
        }
    });

    thread producer2([&] {
        for (int i = 100; i < 105; ++i) {
            stack.push(i);
        }
    });

    producer1.join();
    producer2.join();

    int value;

    while (stack.pop(value)) {
        cout << "popped " << value << '\n';
    }

    cout << "Nodes were reclaimed only after concurrency ended.\n";
}

void abaExplanationDemo() {
    cout << "\n=== ABA explanation ===\n";

    cout << "Initial head: A\n";
    cout << "Thread 1 reads expected = A and pauses.\n";
    cout << "Thread 2 pops A, pops B, then pushes A again.\n";
    cout << "Head is A again, but the structure changed.\n";
    cout << "Thread 1 CAS may see A == A and miss the history.\n";
}

struct PairState {
    void* pointer;
    unsigned long long version;
};

void taggedPointerExplanationDemo() {
    cout << "\n=== tagged pointer explanation ===\n";

    cout << "Compare (pointer, version), not only pointer.\n";
    cout << "Old state: (A, 1)\n";
    cout << "After changes: (A, 3)\n";
    cout << "Pointer matches, but version does not.\n";
}

void lockFreeQueryDemo() {
    cout << "\n=== is_lock_free demo ===\n";

    atomic<int> intAtomic;
    atomic<long long> longLongAtomic;
    atomic<void*> pointerAtomic;

    cout << boolalpha;

    cout << "atomic<int> lock-free = "
         << intAtomic.is_lock_free()
         << '\n';

    cout << "atomic<long long> lock-free = "
         << longLongAtomic.is_lock_free()
         << '\n';

    cout << "atomic<void*> lock-free = "
         << pointerAtomic.is_lock_free()
         << '\n';

    cout << "Results depend on platform and implementation.\n";
}

void progressGuaranteeDemo() {
    cout << "\n=== progress guarantees ===\n";

    cout << "Blocking: a paused lock owner can block others.\n";
    cout << "Lock-free: system-wide progress is guaranteed.\n";
    cout << "Wait-free: every thread has bounded completion.\n";
}

void performanceWarningDemo() {
    cout << "\n=== performance warning ===\n";

    cout << "Lock-free does not automatically mean faster.\n";
    cout << "CAS retries, cache contention, false sharing,\n";
    cout << "and reclamation overhead may dominate.\n";
}

int main() {
    compareExchangeBasicDemo();

    casIncrementDemo();

    lockFreeStackDemo();

    abaExplanationDemo();

    taggedPointerExplanationDemo();

    lockFreeQueryDemo();

    progressGuaranteeDemo();

    performanceWarningDemo();

    cout << "\n=== end of main ===\n";

    return 0;
}