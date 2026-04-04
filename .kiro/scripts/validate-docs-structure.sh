#!/bin/bash

# Validate docs structure - only README.md should be in root docs/

echo "Validating docs/ structure..."

# Find all files in docs root (not in subdirectories)
files=$(find docs -maxdepth 1 -type f -name "*.md" -o -name "*.html")

invalid_files=()

for file in $files; do
    basename=$(basename "$file")
    if [ "$basename" != "README.md" ]; then
        invalid_files+=("$file")
    fi
done

if [ ${#invalid_files[@]} -eq 0 ]; then
    echo "✓ Docs structure is valid - only README.md in root"
    exit 0
else
    echo "✗ Invalid docs structure detected!"
    echo ""
    echo "The following files should be moved to subdirectories:"
    for file in "${invalid_files[@]}"; do
        echo "  - $file"
    done
    echo ""
    echo "Available subdirectories:"
    echo "  - docs/architecture/    (Engine architecture and design)"
    echo "  - docs/features/        (Feature documentation)"
    echo "  - docs/guides/          (How-to guides)"
    echo "  - docs/reference/       (API references)"
    echo "  - docs/implementation/  (Implementation details)"
    echo "  - docs/fixes/           (Bug fixes)"
    echo "  - docs/research/        (Research notes)"
    echo "  - docs/status/          (Status reports)"
    echo "  - docs/planning/        (Planning docs)"
    echo "  - docs/foundation/      (Design principles)"
    exit 1
fi
