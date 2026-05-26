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
