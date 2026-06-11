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
15. [STL Algorithms / Comparators Demo](./stl_algorithms_comparators.cpp)
    Related note: [15. STL Algorithms and Comparators](../cheat_sheet/15_stl_algorithms_comparators.md)
16. [priority_queue Comparator Demo](./priority_queue_comparator.cpp)
    Related note: [16. priority_queue Comparator](../cheat_sheet/16_priority_queue_comparator.md)
17. [Template Basics Demo](./template_basics.cpp)
    Related note: [17. Template Basics](../cheat_sheet/17_template_basics.md)
18. [Template Deduction / Forwarding Demo](./template_deduction_forwarding.cpp)
    Related note: [18. Template Type Deduction and Forwarding Reference](../cheat_sheet/18_template_deduction_forwarding.md)
19. [Advanced Templates Lite Demo](./advanced_templates_lite.cpp)
    Related note: [19. Advanced Templates Lite](../cheat_sheet/19_advanced_templates_lite.md)
20. [Generic Container / Allocator / Placement New Demo](./generic_container_allocator_placement_new.cpp)
    Related note: [20. Generic Container, Allocator, Placement New, and Object Lifetime](../cheat_sheet/20_generic_container_allocator_placement_new.md)
21. [Generic Container Rule of Five Demo](./generic_container_rule_of_five.cpp)
    Related note: [21. Generic Container Rule of Five and Exception Safety](../cheat_sheet/21_generic_container_rule_of_five_exception_safety.md)
22. [Low-level Memory Demo](./low_level_memory_new_malloc_placement_new.cpp)
    Related note: [22. Low-level Memory: malloc/free, new/delete, operator new, Placement New](../cheat_sheet/22_low_level_memory_new_malloc_placement_new.md)
23. [Copy Elision / RVO Demo](./copy_elision_rvo.cpp)
    Related note: [23. Copy Elision, RVO, NRVO, and Return by Value](../cheat_sheet/23_copy_elision_rvo_return_by_value.md)
24. [Smart Pointers Deep Dive Demo](./smart_pointers_deep_dive.cpp)
    Related note: [24. Smart Pointers Deep Dive](../cheat_sheet/24_smart_pointers_deep_dive.md)
25. [shared_ptr Control Block Demo](./shared_ptr_control_block.cpp)
    Related note: [25. shared_ptr Control Block](../cheat_sheet/25_shared_ptr_control_block.md)
26. [string / string_view / C string Demo](./string_string_view_c_string.cpp)
    Related note: [26. std::string, std::string_view, and C string](../cheat_sheet/26_string_string_view_c_string.md)
27. [span / array / vector view Demo](./span_array_vector_view.cpp)
    Related note: [27. std::span, Array, Vector, and Views](../cheat_sheet/27_span_array_vector_view.md)
28. [RAII / Exception Safety Demo](./raii_exception_safety.cpp)
    Related note: [28. RAII, Exception Safety, and Scope Guard](../cheat_sheet/28_raii_exception_safety_scope_guard.md)
29. [optional / variant / any Demo](./optional_variant_any.cpp)
    Related note: [29. optional, variant, and any](../cheat_sheet/29_optional_variant_any.md)

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
