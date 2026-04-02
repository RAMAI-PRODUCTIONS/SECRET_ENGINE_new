# Secret Engine Level Editor - Documentation Index

Welcome to the Secret Engine Level Editor for Blender! This index will help you find the documentation you need.

## Quick Links

- **New Users**: Start with [QUICKSTART.md](QUICKSTART.md)
- **Installation**: See [README.md](README.md#installation)
- **Having Issues?**: Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
- **Advanced Users**: See [ADVANCED.md](ADVANCED.md)

## Documentation Files

### Getting Started

#### [QUICKSTART.md](QUICKSTART.md)
**For**: Complete beginners  
**Time**: 5-10 minutes  
**Content**:
- Installation instructions
- Your first level chunk in 5 minutes
- Common tasks
- Keyboard shortcuts
- Next steps

#### [README.md](README.md)
**For**: All users  
**Time**: 15-20 minutes  
**Content**:
- Complete feature overview
- Detailed installation instructions
- Usage guide
- Object properties reference
- Coordinate system explanation
- Tips and best practices
- File format specification
- Version history

### Workflows

#### [WORKFLOW.md](WORKFLOW.md)
**For**: Intermediate users  
**Time**: 30-45 minutes  
**Content**:
- 7 detailed workflow guides
- Step-by-step instructions
- Time estimates
- Common patterns
- Workflow checklists
- Efficiency tips

**Workflows Covered**:
1. Creating a new level from scratch
2. Editing an existing level
3. Creating modular level pieces
4. Procedural level generation
5. Multi-chunk level design
6. Batch property updates
7. Level optimization

### Automation

#### [example_batch_operations.py](example_batch_operations.py)
**For**: Users who want to automate tasks  
**Time**: 5 minutes per example  
**Content**:
- 10 ready-to-use Python scripts
- Batch editing operations
- Procedural generation examples
- Validation tools
- Property management

**Scripts Included**:
1. Set mesh/material for selected objects
2. Auto-name objects
3. Generate cube grids
4. Add LOD to selected objects
5. Auto-calculate culling radius
6. Create circular patterns
7. Add tags to object types
8. Export collections as chunks
9. Validate object properties
10. Copy properties between objects

### Problem Solving

#### [TROUBLESHOOTING.md](TROUBLESHOOTING.md)
**For**: Users experiencing issues  
**Time**: 5-10 minutes to find solution  
**Content**:
- Installation issues
- Export/import problems
- UI issues
- Property issues
- Performance problems
- Workflow issues
- Engine integration
- Common error messages
- How to report bugs

**Categories**:
- Installation Issues
- Export Issues
- Import Issues
- UI Issues
- Property Issues
- Performance Issues
- Workflow Issues
- Engine Integration Issues

### Advanced Topics

#### [ADVANCED.md](ADVANCED.md)
**For**: Advanced users and developers  
**Time**: 1-2 hours  
**Content**:
- Python scripting guide
- Custom property types
- Custom operators
- Advanced export/import
- UI customization
- Engine integration
- Performance optimization
- 10 extension ideas with code

**Topics Covered**:
- Advanced Python scripting
- Custom property types
- Property validation
- Conditional export
- Multi-format export
- Import with mesh loading
- Custom panels
- Dynamic UI
- Live preview
- Asset browser integration
- Performance optimization

**Extension Ideas**:
1. Collision shape editor
2. Light system
3. Particle system
4. Audio sources
5. Navigation mesh
6. Prefab system
7. Terrain editor
8. Material preview
9. Validation tools
10. Version control integration

### Reference

#### [CHANGELOG.md](CHANGELOG.md)
**For**: Users tracking versions  
**Content**:
- Version history
- Feature additions
- Bug fixes
- Known limitations
- Future considerations

## Learning Paths

### Path 1: Beginner to Productive User
**Time**: ~1 hour

1. Read [QUICKSTART.md](QUICKSTART.md) (10 min)
2. Follow "First Level Chunk in 5 Minutes" (5 min)
3. Read [README.md](README.md) sections:
   - Installation (5 min)
   - Usage (10 min)
   - Object Properties (10 min)
   - Tips (5 min)
4. Try [WORKFLOW.md](WORKFLOW.md) - Workflow 1 (30 min)

**Result**: Can create and export basic level chunks

### Path 2: Efficient Level Designer
**Time**: ~2 hours

1. Complete Path 1
2. Read [WORKFLOW.md](WORKFLOW.md) completely (45 min)
3. Try 3-4 example scripts from [example_batch_operations.py](example_batch_operations.py) (30 min)
4. Practice with your own level (varies)

**Result**: Can efficiently create complex levels with automation

### Path 3: Advanced User/Developer
**Time**: ~4 hours

1. Complete Path 2
2. Read [ADVANCED.md](ADVANCED.md) completely (2 hours)
3. Implement one custom feature (varies)
4. Study the addon source code (1 hour)

**Result**: Can extend and customize the addon

## Common Tasks Quick Reference

### Installation
→ [README.md#Installation](README.md#installation) or [QUICKSTART.md](QUICKSTART.md)

### Create First Level
→ [QUICKSTART.md#First Level Chunk in 5 Minutes](QUICKSTART.md)

### Import Existing Level
→ [README.md#Importing an Existing Chunk](README.md#importing-an-existing-chunk)

### Add Custom Tags
→ [README.md#Tags](README.md#tags)

### Set Up LOD
→ [README.md#Level of Detail (LOD)](README.md#level-of-detail-lod)

### Batch Edit Objects
→ [example_batch_operations.py](example_batch_operations.py)

### Optimize Performance
→ [WORKFLOW.md#Workflow 7: Level Optimization](WORKFLOW.md#workflow-7-level-optimization)

### Fix Export Issues
→ [TROUBLESHOOTING.md#Export Issues](TROUBLESHOOTING.md#export-issues)

### Create Modular Levels
→ [WORKFLOW.md#Workflow 3: Creating Modular Level Pieces](WORKFLOW.md#workflow-3-creating-modular-level-pieces)

### Procedural Generation
→ [WORKFLOW.md#Workflow 4: Procedural Level Generation](WORKFLOW.md#workflow-4-procedural-level-generation)

### Multi-Chunk Levels
→ [WORKFLOW.md#Workflow 5: Multi-Chunk Level Design](WORKFLOW.md#workflow-5-multi-chunk-level-design)

### Extend the Addon
→ [ADVANCED.md](ADVANCED.md)

## File Structure

```
blender_addon/
├── secret_engine_level_editor/    # Addon source code
│   ├── __init__.py                # Main addon file
│   ├── properties.py              # Property definitions
│   ├── operators.py               # Export/import operators
│   └── panels.py                  # UI panels
│
├── README.md                      # Main documentation
├── QUICKSTART.md                  # Quick start guide
├── WORKFLOW.md                    # Workflow guides
├── TROUBLESHOOTING.md             # Problem solving
├── ADVANCED.md                    # Advanced topics
├── CHANGELOG.md                   # Version history
├── INDEX.md                       # This file
│
├── example_batch_operations.py   # Example scripts
│
├── install.sh                     # Linux/macOS installer
└── install.bat                    # Windows installer
```

## Support

### Before Asking for Help

1. Check [TROUBLESHOOTING.md](TROUBLESHOOTING.md) for your issue
2. Verify you followed the installation steps correctly
3. Check the Blender console for error messages
4. Try with a fresh Blender scene
5. Verify your Blender version is 3.0+

### When Reporting Issues

Include:
- Blender version
- Operating system
- Addon version
- Steps to reproduce
- Error messages
- Example files if possible

### Resources

- Blender Documentation: https://docs.blender.org/
- Blender Python API: https://docs.blender.org/api/current/
- Secret Engine Documentation: (link to your docs)

## Contributing

Contributions are welcome! If you:
- Fix a bug
- Add a feature
- Improve documentation
- Create useful scripts

Please share with the community!

## Version Information

**Current Version**: 1.0.0  
**Release Date**: 2026-04-02  
**Blender Compatibility**: 3.0+  
**Engine Compatibility**: Secret Engine 7.3+

See [CHANGELOG.md](CHANGELOG.md) for detailed version history.

## License

This addon is part of the Secret Engine project.

---

**Happy Level Designing!** 🎮

For questions or feedback, refer to the Secret Engine project documentation.
