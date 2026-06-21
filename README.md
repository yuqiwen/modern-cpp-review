# C++ Interview Prep

This repository is a personal C++ review and interview-prep notebook focused on practical understanding rather than full language coverage.

## Quick Navigation

- [Cheat Sheet Index](./cheat_sheet/README.md)
- [Code Traps Index](./code_traps/README.md)
- [Topic 01: Pointer, Reference, and Parameter Passing](./cheat_sheet/01_pointer_reference_parameter.md)
- [Topic 02: Object Lifetime and Storage Duration](./cheat_sheet/02_object_lifetime_storage.md)
- [Topic 03: Constructor, Destructor, and RAII](./cheat_sheet/03_constructor_destructor_raii.md)
- [Topic 04: Rule of Three, Rule of Five, and Rule of Zero](./cheat_sheet/04_rule_of_three_five_zero.md)
- [Topic 05: Copy and Move Call Timing](./cheat_sheet/05_copy_move_call_timing.md)
- [Topic 06: Const Correctness](./cheat_sheet/06_const_correctness.md)
- [Topic 07: Smart Pointers](./cheat_sheet/07_smart_pointers.md)
- [Topic 08: Inheritance, Virtual Functions, VTable, and Virtual Destructor](./cheat_sheet/08_inheritance_virtual_vtable.md)
- [Topic 09: C++ Casts](./cheat_sheet/09_cpp_casts.md)
- [Topic 10: STL Containers Overview](./cheat_sheet/10_stl_containers_overview.md)
- [Topic 11: std::vector Deep Dive](./cheat_sheet/11_vector_deep_dive.md)
- [Topic 12: Iterator Invalidation and unordered_map Rehash](./cheat_sheet/12_iterator_invalidation_unordered_map.md)
- [Topic 13: unordered_map Deep Dive](./cheat_sheet/13_unordered_map_deep_dive.md)
- [Topic 14: map and set Deep Dive](./cheat_sheet/14_map_set_deep_dive.md)
- [Topic 15: STL Algorithms and Comparators](./cheat_sheet/15_stl_algorithms_comparators.md)
- [Topic 16: priority_queue Comparator](./cheat_sheet/16_priority_queue_comparator.md)
- [Topic 17: Template Basics](./cheat_sheet/17_template_basics.md)
- [Topic 18: Template Type Deduction and Forwarding Reference](./cheat_sheet/18_template_deduction_forwarding.md)
- [Topic 19: Advanced Templates Lite](./cheat_sheet/19_advanced_templates_lite.md)
- [Topic 20: Generic Container, Allocator, Placement New, and Object Lifetime](./cheat_sheet/20_generic_container_allocator_placement_new.md)
- [Topic 21: Generic Container Rule of Five and Exception Safety](./cheat_sheet/21_generic_container_rule_of_five_exception_safety.md)
- [Topic 22: Low-level Memory: malloc/free, new/delete, operator new, Placement New](./cheat_sheet/22_low_level_memory_new_malloc_placement_new.md)
- [Topic 23: Copy Elision, RVO, NRVO, and Return by Value](./cheat_sheet/23_copy_elision_rvo_return_by_value.md)
- [Topic 24: Smart Pointers Deep Dive](./cheat_sheet/24_smart_pointers_deep_dive.md)
- [Topic 25: shared_ptr Control Block](./cheat_sheet/25_shared_ptr_control_block.md)
- [Topic 26: std::string, std::string_view, and C string](./cheat_sheet/26_string_string_view_c_string.md)
- [Topic 27: std::span, Array, Vector, and Views](./cheat_sheet/27_span_array_vector_view.md)
- [Topic 28: RAII, Exception Safety, and Scope Guard](./cheat_sheet/28_raii_exception_safety_scope_guard.md)
- [Topic 29: optional, variant, and any](./cheat_sheet/29_optional_variant_any.md)
- [Topic 30: Concurrency: thread, mutex, lock_guard, atomic](./cheat_sheet/30_concurrency_thread_mutex_atomic.md)
- [Topic 31: condition_variable and Producer-Consumer Queue](./cheat_sheet/31_condition_variable_producer_consumer.md)
- [Topic 32: future, promise, and async](./cheat_sheet/32_future_promise_async.md)
- [Topic 33: Atomic Memory Ordering](./cheat_sheet/33_atomic_memory_order.md)
- [Topic 34: Semaphore, Workers, and Thread Pool Basics](./cheat_sheet/34_semaphore_worker_thread_pool.md)
- [Topic 35: std::latch, std::barrier, and Phase Synchronization](./cheat_sheet/35_latch_barrier_phase_sync.md)
- [Topic 36: std::jthread, stop_token, and Cooperative Cancellation](./cheat_sheet/36_jthread_stop_token_cancellation.md)
- [Topic 37: std::call_once, std::once_flag, and thread_local](./cheat_sheet/37_call_once_thread_local.md)
- [Topic 38: shared_mutex and Reader-Writer Locks](./cheat_sheet/38_shared_mutex_reader_writer_lock.md)
- [Topic 39: False Sharing and Cache Lines](./cheat_sheet/39_false_sharing_cache_line.md)
- [Code Trap: Pointer / Reference / Parameter Demo](./code_traps/pointer_reference_parameter.cpp)
- [Code Trap: Object Lifetime / Storage Demo](./code_traps/object_lifetime_storage.cpp)
- [Code Trap: Constructor / Destructor / RAII Demo](./code_traps/constructor_destructor_raii.cpp)
- [Code Trap: Rule of Three / Five / Zero Demo](./code_traps/rule_of_three_five_zero.cpp)
- [Code Trap: Copy / Move Call Timing Demo](./code_traps/copy_move_call_timing.cpp)
- [Code Trap: Const Correctness Demo](./code_traps/const_correctness.cpp)
- [Code Trap: Smart Pointers Demo](./code_traps/smart_pointers.cpp)
- [Code Trap: Inheritance / Virtual / VTable Demo](./code_traps/inheritance_virtual_vtable.cpp)
- [Code Trap: C++ Casts Demo](./code_traps/cpp_casts.cpp)
- [Code Trap: STL Containers Overview Demo](./code_traps/stl_containers_overview.cpp)
- [Code Trap: Vector Deep Dive Demo](./code_traps/vector_deep_dive.cpp)
- [Code Trap: Iterator Invalidation / unordered_map Rehash Demo](./code_traps/iterator_invalidation_unordered_map.cpp)
- [Code Trap: unordered_map Deep Dive Demo](./code_traps/unordered_map_deep_dive.cpp)
- [Code Trap: map and set Deep Dive Demo](./code_traps/map_set_deep_dive.cpp)
- [Code Trap: STL Algorithms / Comparators Demo](./code_traps/stl_algorithms_comparators.cpp)
- [Code Trap: priority_queue Comparator Demo](./code_traps/priority_queue_comparator.cpp)
- [Code Trap: Template Basics Demo](./code_traps/template_basics.cpp)
- [Code Trap: Template Deduction / Forwarding Demo](./code_traps/template_deduction_forwarding.cpp)
- [Code Trap: Advanced Templates Lite Demo](./code_traps/advanced_templates_lite.cpp)
- [Code Trap: Generic Container / Allocator / Placement New Demo](./code_traps/generic_container_allocator_placement_new.cpp)
- [Code Trap: Generic Container Rule of Five Demo](./code_traps/generic_container_rule_of_five.cpp)
- [Code Trap: Low-level Memory Demo](./code_traps/low_level_memory_new_malloc_placement_new.cpp)
- [Code Trap: Copy Elision / RVO Demo](./code_traps/copy_elision_rvo.cpp)
- [Code Trap: Smart Pointers Deep Dive Demo](./code_traps/smart_pointers_deep_dive.cpp)
- [Code Trap: shared_ptr Control Block Demo](./code_traps/shared_ptr_control_block.cpp)
- [Code Trap: string / string_view / C string Demo](./code_traps/string_string_view_c_string.cpp)
- [Code Trap: span / array / vector view Demo](./code_traps/span_array_vector_view.cpp)
- [Code Trap: RAII / Exception Safety Demo](./code_traps/raii_exception_safety.cpp)
- [Code Trap: optional / variant / any Demo](./code_traps/optional_variant_any.cpp)
- [Code Trap: concurrency / thread / mutex / atomic Demo](./code_traps/concurrency_thread_mutex_atomic.cpp)
- [Code Trap: condition_variable / producer-consumer Demo](./code_traps/condition_variable_producer_consumer.cpp)
- [Code Trap: future / promise / async Demo](./code_traps/future_promise_async.cpp)
- [Code Trap: atomic memory order Demo](./code_traps/atomic_memory_order.cpp)
- [Code Trap: semaphore / worker / thread pool Demo](./code_traps/semaphore_worker_thread_pool.cpp)
- [Code Trap: latch / barrier / phase sync Demo](./code_traps/latch_barrier_phase_sync.cpp)
- [Code Trap: jthread / stop_token / cancellation Demo](./code_traps/jthread_stop_token_cancellation.cpp)
- [Code Trap: call_once / thread_local Demo](./code_traps/call_once_thread_local.cpp)
- [Code Trap: shared_mutex / reader-writer lock Demo](./code_traps/shared_mutex_reader_writer_lock.cpp)
- [Code Trap: false sharing Demo](./code_traps/false_sharing.cpp)

