SecretEngine – Core Interfaces Specification
Purpose

This document defines the canonical interface set of SecretEngine.

These names and responsibilities are authoritative.
LLMs and humans must not invent alternatives.

1. Interface Design Rules

Interfaces are stable

Interfaces are minimal

Interfaces expose behavior, not implementation

Interfaces never assume platform or backend

2. Mandatory Core Interfaces (Conceptual)
IPlugin

Represents any loadable engine extension.

Responsibilities:

lifecycle hooks

capability registration

activation / deactivation

IPluginManager

Manages plugin discovery and lifecycle.

Responsibilities:

loading plugins

validating compatibility

activating plugins per config

IRenderer

Abstract rendering service.

Responsibilities:

consume renderable data

submit GPU work

present frame
- **FDA Support:** Expose `GetCommandStream()` for 8-byte packet injection.
- **Performance Monitoring:** Expose `GetStats()` for rendering metrics
- **Memory Tracking:** Expose `GetVRAMUsage()` for VRAM monitoring

Must NOT:

know gameplay

own assets

manage input

IInputSystem

Unified input provider.

Responsibilities:

sample raw input

map to actions

expose frame-stable input state
- **FDA Support:** Expose `GetFastStream()` for 8-byte packet stream.

IPhysicsSystem

Simulation service.

Responsibilities:

collision detection

rigid body updates

spatial queries

INavigationSystem

Navigation & pathfinding.

Responsibilities:

navmesh queries

path generation

IAssetProvider

Runtime asset access.

Responsibilities:

load cooked assets

manage lifetimes

expose handles

ISceneLoader

Scene instantiation service.

Responsibilities:

load cooked scenes

create entities

assign components

INetworkBackend

Multiplayer transport interface.

Responsibilities:

message send/receive

session management

latency reporting

ILogger

Central logging interface.

Responsibilities:

structured logging

category filtering

3. Interface Stability Policy

Interfaces change rarely

Additive changes only

Breaking changes require major version bump

Deprecated interfaces remain supported until removed explicitly

Status

Frozen initial set