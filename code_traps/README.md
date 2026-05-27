# Code Traps Index

This folder contains small C++ programs that demonstrate common interview and implementation pitfalls.

## Traps

1. [Pointer / Reference / Parameter Passing Demo](./pointer_reference_parameter.cpp)
   Related note: [01. Pointer, Reference, and Parameter Passing](../cheat_sheet/01_pointer_reference_parameter.md)
2. [Object Lifetime / Storage Demo](./object_lifetime_storage.cpp)
   Related note: [02. Object Lifetime and Storage Duration](../cheat_sheet/02_object_lifetime_storage.md)
3. [Constructor / Destructor / RAII Demo](./constructor_destructor_raii.cpp)
   Related note: [03. Constructor, Destructor, and RAII](../cheat_sheet/03_constructor_destructor_raii.md)
4. [Rule of Three / Five / Zero Demo](./rule_of_three_five_zero.cpp)
   Related note: [04. Rule of Three, Rule of Five, and Rule of Zero](../cheat_sheet/04_rule_of_three_five_zero.md)
5. [Copy / Move Call Timing Demo](./copy_move_call_timing.cpp)
   Related note: [05. Copy and Move Call Timing](../cheat_sheet/05_copy_move_call_timing.md)
6. [Const Correctness Demo](./const_correctness.cpp)
   Related note: [06. Const Correctness](../cheat_sheet/06_const_correctness.md)
7. [Smart Pointers Demo](./smart_pointers.cpp)
   Related note: [07. Smart Pointers](../cheat_sheet/07_smart_pointers.md)
8. [Inheritance / Virtual / VTable Demo](./inheritance_virtual_vtable.cpp)
   Related note: [08. Inheritance, Virtual Functions, VTable, and Virtual Destructor](../cheat_sheet/08_inheritance_virtual_vtable.md)
9. [C++ Casts Demo](./cpp_casts.cpp)
   Related note: [09. C++ Casts](../cheat_sheet/09_cpp_casts.md)
10. [STL Containers Overview Demo](./stl_containers_overview.cpp)
    Related note: [10. STL Containers Overview](../cheat_sheet/10_stl_containers_overview.md)
11. [Vector Deep Dive Demo](./vector_deep_dive.cpp)
    Related note: [11. std::vector Deep Dive](../cheat_sheet/11_vector_deep_dive.md)
12. [Iterator Invalidation / unordered_map Rehash Demo](./iterator_invalidation_unordered_map.cpp)
    Related note: [12. Iterator Invalidation and unordered_map Rehash](../cheat_sheet/12_iterator_invalidation_unordered_map.md)
13. [unordered_map Deep Dive Demo](./unordered_map_deep_dive.cpp)
    Related note: [13. unordered_map Deep Dive](../cheat_sheet/13_unordered_map_deep_dive.md)
14. [map and set Deep Dive Demo](./map_set_deep_dive.cpp)
    Related note: [14. map and set Deep Dive](../cheat_sheet/14_map_set_deep_dive.md)

## Suggested Naming Rule

Keep trap names aligned with the related topic when possible:

- `pointer_reference_parameter.cpp`
- `const_correctness_traps.cpp`
- `iterator_invalidation.cpp`

## Suggested Pattern

Each trap file can include:

1. Topic summary
2. Compile command
3. Run command
4. A small reproducible pitfall
5. Expected output or explanation
