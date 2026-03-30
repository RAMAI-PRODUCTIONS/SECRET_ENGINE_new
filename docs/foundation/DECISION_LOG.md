SecretEngine – Architectural Decision Log
Purpose

This document records why decisions were made.

It prevents repeated debates and LLM “why not X?” suggestions.

Decisions
001 – Vulkan Only

Decision: Vulkan-only renderer
Reason: Mobile performance, explicit control
Date: Initial scope

002 – Plugin-Based Architecture

Decision: Everything except core is a plugin
Reason: Replaceability, long-term flexibility

003 – No Runtime JSON Parsing

Decision: JSON is authoring-only
Reason: Performance, reliability

004 – C++20/23 Core

Decision: No embedded scripting language
Reason: Performance, simplicity, control

005 – Rust for Multiplayer & Tooling

Decision: Rust limited to infrastructure
Reason: Safety, isolation, ABI stability

006 – Mobile-First Design

Decision: Mobile constraints drive design
Reason: Largest audience, hardest target

007 – Asset Cooker Mandatory

Decision: All assets must be cooked
Reason: Predictability, performance

008 – No Editor Dependency

Decision: Web-based authoring, no editor runtime
Reason: Solo-dev scalability

009 – Bootstrap Scene Loading from JSON

Decision: Load scene data from JSON during bootstrap/development phase
Reason: Rapid iteration, easy to modify without binary tools
Date: 2026-02-02

Status

Living document (append-only)