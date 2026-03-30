SecretEngine – Design Principles (Frozen)

These principles are binding.
Any code, plugin, tool, or LLM output that violates them is rejected — no exceptions.

0️⃣ Prime Identity

SecretEngine is:

Mobile-first

Performance-first

Data-driven

Plugin-based

Renderer-agnostic at core

Built to ship games, not demos

Everything else is secondary.

1️⃣ Single Responsibility Above All

Every module, file, class, and function must have exactly ONE reason to change.

If a file grows because of convenience → it is wrong

If a class “helps” another system → it is wrong

If logic and data mix unnecessarily → it is wrong

Boring code is correct code.

2️⃣ Core Is Sacred (Immutable Layer)
Core rules:

Core defines what is possible

Plugins define how it is done

Core:

owns interfaces

owns contracts

owns lifecycle

owns data flow

Core never:

renders

talks to OS

reads raw assets

handles devices

assumes a platform

If core changes frequently, the design failed.

3️⃣ Plugins Are Replaceable, Not Optional

A plugin must assume it can be removed, replaced, or disabled.

Therefore:

No plugin may assume it is unique

No plugin may talk to another plugin directly

All interaction goes through core interfaces

If two plugins need to communicate, core mediates.

4️⃣ Interfaces Before Implementations

No system exists until its interface exists.

Rules:

Interface is designed first

Interface is reviewed

Interface is frozen

Only then implementation starts

Implementations are disposable.
Interfaces are long-term commitments.

5️⃣ Data Over Code (Always)

SecretEngine prefers:

Data

Tables

Configs

Descriptors

Tags

Flags

Over:

Hardcoded logic

Virtual chains

Inheritance trees

“Smart” behavior

Data scales. Logic ossifies.

6️⃣ Authoring ≠ Runtime

Humans write JSON. Machines run binary.

Rules:

JSON is authoring only

Runtime never parses JSON

Runtime never loads raw assets

Runtime consumes cooked binary blobs only

If runtime touches authoring formats → reject.

7️⃣ Performance Is a Design Constraint, Not an Optimization

Performance is considered:

before writing code

before choosing abstractions

before accepting LLM output

Assumptions:

mobile GPUs

thermal throttling

limited bandwidth

limited memory

long play sessions

If something is fast “later”, it is slow now.

8️⃣ Mobile First, Desktop Benefits Automatically

If it runs well on mobile, it will scream on desktop.

Rules:

Avoid overdraw

Avoid state changes

Batch aggressively

Minimize memory traffic

Design for tilers (mobile GPUs)

Desktop is not a design target — it is a bonus.

9️⃣ Rendering Is a Service, Not the Engine

Renderer:

consumes scene data

produces pixels

knows nothing about gameplay

Gameplay:

never queries renderer

never depends on GPU state

This allows:

renderer swapping

headless servers

offline simulations

🔟 Vulkan Is a Tool, Not an Identity

Vulkan is:

the chosen backend

not the engine’s personality

Core does not know Vulkan exists.
Renderer plugins do.

1️⃣1️⃣ Instancing Is the Default

Assumptions:

Multiple objects share meshes

Multiple objects share materials

Per-instance data is cheap

Drawcalls are expensive

Everything is designed assuming instancing first.

1️⃣2️⃣ Visibility Is Binary, Not Continuous

Objects are:

visible

hidden

culled

occluded

Visibility is decided:

early

aggressively

cheaply

Late decisions are expensive.

1️⃣3️⃣ LOD Is Mandatory, Not Optional

Rules:

Mesh LODs exist or are generated

Texture LODs exist or are generated

Shader complexity scales with distance

If content has no LOD strategy → it is invalid content.

1️⃣4️⃣ Texture Strategy Is Hybrid but Controlled

Principles:

Atlases reduce drawcalls

Individual textures preserve flexibility

Cooker decides, not artists

Runtime never repacks textures

Artists focus on quality.
Cooker enforces performance.

1️⃣5️⃣ Input Latency Is Sacred

Input system rules:

Input is sampled early

Mapped before simulation

Never blocked by UI

Never delayed by rendering

Gameplay feel > visuals.

1️⃣6️⃣ Determinism Where It Matters

Multiplayer simulation prefers determinism

Server logic must be reproducible

Floating-point chaos is contained

Not everything must be deterministic — only what matters.

1️⃣7️⃣ Networking Is Infrastructure, Not Gameplay

Networking:

transmits state

synchronizes events

does not own game rules

Game logic does not depend on network timing.

1️⃣8️⃣ Tooling Is Part of the Engine

Cooker, validators, converters are:

first-class citizens

versioned

tested

maintained

A weak toolchain kills engines faster than bad rendering.

1️⃣9️⃣ Build Fast or Die

Rules:

Debug builds must be fast

Android iteration must be seconds, not minutes

Hot reload where possible

Incremental builds always preferred

If iteration is slow, the engine will stall.

2️⃣0️⃣ Explicit Over Implicit

SecretEngine prefers:

explicit lifetimes

explicit ownership

explicit dependencies

explicit costs

Magic is forbidden.

2️⃣1️⃣ Minimal Surface Area

Every system exposes:

the smallest API possible

no convenience leaks

no future-proofing guesses

You can add APIs later.
You cannot easily remove them.

2️⃣2️⃣ Stability Beats Cleverness

Clever code impresses today.
Stable code ships games for years.

2️⃣3️⃣ Solo-Founder Reality Check

This engine is designed so that:

one person can maintain it

features justify themselves via games

complexity is paid only once

If a system requires a team → redesign it.

2️⃣4️⃣ Final Principle (Non-Negotiable)

SecretEngine exists to ship games and earn money.
Not to win arguments.
Not to chase trends.
Not to impress engineers.

Status

✅ Frozen
Any change requires revisiting scope.md first.