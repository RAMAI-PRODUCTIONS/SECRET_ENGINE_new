SecretEngine – Terminology Reference
Purpose

This document defines exact meanings of words used in SecretEngine.

These meanings are binding.

Core Terms

Engine Core
The immutable layer defining interfaces, lifecycle, and data flow.

Plugin
A loadable module that implements one engine capability.

System
A logical service provided by a plugin (renderer, input, physics).

Entity
A runtime object identified by an ID.

Component
Data attached to an entity.

Scene
A collection of entities and components loaded together.

World
The active simulation context containing one or more scenes.

Asset Terms

Authoring Asset
Human-editable source asset (JSON, GLTF, JPG).

Cooked Asset
Binary, optimized asset used at runtime.

Runtime Asset
Loaded cooked asset in memory.

Rendering Terms

Renderable
An entity eligible for rendering.

Instance
A per-object draw representation sharing mesh/material.

LOD
Level of detail, mesh or texture variant.

Input Terms

Raw Input
Unprocessed device input.

Action
Mapped gameplay intent.

Networking Terms

Client
Game instance.

Server
Authoritative simulation instance.

Deterministic
Produces identical results given same input.

Status

Frozen vocabulary