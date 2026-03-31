# C++26 Rules for Vulkan Android Game Development

> **Compiler**: Clang 19 (NDK r29) · `-std=c++26` · `libc++` (LLVM)
> **Note**: C++26 is standardised but Clang 19 support is partial. Rules below are
> annotated with the actual standard version that introduced each feature. All
> annotated features are confirmed available in Clang 19 + NDK r29 libc++.

---

## Compilation Requirements

- **MANDATORY**: Use `-std=c++26` (Clang 19 supports this flag directly; do not use `-std=c++2c`)
- **REQUIRED**: Enable all warnings: `-Wall -Wextra -Wpedantic -Wshadow -Wconversion`
- **MANDATORY**: Treat warnings as errors: `-Werror`
- **REQUIRED**: Enable sanitisers in debug builds: `-fsanitize=address,undefined`; use `-fsanitize-minimal-runtime` for release-mode UBSan if needed
- **ENFORCED**: `-fno-exceptions` — required for NDK; all error propagation via `std::expected`
- **REQUIRED**: `-fno-rtti` — required for NDK binary size; use `std::variant` + `std::visit` instead of `dynamic_cast`
- **ENFORCED**: `-flto=thin` in release builds for cross-TU inlining (NDK r29 supports ThinLTO)
- **REQUIRED**: `-O2` for release (balanced); `-O3` only for profiled hot paths; `-Oz` for JNI glue and rarely-called code
- **MANDATORY**: `-march=armv8.2-a+dotprod+fp16` to enable Arm v8.2 intrinsics (dot-product, native fp16) on NDK r29 arm64-v8a targets

---

## Language Features by Standard

### C++20 (fully stable in Clang 19)

- **MANDATORY**: `std::span<T>` for all non-owning contiguous buffer views — no raw pointer + size pairs
- **MANDATORY**: `std::array<T, N>` instead of C-style arrays for fixed-size data
- **REQUIRED**: Concepts and `requires` clauses for all generic code — no unconstrained templates
- **ENFORCED**: Designated initialisers (`.member = value`) for all aggregate structs; no positional init lists
- **REQUIRED**: `[[likely]]` / `[[unlikely]]` on branches **after** profiling; never guessed
- **MANDATORY**: `[[nodiscard("reason")]]` on every function returning a status, error, or resource handle
- **ENFORCED**: `constexpr` / `consteval` on all compile-time-evaluable functions
- **REQUIRED**: Three-way comparison (`operator<=>`) for all comparable value types
- **MANDATORY**: Coroutines (`co_await`, `co_yield`, `co_return`) for async asset loading pipelines — use a lightweight coroutine library (e.g. `cppcoro` or a custom task type)
- **ENFORCED**: `std::jthread` + `std::stop_token` for all threads; `std::thread` is **banned**
- **REQUIRED**: `std::atomic<T>` with explicit memory order arguments — `memory_order::acquire/release/seq_cst` as appropriate; never `memory_order::relaxed` for synchronisation
- **ENFORCED**: `std::bit_cast<T>` for type punning; `reinterpret_cast` is **banned** for this use case
- **REQUIRED**: `std::to_underlying(enum_val)` for scoped-enum-to-integer conversion
- **MANDATORY**: `std::source_location::current()` in all logging and assertion macros

### C++23 (fully stable in Clang 17+, confirmed in NDK r29)

- **MANDATORY**: `std::expected<T, E>` / `std::unexpected<E>` for all fallible return values — exceptions are **banned** (`-fno-exceptions`)
- **REQUIRED**: `std::print` / `std::println` for debug/tool output (uses `std::format` formatting, no `printf`/`cout`)
- **ENFORCED**: `std::flat_map<K, V>` and `std::flat_set<T>` (sorted, cache-friendly, contiguous storage) instead of `std::map` / `std::set` for hot-path lookups
- **REQUIRED**: `std::mdspan<T, Extents>` for multi-dimensional buffer views (texture mip data, 2-D game maps)
- **MANDATORY**: `if consteval { ... }` to branch between compile-time and runtime paths inside `constexpr` functions; replaces `std::is_constant_evaluated()`
- **ENFORCED**: `std::unreachable()` to mark logically-unreachable branches (replaces `__builtin_unreachable()`)
- **REQUIRED**: Explicit object parameter (`this auto& self`) for CRTP-like patterns and recursive lambdas; eliminates the CRTP boilerplate entirely
- **MANDATORY**: Multidimensional subscript operator `operator[](i, j, k)` for matrix and grid types
- **ENFORCED**: `std::ranges::to<Container>()` for converting range adaptors to containers; no manual `std::back_insert_iterator` boilerplate
- **REQUIRED**: `std::stacktrace` **only if** the NDK libc++ ships it — verify at configure time; fall back to `unwind.h` + `dladdr` if absent

