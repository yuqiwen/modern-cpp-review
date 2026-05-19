# Cheat Sheet Index

This folder contains topic-based C++ review notes.

## Topics

1. [01. Pointer, Reference, and Parameter Passing](./01_pointer_reference_parameter.md)
2. [02. Object Lifetime and Storage Duration](./02_object_lifetime_storage.md)
3. [03. Constructor, Destructor, and RAII](./03_constructor_destructor_raii.md)
4. [04. Rule of Three, Rule of Five, and Rule of Zero](./04_rule_of_three_five_zero.md)
5. [05. Copy and Move Call Timing](./05_copy_move_call_timing.md)
6. [06. Const Correctness](./06_const_correctness.md)

## Related Code Traps

1. [Pointer / Reference / Parameter Passing Demo](../code_traps/pointer_reference_parameter.cpp)
2. [Object Lifetime / Storage Demo](../code_traps/object_lifetime_storage.cpp)
3. [Constructor / Destructor / RAII Demo](../code_traps/constructor_destructor_raii.cpp)
4. [Rule of Three / Five / Zero Demo](../code_traps/rule_of_three_five_zero.cpp)
5. [Copy / Move Call Timing Demo](../code_traps/copy_move_call_timing.cpp)
6. [Const Correctness Demo](../code_traps/const_correctness.cpp)

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
