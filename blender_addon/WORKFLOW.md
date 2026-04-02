# Workflow Guide

This guide describes recommended workflows for different level design scenarios.

## Basic Workflow

```
1. Design in Blender
   ↓
2. Set Properties
   ↓
3. Export Chunk
   ↓
4. Test in Engine
   ↓
5. Iterate
```

## Detailed Workflows

### Workflow 1: Creating a New Level from Scratch

**Goal**: Create a complete level chunk with ground, props, and triggers.

**Steps**:

1. **Setup** (5 min)
   - Open Blender
   - Delete default objects (camera, light, cube)
   - Save as `my_level.blend`

2. **Create Ground** (5 min)
   - Add Cube (`Shift+A` > Mesh > Cube)
   - Scale to ground size (`S` > `100`)
   - Flatten (`S` > `Z` > `0.01`)
   - Set properties:
     - Mesh: `cube.meshbin`
     - Material: `ground.mat`
     - Type: `ground`
     - Name: `ground_plane`
     - Culling: `100`

3. **Add Props** (15 min)
   - Add objects for level geometry
   - Position with `G` (move), `R` (rotate), `S` (scale)
   - Set properties for each:
     - Mesh: appropriate mesh file
     - Material: appropriate material
     - Type: `mesh` or specific type
     - Name: descriptive name
     - Color: if using custom colors
     - Culling: based on size
     - LOD: if needed

4. **Add Triggers** (5 min)
   - Add cubes at trigger locations
   - Set properties:
     - Mesh: `cube.meshbin`
     - Material: `trigger.mat`
     - Type: `trigger`
     - Name: descriptive trigger name
     - Custom tags: trigger-specific data

5. **Export** (2 min)
   - Set Chunk ID in Chunk Settings
   - Click Export Chunk
   - Save to engine assets folder

6. **Test** (varies)
   - Run engine
   - Check positions, rotations, scales
   - Verify triggers work
   - Check culling and LOD

7. **Iterate** (repeat as needed)
   - Make changes in Blender
   - Re-export
   - Test again

**Total Time**: ~30 minutes for first iteration

---

### Workflow 2: Editing an Existing Level

**Goal**: Modify an existing level chunk.

**Steps**:

1. **Import** (2 min)
   - Open Blender
   - Click Import Chunk
   - Select existing chunk JSON
   - Wait for import to complete

2. **Review** (5 min)
   - Check all objects imported correctly
   - Verify properties are set
   - Use outliner to see object hierarchy

3. **Make Changes** (varies)
   - Move, rotate, scale objects
   - Add new objects
   - Delete unwanted objects
   - Modify properties
   - Change colors, tags, etc.

4. **Validate** (2 min)
   - Run validation script (see example_batch_operations.py)
   - Check for missing properties
   - Verify all objects have names

5. **Export** (2 min)
   - Export to same or new file
   - Overwrite or create new version

6. **Test** (varies)
   - Test in engine
   - Compare with previous version

**Total Time**: ~15 minutes + editing time

---

### Workflow 3: Creating Modular Level Pieces

**Goal**: Create reusable level chunks that can be combined.

**Steps**:

1. **Plan Modules** (10 min)
   - Decide on module types (corridors, rooms, etc.)
   - Define standard sizes
   - Plan connection points

2. **Create Template** (5 min)
   - Create one module completely
   - Set all properties
   - Test export/import

3. **Create Variations** (varies)
   - Duplicate template
   - Modify geometry
   - Keep same size/connection points
   - Export each as separate chunk

4. **Organize** (5 min)
   - Use Blender collections for each module type
   - Name consistently
   - Document module specifications

5. **Test Combinations** (varies)
   - Import multiple modules in engine
   - Verify they connect properly
   - Check for gaps or overlaps

**Total Time**: ~30 minutes + variation creation time

---

### Workflow 4: Procedural Level Generation

**Goal**: Use Python scripts to generate level layouts.

**Steps**:

1. **Write Generator Script** (varies)
   - Use example_batch_operations.py as reference
   - Define generation rules
   - Add randomization if desired

2. **Run Script** (1 min)
   - Open Blender scripting workspace
   - Load or paste script
   - Run script

3. **Review Generated Content** (5 min)
   - Check object placement
   - Verify properties are set correctly
   - Make manual adjustments if needed

4. **Export** (2 min)
   - Export generated chunk
   - Test in engine

5. **Iterate on Script** (varies)
   - Adjust generation parameters
   - Fix issues
   - Re-run and test

**Example Scripts**:
- Grid of objects: `create_cube_grid()`
- Circular patterns: `create_circle_pattern()`
- Random placement with rules

**Total Time**: Varies greatly based on complexity

---

### Workflow 5: Multi-Chunk Level Design

**Goal**: Create a large level split into multiple chunks.

**Steps**:

1. **Plan Chunk Layout** (15 min)
   - Decide on chunk size (e.g., 200x200 units)
   - Plan how many chunks needed
   - Define chunk boundaries
   - Plan streaming strategy