## Repository Structure

- `cheat_sheet/`: topic-based notes with core ideas, traps, and examples
- `code_traps/`: small C++ programs that demonstrate common pitfalls
- `interview_answers/`: concise answers to common C++ interview questions
- `practice/`: implementation practice for data structures, concurrency, and system design components

## Code Traps

1. [Pointer / Reference / Parameter Passing Demo](./code_traps/pointer_reference_parameter.cpp)
   Related note: [01. Pointer, Reference, and Parameter Passing](./cheat_sheet/01_pointer_reference_parameter.md)
2. [Object Lifetime / Storage Demo](./code_traps/object_lifetime_storage.cpp)
   Related note: [02. Object Lifetime and Storage Duration](./cheat_sheet/02_object_lifetime_storage.md)
3. [Constructor / Destructor / RAII Demo](./code_traps/constructor_destructor_raii.cpp)
   Related note: [03. Constructor, Destructor, and RAII](./cheat_sheet/03_constructor_destructor_raii.md)
4. [Rule of Three / Five / Zero Demo](./code_traps/rule_of_three_five_zero.cpp)
   Related note: [04. Rule of Three, Rule of Five, and Rule of Zero](./cheat_sheet/04_rule_of_three_five_zero.md)
5. [Copy / Move Call Timing Demo](./code_traps/copy_move_call_timing.cpp)
   Related note: [05. Copy and Move Call Timing](./cheat_sheet/05_copy_move_call_timing.md)
