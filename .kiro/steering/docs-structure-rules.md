---
inclusion: fileMatch
fileMatchPattern: 'docs/**/*.md'
---

# Documentation Structure Rules

## Root Docs Folder Rule

**CRITICAL**: The `docs/` root folder should ONLY contain `README.md`. All other documentation files must be organized into subdirectories.

## Directory Structure

```
docs/
├── README.md                    # ONLY file allowed in root
├── architecture/                # Engine architecture and design
├── features/                    # Feature documentation
├── guides/                      # How-to guides and tutorials
├── reference/                   # API references and specs
├── implementation/              # Implementation details
├── fixes/                       # Bug fixes and troubleshooting
├── research/                    # Research and analysis
├── status/                      # Build status and progress
├── planning/                    # Project planning
├── foundation/                  # Core principles
└── getting-started/             # Getting started guides
```

## File Placement Guidelines

- **Architecture docs** → `docs/architecture/`
- **Feature documentation** → `docs/features/`
- **How-to guides** → `docs/guides/`
- **API references** → `docs/reference/`
- **Implementation plans** → `docs/implementation/`
- **Bug fixes** → `docs/fixes/`
- **Research notes** → `docs/research/`
- **Status reports** → `docs/status/`
- **Planning docs** → `docs/planning/`
- **Design principles** → `docs/foundation/`

## When Creating New Documentation

1. Determine the appropriate category
2. Place the file in the corresponding subdirectory
3. Update the main `docs/README.md` if adding a major new document
4. Never create markdown files directly in `docs/` root

## Enforcement

Automated hooks will:
- Alert when files are created in `docs/` root (except README.md)
- Warn when editing non-README files in `docs/` root
- Suggest appropriate subdirectories for misplaced files
