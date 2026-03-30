SecretEngine – LLM Coding Rules & Discipline
0️⃣ Prime Directive (non-negotiable)

LLMs are assistants, not architects.
Architecture, ownership, and final decisions are always human.

If an LLM suggestion conflicts with:

performance

modularity

plugin boundaries

mobile constraints

➡️ Reject it immediately.

1️⃣ LLM Usage Scope (what LLMs ARE allowed to do)

LLMs MAY be used for:

✅ Boilerplate generation
✅ Interface definitions (from specs you provide)
✅ Repetitive code (serialization, bindings, enums)
✅ Documentation drafts
✅ Refactoring within a single module
✅ Test scaffolding
✅ Build scripts templates
✅ Comment cleanup
✅ Naming suggestions

LLMs must never invent architecture.

2️⃣ What LLMs are FORBIDDEN to decide

LLMs must NOT:

❌ Define engine architecture
❌ Choose rendering techniques
❌ Introduce dependencies
❌ Decide threading models
❌ Design memory layouts
❌ Change folder structure
❌ Add “helpful” abstractions
❌ Add frameworks
❌ Add hidden globals
❌ Optimize “automatically”

Any of these = hard rejection.

3️⃣ Strict prompting rule (mandatory)

Every LLM request MUST include:

Context

Constraints

Scope

Explicit boundaries

Example (correct)

“Generate a C++20 interface for an input plugin.
Constraints: no STL allocations, no platform code, no static globals.
Scope: interface only, no implementation.”

Example (wrong)

“Create an input system for my engine”

Wrong prompts create engine cancer.

4️⃣ File ownership & responsibility

Every file must clearly answer:

Who owns this?

What layer is this?

What plugin is this for?

Mandatory header comment (ALL files)
// SecretEngine
// Module: <core | plugin-name>
// Responsibility: <single sentence>
// Dependencies: <explicit list>


If an LLM generates a file without this, reject it.

5️⃣ Core vs Plugin enforcement rules
Core rules

Core cannot include plugin headers

Core cannot include platform headers

Core cannot allocate GPU resources

Core cannot talk to OS

Core cannot assume Vulkan exists

If an LLM violates this → delete output.

Plugin rules

Plugins depend on core

Plugins never depend on other plugins directly

Communication via interfaces only

No plugin may assume it is “the only one”

6️⃣ Interfaces-first rule

LLMs may generate:

✔ Interfaces
✔ Abstract base classes
✔ POD data structures

LLMs may NOT generate:
❌ Concrete implementations unless explicitly asked

Order is mandatory:

Interface

Review

Approve

Implementation (optional)

7️⃣ One responsibility per file (strict)

LLMs love to:

merge concerns

add helpers

sneak utilities

This is forbidden.

Rule:

If a file needs “and”, it’s wrong.

Example:

❌ RendererAndShaderManager.cpp

✅ Renderer.cpp

✅ ShaderManager.cpp

8️⃣ No “helpful abstractions”

LLMs often introduce:

managers of managers

factories of factories

wrapper layers “for safety”

These are explicitly banned.

Only abstractions that exist in scope.md are allowed.

9️⃣ Performance assumptions rule

LLMs must assume:

mobile GPUs

low memory

thermal throttling

30–60 fps targets

zero exceptions

minimal heap usage

If LLM code assumes:

unlimited RAM

desktop GPUs

STL-heavy patterns
- Implicit heap usage
- Non-FDA cross-thread communication

➡️ Reject.

**Reference**: See [Performance Guidelines](../guides/PERFORMANCE_GUIDELINES.md) for technical implementation rules.

🔟 Memory rules (very important)

LLMs must follow:

No hidden allocations

No new in hot paths

No shared_ptr in runtime

No RTTI reliance

No exceptions in engine core

If needed:

explicit allocators

arenas

handles, not pointers

1️⃣1️⃣ Error handling discipline

Allowed:

return codes

result structs

explicit error channels

Forbidden:

throwing exceptions

logging as control flow

silent failures

LLMs must never swallow errors.

1️⃣2️⃣ Logging rules

LLMs must:

use engine logging macros only

never print directly

never log inside hot loops

Debug logs must be removable at build time.

1️⃣3️⃣ Build system discipline

LLMs:

may write CMake templates

may NOT change toolchains

may NOT add build-time dependencies

may NOT introduce scripting languages

Android builds must stay:

fast

incremental

minimal

1️⃣4️⃣ Asset & data rules

LLMs must assume:

binary cooked assets

no runtime JSON parsing

no runtime GLTF loading

no runtime shader compilation

If LLM suggests otherwise → reject.

1️⃣5️⃣ Renderer-specific LLM constraints

If working on renderer-related code, LLM must:

assume Vulkan only

avoid API-specific logic leaking into core

never assume desktop extensions

avoid dynamic state explosions

1️⃣6️⃣ Multiplayer & Rust boundary rules

LLMs must treat Rust modules as:

separate processes or libraries

strict API boundaries

message-based interfaces

No shared memory fantasies.

1️⃣7️⃣ Review checklist (mandatory after every LLM output)

Before accepting any LLM code, ask:

Does it respect plugin boundaries?

Does it allocate memory implicitly?

Is responsibility single & clear?

Does it assume desktop?

Can I remove this file without breaking core?

Would this survive 2 years of refactors?

If any answer is “no” → reject.

### 19. **FDA & Communication Discipline**

All high-frequency communication between plugins (Input -> Logic, Logic -> Render) MUST use the **Fast Data Architecture (FDA)**.

- **The Pipe:** `Fast::UltraRingBuffer` (Lock-free SPSC).
- **The Payload:** `Fast::UltraPacket` (8-byte bit-packed).
- **The Rule:** If it happens every frame, it must be an 8-byte packet. No exceptions.
- **Modularity:** Core defines the primitives; Plugins define the specific packet layouts and builders.

---
**Document Version**: 1.2
**Last Updated**: 2026-02-04
**Status**: FDA Standard Verified ✅

1️⃣8️⃣ Philosophy reminder (pin this)

A small, boring, correct engine beats a clever broken one.
Discipline beats features.