6. [Const Correctness Demo](./code_traps/const_correctness.cpp)
   Related note: [06. Const Correctness](./cheat_sheet/06_const_correctness.md)
7. [Smart Pointers Demo](./code_traps/smart_pointers.cpp)
   Related note: [07. Smart Pointers](./cheat_sheet/07_smart_pointers.md)
8. [Inheritance / Virtual / VTable Demo](./code_traps/inheritance_virtual_vtable.cpp)
   Related note: [08. Inheritance, Virtual Functions, VTable, and Virtual Destructor](./cheat_sheet/08_inheritance_virtual_vtable.md)
9. [C++ Casts Demo](./code_traps/cpp_casts.cpp)
   Related note: [09. C++ Casts](./cheat_sheet/09_cpp_casts.md)
10. [STL Containers Overview Demo](./code_traps/stl_containers_overview.cpp)
    Related note: [10. STL Containers Overview](./cheat_sheet/10_stl_containers_overview.md)
11. [Vector Deep Dive Demo](./code_traps/vector_deep_dive.cpp)
    Related note: [11. std::vector Deep Dive](./cheat_sheet/11_vector_deep_dive.md)
12. [Iterator Invalidation / unordered_map Rehash Demo](./code_traps/iterator_invalidation_unordered_map.cpp)
    Related note: [12. Iterator Invalidation and unordered_map Rehash](./cheat_sheet/12_iterator_invalidation_unordered_map.md)
13. [unordered_map Deep Dive Demo](./code_traps/unordered_map_deep_dive.cpp)
    Related note: [13. unordered_map Deep Dive](./cheat_sheet/13_unordered_map_deep_dive.md)
14. [map and set Deep Dive Demo](./code_traps/map_set_deep_dive.cpp)
    Related note: [14. map and set Deep Dive](./cheat_sheet/14_map_set_deep_dive.md)
15. [STL Algorithms / Comparators Demo](./code_traps/stl_algorithms_comparators.cpp)
    Related note: [15. STL Algorithms and Comparators](./cheat_sheet/15_stl_algorithms_comparators.md)
16. [priority_queue Comparator Demo](./code_traps/priority_queue_comparator.cpp)
    Related note: [16. priority_queue Comparator](./cheat_sheet/16_priority_queue_comparator.md)
17. [Template Basics Demo](./code_traps/template_basics.cpp)
    Related note: [17. Template Basics](./cheat_sheet/17_template_basics.md)
18. [Template Deduction / Forwarding Demo](./code_traps/template_deduction_forwarding.cpp)
    Related note: [18. Template Type Deduction and Forwarding Reference](./cheat_sheet/18_template_deduction_forwarding.md)
19. [Advanced Templates Lite Demo](./code_traps/advanced_templates_lite.cpp)
    Related note: [19. Advanced Templates Lite](./cheat_sheet/19_advanced_templates_lite.md)
20. [Generic Container / Allocator / Placement New Demo](./code_traps/generic_container_allocator_placement_new.cpp)
    Related note: [20. Generic Container, Allocator, Placement New, and Object Lifetime](./cheat_sheet/20_generic_container_allocator_placement_new.md)
21. [Generic Container Rule of Five Demo](./code_traps/generic_container_rule_of_five.cpp)
    Related note: [21. Generic Container Rule of Five and Exception Safety](./cheat_sheet/21_generic_container_rule_of_five_exception_safety.md)
