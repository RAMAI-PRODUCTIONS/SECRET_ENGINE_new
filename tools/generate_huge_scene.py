import json
import random

def generate_scene():
    entities = []

    # 1. Player Start (Looking at the grid)
    # Positioned at (0, 5, -50), looking towards +Z (where the grid will be)
    # Rotation roughly 0 degrees Y if +Z is forward. 
    # If 180 is +Z, then we use 180.
    entities.append({
        "name": "PlayerStart",
        "transform": {
            "position": [0, 5, -50],
            "rotation": [0, 180, 0], 
            "scale": [1, 1, 1]
        },
        "isPlayerStart": True
    })

    # 2. 1000 Instances of Character
    # Arranged in a grid from X=-200 to X=200, Z=0 to Z=400
    rows = 25
    cols = 40 # 25 * 40 = 1000
    spacing_x = 15.0
    spacing_z = 15.0
    
    start_x = -(cols * spacing_x) / 2.0
    start_z = 0.0

    count = 0
    for r in range(rows):
        for c in range(cols):
            x = start_x + c * spacing_x
            z = start_z + r * spacing_z
            
            entities.append({
                "name": f"Char_{count}",
                "transform": {
                    "position": [x, 0, z],
                    "rotation": [0, 90, 0],
                    "scale": [15, 15, 15]
                },
                "mesh": {
                    "path": "meshes/Character.meshbin",
                    "color": [
                        random.random(), # R
                        random.random(), # G
                        random.random(), # B
                        1.0
                    ]
                },
                "isPlayer": False
            })
            count += 1

    scene = {"entities": entities}
    
    with open('Assets/scene.json', 'w') as f:
        json.dump(scene, f, indent=4)
    
    print(f"Generated scene.json with {len(entities)} entities ({count} characters).")

if __name__ == "__main__":
    generate_scene()
