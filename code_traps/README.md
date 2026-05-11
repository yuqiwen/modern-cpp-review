# Code Traps Index

This folder contains small C++ programs that demonstrate common interview and implementation pitfalls.

## Traps

1. [Pointer / Reference / Parameter Passing Demo](./pointer_reference_parameter.cpp)
   Related note: [01. Pointer, Reference, and Parameter Passing](../cheat_sheet/01_pointer_reference_parameter.md)
2. [Object Lifetime / Storage Demo](./object_lifetime_storage.cpp)
   Related note: [02. Object Lifetime and Storage Duration](../cheat_sheet/02_object_lifetime_storage.md)
3. [Constructor / Destructor / RAII Demo](./constructor_destructor_raii.cpp)
   Related note: [03. Constructor, Destructor, and RAII](../cheat_sheet/03_constructor_destructor_raii.md)

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