22. [Low-level Memory Demo](./code_traps/low_level_memory_new_malloc_placement_new.cpp)
    Related note: [22. Low-level Memory: malloc/free, new/delete, operator new, Placement New](./cheat_sheet/22_low_level_memory_new_malloc_placement_new.md)
23. [Copy Elision / RVO Demo](./code_traps/copy_elision_rvo.cpp)
    Related note: [23. Copy Elision, RVO, NRVO, and Return by Value](./cheat_sheet/23_copy_elision_rvo_return_by_value.md)
24. [Smart Pointers Deep Dive Demo](./code_traps/smart_pointers_deep_dive.cpp)
    Related note: [24. Smart Pointers Deep Dive](./cheat_sheet/24_smart_pointers_deep_dive.md)
25. [shared_ptr Control Block Demo](./code_traps/shared_ptr_control_block.cpp)
    Related note: [25. shared_ptr Control Block](./cheat_sheet/25_shared_ptr_control_block.md)
26. [string / string_view / C string Demo](./code_traps/string_string_view_c_string.cpp)
    Related note: [26. std::string, std::string_view, and C string](./cheat_sheet/26_string_string_view_c_string.md)
27. [span / array / vector view Demo](./code_traps/span_array_vector_view.cpp)
    Related note: [27. std::span, Array, Vector, and Views](./cheat_sheet/27_span_array_vector_view.md)
28. [RAII / Exception Safety Demo](./code_traps/raii_exception_safety.cpp)
    Related note: [28. RAII, Exception Safety, and Scope Guard](./cheat_sheet/28_raii_exception_safety_scope_guard.md)
29. [optional / variant / any Demo](./code_traps/optional_variant_any.cpp)
    Related note: [29. optional, variant, and any](./cheat_sheet/29_optional_variant_any.md)
30. [concurrency / thread / mutex / atomic Demo](./code_traps/concurrency_thread_mutex_atomic.cpp)
    Related note: [30. Concurrency: thread, mutex, lock_guard, atomic](./cheat_sheet/30_concurrency_thread_mutex_atomic.md)
31. [condition_variable / producer-consumer Demo](./code_traps/condition_variable_producer_consumer.cpp)
    Related note: [31. condition_variable and Producer-Consumer Queue](./cheat_sheet/31_condition_variable_producer_consumer.md)
32. [future / promise / async Demo](./code_traps/future_promise_async.cpp)
    Related note: [32. future, promise, and async](./cheat_sheet/32_future_promise_async.md)
33. [atomic memory order Demo](./code_traps/atomic_memory_order.cpp)
    Related note: [33. Atomic Memory Ordering](./cheat_sheet/33_atomic_memory_order.md)
34. [semaphore / worker / thread pool Demo](./code_traps/semaphore_worker_thread_pool.cpp)
    Related note: [34. Semaphore, Workers, and Thread Pool Basics](./cheat_sheet/34_semaphore_worker_thread_pool.md)
35. [latch / barrier / phase sync Demo](./code_traps/latch_barrier_phase_sync.cpp)
    Related note: [35. std::latch, std::barrier, and Phase Synchronization](./cheat_sheet/35_latch_barrier_phase_sync.md)
36. [jthread / stop_token / cancellation Demo](./code_traps/jthread_stop_token_cancellation.cpp)
    Related note: [36. std::jthread, stop_token, and Cooperative Cancellation](./cheat_sheet/36_jthread_stop_token_cancellation.md)
37. [call_once / thread_local Demo](./code_traps/call_once_thread_local.cpp)
    Related note: [37. std::call_once, std::once_flag, and thread_local](./cheat_sheet/37_call_once_thread_local.md)
38. [shared_mutex / reader-writer lock Demo](./code_traps/shared_mutex_reader_writer_lock.cpp)
    Related note: [38. shared_mutex and Reader-Writer Locks](./cheat_sheet/38_shared_mutex_reader_writer_lock.md)
39. [false sharing Demo](./code_traps/false_sharing.cpp)
    Related note: [39. False Sharing and Cache Lines](./cheat_sheet/39_false_sharing_cache_line.md)

## Planned Topics

1. Object, lifetime, and storage duration
2. Pointer vs reference
3. Parameter passing
4. Const correctness
5. Constructor, destructor, copy, and move
6. RAII and smart pointers
7. Inheritance, virtual functions, and vtable
8. Templates
9. STL containers
10. Iterator invalidation
11. `unordered_map` and rehashing
12. Multithreading
13. Atomic operations and memory ordering
14. Performance, cache locality, and alignment

## Suggested Writing Pattern

Each topic can follow this structure:

1. Core idea
2. Common interview questions
3. Code traps
4. Best interview answer
5. Practice examples
