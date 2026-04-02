# Changelog

All notable changes to the Secret Engine Level Editor addon will be documented in this file.

## [1.0.0] - 2026-04-02

### Added
- Initial release of Secret Engine Level Editor for Blender
- Export level chunks to JSON format compatible with Secret Engine
- Import existing level chunks for editing
- Object property management:
  - Mesh and material assignment
  - Tag system (type, name, custom key-value pairs)
  - Material parameter overrides (color)
  - Frustum culling radius configuration
  - Level of Detail (LOD) system with multiple distance levels
- Automatic coordinate system conversion (Blender ↔ Engine)
- Automatic grouping of instances by mesh and material
- UI panels in Blender sidebar:
  - Main panel with export/import buttons
  - Chunk settings panel
  - Object properties panel with all configuration options
- Installation scripts for Windows, macOS, and Linux
- Comprehensive documentation:
  - README with full feature documentation
  - Quick Start guide for beginners
  - Example batch operations Python scripts
- Batch operation examples:
  - Set mesh/material for multiple objects
  - Auto-naming based on type
  - Generate cube grids with random colors
  - Add LOD to selected objects
  - Auto-calculate culling radius
  - Create circular patterns
  - Add tags to object types
  - Export collections as separate chunks
  - Validate object properties
  - Copy properties between objects

### Technical Details
- Blender 3.0+ compatibility
- Python-based addon architecture
- Modular code structure (properties, operators, panels)
- Proper coordinate space conversion (Y-up to Z-up)
- Rotation conversion (radians to degrees)

### Known Limitations
- Uses placeholder cubes on import (actual mesh geometry not loaded)
- No visual preview of engine materials in Blender
- Custom tags must be added one at a time through UI

### Future Considerations
- Mesh preview integration
- Material preview system
- Batch tag editing
- Chunk validation tools
- Multi-chunk level editing
- Streaming zone visualization
- Collision shape editing
- Light and entity placement
