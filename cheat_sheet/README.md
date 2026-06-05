# Cheat Sheet Index

This folder contains topic-based C++ review notes.

## Topics

1. [01. Pointer, Reference, and Parameter Passing](./01_pointer_reference_parameter.md)
2. [02. Object Lifetime and Storage Duration](./02_object_lifetime_storage.md)
3. [03. Constructor, Destructor, and RAII](./03_constructor_destructor_raii.md)
4. [04. Rule of Three, Rule of Five, and Rule of Zero](./04_rule_of_three_five_zero.md)
5. [05. Copy and Move Call Timing](./05_copy_move_call_timing.md)
6. [06. Const Correctness](./06_const_correctness.md)
7. [07. Smart Pointers](./07_smart_pointers.md)
8. [08. Inheritance, Virtual Functions, VTable, and Virtual Destructor](./08_inheritance_virtual_vtable.md)
9. [09. C++ Casts](./09_cpp_casts.md)
10. [10. STL Containers Overview](./10_stl_containers_overview.md)
11. [11. std::vector Deep Dive](./11_vector_deep_dive.md)
12. [12. Iterator Invalidation and unordered_map Rehash](./12_iterator_invalidation_unordered_map.md)
13. [13. unordered_map Deep Dive](./13_unordered_map_deep_dive.md)
14. [14. map and set Deep Dive](./14_map_set_deep_dive.md)
15. [15. STL Algorithms and Comparators](./15_stl_algorithms_comparators.md)
16. [16. priority_queue Comparator](./16_priority_queue_comparator.md)
17. [17. Template Basics](./17_template_basics.md)
18. [18. Template Type Deduction and Forwarding Reference](./18_template_deduction_forwarding.md)
19. [19. Advanced Templates Lite](./19_advanced_templates_lite.md)
20. [20. Generic Container, Allocator, Placement New, and Object Lifetime](./20_generic_container_allocator_placement_new.md)
21. [21. Generic Container Rule of Five and Exception Safety](./21_generic_container_rule_of_five_exception_safety.md)
22. [22. Low-level Memory: malloc/free, new/delete, operator new, Placement New](./22_low_level_memory_new_malloc_placement_new.md)
23. [23. Copy Elision, RVO, NRVO, and Return by Value](./23_copy_elision_rvo_return_by_value.md)

## Related Code Traps

1. [Pointer / Reference / Parameter Passing Demo](../code_traps/pointer_reference_parameter.cpp)
2. [Object Lifetime / Storage Demo](../code_traps/object_lifetime_storage.cpp)
3. [Constructor / Destructor / RAII Demo](../code_traps/constructor_destructor_raii.cpp)
4. [Rule of Three / Five / Zero Demo](../code_traps/rule_of_three_five_zero.cpp)
5. [Copy / Move Call Timing Demo](../code_traps/copy_move_call_timing.cpp)
6. [Const Correctness Demo](../code_traps/const_correctness.cpp)
7. [Smart Pointers Demo](../code_traps/smart_pointers.cpp)
8. [Inheritance / Virtual / VTable Demo](../code_traps/inheritance_virtual_vtable.cpp)
9. [C++ Casts Demo](../code_traps/cpp_casts.cpp)
10. [STL Containers Overview Demo](../code_traps/stl_containers_overview.cpp)
11. [Vector Deep Dive Demo](../code_traps/vector_deep_dive.cpp)
12. [Iterator Invalidation / unordered_map Rehash Demo](../code_traps/iterator_invalidation_unordered_map.cpp)
13. [unordered_map Deep Dive Demo](../code_traps/unordered_map_deep_dive.cpp)
14. [map and set Deep Dive Demo](../code_traps/map_set_deep_dive.cpp)
15. [STL Algorithms / Comparators Demo](../code_traps/stl_algorithms_comparators.cpp)
16. [priority_queue Comparator Demo](../code_traps/priority_queue_comparator.cpp)
17. [Template Basics Demo](../code_traps/template_basics.cpp)
18. [Template Deduction / Forwarding Demo](../code_traps/template_deduction_forwarding.cpp)
19. [Advanced Templates Lite Demo](../code_traps/advanced_templates_lite.cpp)
20. [Generic Container / Allocator / Placement New Demo](../code_traps/generic_container_allocator_placement_new.cpp)
21. [Generic Container Rule of Five Demo](../code_traps/generic_container_rule_of_five.cpp)
22. [Low-level Memory Demo](../code_traps/low_level_memory_new_malloc_placement_new.cpp)
23. [Copy Elision / RVO Demo](../code_traps/copy_elision_rvo.cpp)

## Suggested Naming Rule

Use a numeric prefix so topics stay ordered on GitHub:

- `01_pointer_reference_parameter.md`
- `02_const_correctness.md`
- `03_raii_and_smart_pointers.md`

## Suggested Per-Topic Template

Copy this structure when adding a new page:

```md
# Topic Title

## Table of Contents

- [Core Idea](#1-core-idea)
- [Common Trap](#2-common-trap)
- [Interview Questions](#3-interview-questions)
- [Key Takeaways](#4-key-takeaways)
```
