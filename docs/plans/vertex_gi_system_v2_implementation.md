# Vertex GI System v2.0 - SECRET_ENGINE Implementation

**Engine:** SECRET_ENGINE Custom Vulkan  
**Philosophy:** Every byte counts. Every cycle matters. Waste nothing.  
**Target:** 240 FPS · 10M triangles · 2000 instances · ≤20 draw calls · Zero heap allocation at runtime

---

## Table of Contents

1. [Architecture Overview](#1-architecture-overview)
2. [Data Layout & Bit-Packing](#2-data-layout--bit-packing)
3. [Phase 1 — GPU-Driven Vertex Lighting](#3-phase-1--gpu-driven-vertex-lighting)
4. [Phase 2 — Light Vertex Cache (Simplified VCM)](#4-phase-2--light-vertex-cache-simplified-vcm)
5. [Phase 3 — Temporal Accumulation GI (TAGI)](#5-phase-3--temporal-accumulation-gi-tagi)
6. [Phase 4 — Probe Grid GI](#6-phase-4--probe-grid-gi)
7. [Vulkan Pipeline Integration](#7-vulkan-pipeline-integration)
8. [ADPF-Aware GI Scheduling](#8-adpf-aware-gi-scheduling)
9. [Performance Budget](#9-performance-budget)
10. [Implementation Prompts](#10-ai-prompts-for-implementation)
11. [Migration Checklist](#11-migration-checklist)

---

## 1. Architecture Overview

### The Problem with Lightmaps (Why We're Replacing Them)

```
❌ Traditional Lightmap Pipeline
   Bake → UV2 unwrap → 512×512 texture per chunk → GPU memory → Static only
   Cost: 4MB–16MB VRAM per scene, full rebake on any light change

✅ Our Vertex GI Pipeline
   Light trace → LightVertex cache → Spatial merge → vertex.color (4 bytes/vertex)
   Cost: 4 bytes per vertex, fully dynamic, LOD-aware, zero rebake
```

### System Diagram

```
Frame N:
┌─────────────────────────────────────────────────────────────────┐
│  COMPUTE PASS 0 (≤1 dispatch)                                   │
│  LightPathTracer.comp → GPU LightVertex Ring Buffer             │
│                                                                  │
│  COMPUTE PASS 1 (≤1 dispatch)                                   │
│  VertexLighting.comp  → Mesh VertexColor Buffer (R11G11B10F)    │
│                                                                  │
│  GRAPHICS PASS (≤18 draw calls)                                 │
│  mega_geometry.vert   → reads vertex color from buffer          │
│  mega_geometry.frag   → albedo * vertexColor * normalDetail     │
└─────────────────────────────────────────────────────────────────┘

Total new draw calls added: 0 (compute only)
Total VRAM added: vertex_count × 4 bytes
```

### Design Constraints Mapped to Decisions

| Constraint | Decision |
|------------|----------|
| Zero heap at runtime | Pre-allocated LightVertex ring buffer via VMA |
| ≤20 draw calls | GI is compute-only, zero raster passes |
| 240 FPS target | GI runs async, temporal blend hides latency |
| Android + ADPF | GI update rate throttled by thermal budget |
| VK_EXT_mesh_shader | Amplification stage pre-clips dark vertices |

---

## 2. Data Layout & Bit-Packing

### 2.1 LightVertex — 48 bytes, cache-line friendly

