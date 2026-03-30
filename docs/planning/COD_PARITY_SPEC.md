# Technical Architecture Specification: Call of Duty Parity (CODM)

## 1. Executive Summary
This document defines the requisite technologies and subsystems necessary for SecretEngine to achieve parity with titles like *Call of Duty: Mobile*. The architecture prioritizes **Data-Oriented Design (DOD)**, **Zero-Copy Memory**, and **Tile-Based Rendering** to scale across mobile hardware while maintaining 60/120 FPS.

## 2. Hardware & Performance Budget
| Constraint | Target | SecretEngine Implementation |
| :--- | :--- | :--- |
| **Frame Rate** | 60/120 FPS | **89 FPS** (Current @ 6M Tris - Milestone �️) |
| **Draw Calls** | < 2,500 | **1-2** (GPU-Driven Indirect) |
| **Triangles** | 1M Visible | **6,056,050 Triangles** (Virtual Culling) |
| **Bandwidth** | < 100 MB/s | NITRO 16-bit/8-bit + GPU Cull |
| **Thermal** | Adaptive | ADPF Integration (Thermal Headroom) |

## 3. Core Systems
### 3.1 Fiber-Based Job System
- Utilize user-mode threads (fibers) for task scheduling.
- Pin Render/Audio threads to "Big" cores; background streaming to "Little" cores.
- Frame-Graph dependency sorting.

### 3.2 Tiered Memory Allocators
- **Linear/Arena:** Per-frame temporary data. Reset every frame.
- **Pool:** Fixed-size objects (bullets, particles) to prevent fragmentation.
- **TLSF:** General purpose for variable-sized assets.

## 4. Rendering Pipeline
### 4.1 Tile-Based Deferred Rendering (TBDR)
- Use **Vulkan Subpasses** to keep G-Buffer data in local tile memory (SRAM).
- Avoid writing/reading high-bandwidth buffers to DRAM.

### 4.2 Asset Cooking (ASTC)
- Mandatory **ASTC** (Adaptive Scalable Texture Compression) for all textures.
- Virtual Texturing (Sparse Residency) for large Battle Royale maps.

### 4.3 Occlusion Strategies
- **Precomputed Visibility (PVS):** Offline cell-based visibility lookup.
- **Software Occlusion:** Rasterize low-res occluders on CPU via SIMD (NEON).

## 5. Competitive Networking
### 5.1 Protocol: Custom UDP
- Rust-based authoritative game server (Server-side).
- C++ Client plugin for communication (Client-side).
- Packet sequencing and variable reliability.
- Delta compression and bit-packed quantization.

### 5.2 Client-Side Prediction (CSP)
- Local simulation of inputs to hide latency.
- State reconciliation (replay) on server mismatch.

### 5.3 Server-Side Rewind (Lag Compensation)
- Server maintains 1s history of player hitboxes.
- Rewinds simulation time to validate individual client shots.

### 5.4 Local Latency: Fast Data Architecture (FDA)
- Use 8-byte packets and lock-free SPSC queues for sub-microsecond local communication.
- Guaranteed L1 cache hits for high-frequency input and render command updates.
- Enables the "instant" response feel critical for competitive shooter parity.

## 6. Gameplay Mechanics
### 6.1 Kinematic Character Controller (KCC)
- Code-driven physics (Slide vectors, mantling).
- Predictive collision resolution (no "sticking" to walls).

### 6.2 Gunsmith Logic (Modular ECS)
- Weapon entities built from composite socketed attachments.
- Procedural IK for hand placement across 50+ attachments.

## 7. Platform & Thermal Strategy
- **ADPF (Android Dynamic Performance Framework):** Monitor thermal headroom.
- **Dynamic Fidelity Scaling:** Reduce resolution/post-processing *before* OS throttling occurs.

---
**Status:** REFERENCE SPECIFICATION (Drives Implementation Task List)
**Last Updated:** 2026-02-06
