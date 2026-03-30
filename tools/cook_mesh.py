import sys
import json
import struct
import os

def cook(input_path, output_path):
    print(f"Cooking {input_path} to {output_path}...")
    with open(input_path, 'rb') as f:
        magic = f.read(4)
        if magic != b'glTF':
            print("Not a GLB file")
            return
        struct.unpack('<I', f.read(4)) # version
        struct.unpack('<I', f.read(4)) # length
        
        chunk0_len = struct.unpack('<I', f.read(4))[0]
        chunk0_type = struct.unpack('<I', f.read(4))[0]
        json_data = json.loads(f.read(chunk0_len).decode('utf-8'))
        
        chunk1_len = struct.unpack('<I', f.read(4))[0]
        chunk1_type = struct.unpack('<I', f.read(4))[0]
        bin_data = f.read(chunk1_len)
        
    def get_data(accessor_idx):
        accessor = json_data['accessors'][accessor_idx]
        if 'bufferView' not in accessor: return None
        bv = json_data['bufferViews'][accessor['bufferView']]
        offset = bv.get('byteOffset', 0) + accessor.get('byteOffset', 0)
        count = accessor['count']
        ctype = accessor['componentType']
        type_map = {5126: 'f', 5123: 'H', 5125: 'I', 5121: 'B'}
        item_size_map = {'SCALAR': 1, 'VEC2': 2, 'VEC3': 3, 'VEC4': 4}
        item_size = item_size_map[accessor['type']]
        fmt = '<' + type_map.get(ctype, 'f') * item_size
        stride = bv.get('byteStride', struct.calcsize(fmt))
        
        data = []
        for i in range(count):
            item = struct.unpack_from(fmt, bin_data, offset + i * stride)
            data.append(item)
        return data

    all_positions = []
    all_normals = []
    all_uvs = []
    all_indices = []

    for mesh in json_data.get('meshes', []):
        for primitive in mesh.get('primitives', []):
            base_idx = len(all_positions)
            
            attrs = primitive['attributes']
            pos = get_data(attrs['POSITION'])
            if not pos: continue
            
            norm = get_data(attrs.get('NORMAL', -1))
            if norm is None or len(norm) == 0: norm = [(0,1,0)] * len(pos)
            
            uv = get_data(attrs.get('TEXCOORD_0', -1))
            if uv is None or len(uv) == 0: uv = [(0,0)] * len(pos)
            
            all_positions.extend(pos)
            all_normals.extend(norm)
            all_uvs.extend(uv)
            
            indices_idx = primitive.get('indices')
            if indices_idx is not None:
                idx_data = get_data(indices_idx)
                for i in idx_data:
                    # idx_data might be list of scalars or tuples
                    val = i[0] if isinstance(i, tuple) else i
                    all_indices.append(val + base_idx)
            else:
                for i in range(len(pos)):
                    all_indices.append(i + base_idx)

    if not all_positions:
        print("No vertex data found!")
        return

    with open(output_path, 'wb') as f:
        f.write(b'MESH')
        f.write(struct.pack('<I', 1)) 
        f.write(struct.pack('<I', len(all_positions)))
        f.write(struct.pack('<I', len(all_indices)))
        
        min_p = [min(p[i] for p in all_positions) for i in range(3)]
        max_p = [max(p[i] for p in all_positions) for i in range(3)]
        f.write(struct.pack('<ffffff', *min_p, *max_p))
        
        for i in range(len(all_positions)):
            p = all_positions[i]
            n = all_normals[i]
            u = all_uvs[i]
            # Handle possible VEC4 normals or UVs
            f.write(struct.pack('<ffffffff', p[0], p[1], p[2], n[0], n[1], n[2], u[0], u[1]))
            
        for idx in all_indices:
            f.write(struct.pack('<I', int(idx)))

    print(f"Done! Cooked {len(all_positions)} vertices and {len(all_indices)} indices (Merged all primitives)")

if __name__ == "__main__":
    if len(sys.argv) < 3:
        print("Usage: python cook_mesh.py <input.glb> <output.meshbin>")
    else:
        cook(sys.argv[1], sys.argv[2])
