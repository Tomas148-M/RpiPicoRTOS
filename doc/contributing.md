Formatting
All the source code in this repository is automatically formatted:

C/C++ files using [](https://clang.llvm.org/docs/ClangFormat.html),
Python files using [yapf](https://github.com/google/yapf),
and CMake files using [cmake-format](https://github.com/cheshirekow/cmake_format).
Running ./utils/bootstrap.py installs the prek Git hooks for you, so all your future commits will be formatted appropriately. Our build server automatically rejects improperly formatted pull requests. To run the checks manually, use prek run -c .pre-commit-config.yaml. The -c .pre-commit-config.yaml flag pins prek to the root config so it does not descend into the nested configs of vendored libraries under lib/; bootstrap bakes the same flag into the installed hook.

Files: Include Guards
Use the #pragma once as a file guard. Do not use the #ifdef FILE_X, #define FILE_X, #endif pattern.


Code Style: C/C++ Naming Conventions
Types & Classes are in PascalCase.
Global constants in SCREAMING_CASE
Variables (local, class, etc), class-level constants, enum class items, methods and namespaces are in snake_case.
File names are in snake_case.cpp (even if the only thing the file contains is a class named in PascalCase).
Types never end with a '_t'.
Code Style: Coding Conventions
(recommendation) Prefer using anonymous namespaces instead of static cpp members.
# Contributing

Read [README.md](../README.md) for setup, build tasks, and hardware checks.
Repository-specific coding and validation rules are maintained in
[.codex/codex.md](../.codex/codex.md).

Keep changes focused, preserve unrelated work, and update documentation when
behavior changes. Include the checks performed and any hardware validation still
needed when submitting a change. Never include real Wi-Fi credentials.
