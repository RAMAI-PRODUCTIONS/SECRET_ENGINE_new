# Contributing to Documentation

## Documentation Structure Rules

The `docs/` folder follows a strict organizational structure:

### Root Folder Rule
**Only `README.md` is allowed in the root `docs/` folder.** All other documentation must be placed in appropriate subdirectories.

### Directory Organization

| Directory | Purpose | Examples |
|-----------|---------|----------|
| `architecture/` | Engine architecture, design patterns, system overviews | RENDERING_ARCHITECTURE.md, CORE_INTERFACES.md |
| `features/` | Feature documentation and specifications | LEVEL_SYSTEM.md, GAMEPLAY_TAG_SYSTEM.md |
| `guides/` | How-to guides, tutorials, quickstarts | LEVEL_SYSTEM_GUIDE.md, VERTEX_GI_QUICKSTART.md |
| `reference/` | API references, technical specs, indexes | DATA_FLOW_JSON_TO_RENDERER.md, INDEX.md |
| `implementation/` | Implementation details and plans | LEVEL_SYSTEM_IMPLEMENTATION.md |
| `fixes/` | Bug fixes, debugging guides, troubleshooting | ANDROID_BLACK_SCREEN_DEBUG.md |
| `research/` | Research notes, analysis, comparisons | ARCHITECTURE_RESEARCH_SUMMARY.md |
| `status/` | Build status, progress reports, summaries | BUILD_STATUS.md, PERFORMANCE_STATS.md |
| `planning/` | Project planning, roadmaps, checklists | VERIFICATION_CHECKLIST.md |
| `foundation/` | Core principles, design decisions, terminology | DESIGN_PRINCIPLES.md, DECISION_LOG.md |
| `getting-started/` | Getting started guides for new developers | |

## Adding New Documentation

1. **Choose the right directory** based on the content type
2. **Use descriptive filenames** in UPPER_SNAKE_CASE.md
3. **Update the main README.md** if adding significant new content
4. **Never create files directly in `docs/` root** (except README.md)

## Automated Enforcement

The project includes automated hooks that will:
- Alert you when creating files in the wrong location
- Suggest the correct subdirectory for your content
- Validate the structure on file edits

## Validation

Run the validation script to check structure compliance:

```bash
# Linux/Mac
bash .kiro/scripts/validate-docs-structure.sh

# Windows
.kiro\scripts\validate-docs-structure.bat
```

## Questions?

If you're unsure where to place a document, consider:
- Is it explaining how something works? → `architecture/`
- Is it teaching how to do something? → `guides/`
- Is it describing a feature? → `features/`
- Is it a reference or spec? → `reference/`
- Is it tracking progress? → `status/`
