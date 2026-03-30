import json, struct
with open('Assets/meshes/Character-Animated.glb', 'rb') as f:
    f.read(12)
    jlen = struct.unpack('<I', f.read(4))[0]
    f.read(4)
    jdata = json.loads(f.read(jlen).decode('utf-8'))
    print(json.dumps(jdata, indent=2))
