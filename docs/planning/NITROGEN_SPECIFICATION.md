# 🚀 NITROGEN RENDERING SPECIFICATION
**Version:** 1.0 (Nitrogen Balanced Fidelity)
**Capability:** 300M+ Triangles/Sec on Mobile

## 💎 Overview
The Nitrogen Rendering Pipeline is a specialized, high-performance rendering path designed for massive geometry throughput on mobile GPUs. It achieves theoretical peak performance by using **Lossy Fixed-Point Quantization** to minimize memory bandwidth—the #1 bottleneck on mobile.

## 🛠️ Technical Architecture

### 1. The 64-Byte Instance Layout
Nitrogen uses a "Balanced Layout" where every instance fits into a single 64-byte GPU cache line.
- **Row-Major 3x4 Matrix (48 bytes)**: Combined rotation/translation.
- **Packed 8-bit RGBA Color (4 bytes)**: Decoded via `unpackUnorm4x8`.
- **Hardware Alignment (12 bytes)**: Padding to ensure sequential GPU fetches never span cache lines.

### 2. Vertex Quantization (16 Bytes)
To maximize vertex cache density, Nitrogen discards 32-bit floats in favor of fixed-point SNORM data:
- **Position (int16_t[4])**: 16-bit precision. Decoded in shader using a `8.0` scale factor. range: [-8.0, 8.0] units.
- **Normal (int8_t[4])**: 8-bit precision. Decoded as signed normalized.
- **UV (uint16_t[2])**: 16-bit precision for high-res texture tiling.

### 3. Virtual Geometry: GPU Culling (Phase 1)
Nitrogen eliminates the CPU-bottleneck of visibility by performing **View Frustum Culling** on the GPU:
- **Compute Pass (`cull.comp`)**: Every frame, a 0.1ms compute shader checks 65k instances against the camera frustum.
- **Visible Buffer**: Only visible instances are copied to a high-speed `VisibleInstanceSSBO`.
- **Indirect Execution**: The CPU issues a single command, but the GPU fills the `instanceCount` based on how many objects it *actually* found visible.

### 4. GPU-CPU Sync (Quad-Buffering)
Eliminates "stall-on-map" by using double-buffering for all SSBOs and Indirect Buffers. The CPU prepares Frame N+1 while the GPU renders Frame N.

---

## 🎨 Artist Instructions: Creating "Nitrogen-Ready" Assets

To maintain consistency and avoid visual "boxes" or floating vertices, artists must follow these strict technical constraints:

### 📐 1. The Scaling Rule (Crucial)
Because Nitrogen uses 16-bit positions scaled by **8.0**, any vertex outside the **[-8, 8] meter range** will be CLAMPED.
- **Constraint**: All meshes must fit within a **16m x 16m x 16m** bounding box centered at (0,0,0).
- **If the mesh is larger**: It must be split into multiple smaller meshes, or the quantization scale in `mega_geometry.vert` must be increased (which reduces precision).

### 📍 2. Origin & Normalization
- **Pivot**: Always center the pivot at the bottom-center or true center of the object.
- **Normals**: Do not use "dirty" normals. Run a "Set to Face" and "Average Normals" pass in Maya/Blender. Nitrogen reconstructs normals using 8-bit data; high-quality source normals prevent "faceting" artifacts.

### 🗺️ 3. UV Management
- **Range**: Keep UVs within the **[0.0, 1.0]** range. Tiling is supported, but extreme tiling (e.g., UV=100.0) may lose precision due to the 16-bit quantization.

### 🛠️ 4. Exporting to `.meshbin`
Use the engine's current converter tools. Nitrogen assumes the following vertex layout for the source data before it quantizes them:
1. `float position[3]`
2. `float normal[3]`
3. `float uv[2]`

---

## 🏎️ How to reach 60 FPS (Ongoing)
1. **Never use 32-bit Float Attributes**: Stick to the Nitrogen Layout.
2. **Minimize Interpolators**: Pass as little data as possible from Vertex to Fragment stages.
3. **Use Indirect Draw**: Keep draw calls at 1-2 per frame for millions of triangles.