### C++26 (available in Clang 18–19, confirmed in NDK r29)

- **MANDATORY**: `std::inplace_vector<T, N>` for small, stack-resident dynamic sequences (ECS component scratch lists, frame-local entity queues) — fixed capacity, no heap allocation, no `reserve()`
- **REQUIRED**: Pack indexing (`Args...[I]`, `Types...[I]`) to access variadic template arguments by index without recursive template metaprogramming
- **ENFORCED**: `_` as a placeholder name in structured bindings when a binding is intentionally unused: `auto [result, _] = twoValueReturn();`
- **REQUIRED**: `#embed "path/to/blob.spv"` to embed SPIR-V bytecode or binary assets directly into the executable at compile time, eliminating `AAssetManager` calls for shader data
- **MANDATORY**: `= delete("reason")` on explicitly-deleted functions to document why the function is deleted: `Foo(const Foo&) = delete("Foo is move-only — use std::move");`
- **ENFORCED**: `constexpr` placement new (C++26) for compile-time object construction inside `constexpr` buffers

---

## Forbidden Patterns

| Pattern | Replacement |
|---|---|
| `(int)x`, `int(x)` C-style casts | `static_cast<int>(x)` / `std::bit_cast<int>(x)` |
| `reinterpret_cast` for type punning | `std::bit_cast<T>` |
| `dynamic_cast` | `std::variant` + `std::visit` |
| `std::thread` | `std::jthread` |
| `NULL` | `nullptr` |
| `malloc` / `free` | `std::pmr` allocators, `std::make_unique`, VMA |
| `new T[N]` raw arrays | `std::array<T,N>` / `std::inplace_vector<T,N>` / `std::pmr::vector<T>` |
| `typedef` | `using` |
| `std::endl` | `'\n'` (avoids flush) |
| `using namespace X` in headers | Explicit `X::` qualification |
| `goto` | Structured control flow |
| Manual index `for` loop when ranges work | `std::ranges::for_each`, range-`for` |
| `std::string` in hot paths | `std::string_view` |
| `std::map` / `std::set` | `std::flat_map` / `std::flat_set` |
| `__builtin_unreachable()` | `std::unreachable()` |
| `__builtin_expect(x, 1)` | `[[likely]]` / `[[unlikely]]` |

---

## Vulkan-Specific C++26 Rules

- **MANDATORY**: Use `std::byte` for raw memory buffers and byte-level Vulkan operations
- **REQUIRED**: Use `std::bit_cast<uint32_t>(floatVal)` for shader push-constant type punning
- **ENFORCED**: Use `std::bit_ceil(size)` for power-of-two buffer size alignment
- **REQUIRED**: Use `std::to_underlying(vkFormat)` when storing `VkFormat` in a hash map key or log message
- **MANDATORY**: Use `#embed "shader.spv"` (C++26) to embed compiled SPIR-V at compile time for shaders that do not need runtime hot-reload
- **ENFORCED**: Use `std::inplace_vector<VkWriteDescriptorSet, 16>` for stack-local descriptor write batches; eliminates per-frame heap allocation

## Memory and Allocation

- **MANDATORY**: Use `std::pmr::monotonic_buffer_resource` backed by a per-frame stack arena for all per-frame transient allocations
- **REQUIRED**: Use `std::pmr::unsynchronized_pool_resource` for thread-local scratch allocations (no locking overhead)
- **ENFORCED**: Use `std::pmr::polymorphic_allocator` as the allocator template parameter for ECS component storage containers
- **MANDATORY**: Use `std::vector::reserve()` at construction for containers whose final size is known or estimable; never let a hot-path container reallocate
- **REQUIRED**: Use `std::inplace_vector<T, N>` (C++26) instead of `std::array<T, N>` + a manual size counter when the container needs dynamic size but a fixed capacity

## Code Organisation

- **MANDATORY**: `#pragma once` for all headers
- **REQUIRED**: Include order: (1) C system headers, (2) POSIX / Android NDK headers, (3) third-party headers (Vulkan, VMA, entt), (4) project headers; one blank line between groups
- **ENFORCED**: Alphabetical order within each include group
- **REQUIRED**: Forward declarations in headers to minimise transitive includes
- **MANDATORY**: All project code in `namespace game { }` or a project-specific nested namespace; no global-scope definitions except `main` / `android_main`
- **ENFORCED**: Module-like header structure: one class / concept / set of related free functions per header