2. **Setup Collections** (5 min)
   - Create one collection per chunk
   - Name: `chunk_0_0`, `chunk_0_1`, etc.
   - Use grid naming for easy reference

3. **Design Level** (varies)
   - Create all geometry
   - Assign objects to appropriate collections
   - Keep objects within chunk boundaries
   - Consider chunk borders (avoid splitting objects)

4. **Export All Chunks** (5 min)
   - Use export_collections_as_chunks() script
   - Or manually export each collection
   - Name files consistently

5. **Create Level Definition** (10 min)
   - Create or update level JSON
   - Reference all chunk files
   - Set streaming parameters
   - Define load/unload radii

6. **Test Streaming** (varies)
   - Test in engine
   - Verify chunks load/unload correctly
   - Check for seams between chunks
   - Optimize streaming parameters

**Total Time**: ~45 minutes + design time

---

### Workflow 6: Batch Property Updates

**Goal**: Update properties for many objects at once.

**Steps**:

1. **Identify Objects** (5 min)
   - Determine which objects need updates
   - Group by common property changes
   - Use outliner to select

2. **Choose Method**:

   **Method A: Manual Selection**
   - Select all objects to update
   - Use copy_properties_to_selected() script
   - Or manually edit each

   **Method B: By Type**
   - Use add_tag_to_type() script
   - Or set_mesh_material_for_selected() script
   - Filters by object type

   **Method C: Custom Script**
   - Write custom Python script
   - Use example_batch_operations.py as template
   - Run on all or filtered objects

3. **Validate Changes** (2 min)
   - Run validate_objects() script
   - Check a few objects manually
   - Verify changes are correct

4. **Export and Test** (5 min)
   - Export chunk
   - Test in engine
   - Verify changes work as expected

**Total Time**: ~15 minutes

---

### Workflow 7: Level Optimization

**Goal**: Optimize level for performance.

**Steps**:

1. **Analyze Current State** (10 min)
   - Count total objects
   - Check culling radii
   - Review LOD settings
   - Identify performance issues in engine

2. **Optimize Culling** (10 min)
   - Run auto_culling_radius() script
   - Manually adjust for important objects
   - Reduce radius for small objects
   - Increase for large objects

3. **Add LOD** (15 min)
   - Identify objects that need LOD
   - Use add_lod_to_selected() script
   - Set appropriate distances
   - Test in engine

4. **Reduce Object Count** (varies)
   - Combine similar objects
   - Remove unnecessary detail
   - Use instancing where possible
   - Merge static geometry

5. **Test Performance** (varies)
   - Export and test in engine
   - Measure frame rate
   - Check draw calls
   - Profile rendering

6. **Iterate** (varies)
   - Adjust based on profiling
   - Balance quality vs performance
   - Re-test

**Total Time**: ~45 minutes + iteration time

---

## Tips for Efficient Workflows

### Organization
- Use Blender collections to organize objects
- Name objects descriptively
- Use consistent naming conventions
- Keep related objects together

### Iteration Speed
- Export frequently to test
- Use small test chunks first
- Keep Blender and engine open simultaneously
- Use hot-reload in engine if available

### Automation
- Learn basic Python scripting
- Create custom batch operation scripts
- Use Blender's built-in batch operations
- Automate repetitive tasks

### Quality Control
- Run validation scripts before export
- Check exported JSON manually occasionally
- Test in engine regularly
- Keep backups of working versions

### Collaboration
- Use version control for .blend files
- Document custom properties and tags
- Share batch operation scripts
- Maintain consistent standards

### Performance
- Set appropriate culling radii
- Use LOD for distant objects
- Split large levels into chunks
- Profile and optimize regularly

## Common Patterns

### Pattern: Template Objects
1. Create one object with all properties set
2. Duplicate for variations
3. Only change position/rotation/scale
4. Keeps properties consistent

### Pattern: Collection-Based Organization
1. Create collections for object types
2. Assign objects to collections
3. Export collections separately
4. Easier to manage large levels

### Pattern: Iterative Refinement
1. Start with basic layout
2. Export and test
3. Add detail incrementally
4. Test after each addition
5. Optimize at the end

### Pattern: Modular Design
1. Create reusable modules
2. Test modules individually
3. Combine in different ways
4. Reduces duplication of work

## Workflow Checklist

Before exporting, verify:
- [ ] All objects have mesh_name set
- [ ] All objects have material_name set
- [ ] All objects have type tag
- [ ] All objects have name tag
- [ ] Culling radii are appropriate
- [ ] LOD is set for distant objects
- [ ] Custom tags are correct
- [ ] Chunk ID is set
- [ ] Export path is correct

After exporting, verify:
- [ ] JSON file exists
- [ ] JSON is valid
- [ ] File size is reasonable
- [ ] All expected objects are present
- [ ] Properties are correct in JSON

In engine, verify:
- [ ] Level loads without errors
- [ ] Objects appear in correct positions
- [ ] Rotations are correct
- [ ] Scales are correct
- [ ] Colors are correct
- [ ] Triggers work
- [ ] Culling works
- [ ] LOD works
- [ ] Performance is acceptable
