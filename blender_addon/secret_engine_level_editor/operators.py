import bpy
import json
import math
from bpy.props import StringProperty
from bpy_extras.io_utils import ExportHelper, ImportHelper

class SECRETENGINE_OT_ExportChunk(bpy.types.Operator, ExportHelper):
    bl_idname = "secret_engine.export_chunk"
    bl_label = "Export Chunk"
    bl_description = "Export level chunk to JSON"
    
    filename_ext = ".json"
    filter_glob: StringProperty(default="*.json", options={'HIDDEN'})
    
    def execute(self, context):
        chunk_data = self.build_chunk_data(context)
        
        with open(self.filepath, 'w') as f:
            json.dump(chunk_data, f, indent=2)
        
        self.report({'INFO'}, f"Exported chunk to {self.filepath}")
        return {'FINISHED'}
    
    def build_chunk_data(self, context):
        scene = context.scene
        chunk_props = scene.secret_engine
        
        # Group objects by mesh(es) and material
        mesh_groups = {}
        
        for obj in scene.objects:
            if obj.type != 'MESH':
                continue
            
            props = obj.secret_engine
            
            # Determine mesh name
            if chunk_props.auto_mesh_from_name:
                # Extract base name from object name (remove suffix like _1, _2, .001, etc.)
                base_name = self.extract_base_name(obj.name)
                mesh_name = base_name + chunk_props.mesh_extension
            else:
                # Use manually set mesh name
                if props.mesh_mode == 'ARRAY' and len(props.meshes) > 0:
                    mesh_list = [m.mesh_name for m in props.meshes]
                    if len(set(mesh_list)) > 1 or len(mesh_list) > 1:
                        mesh_key = tuple(mesh_list)
                    else:
                        mesh_key = (mesh_list[0] if mesh_list else props.mesh_name,)
                else:
                    mesh_key = (props.mesh_name,)
                mesh_name = mesh_key[0] if len(mesh_key) == 1 else mesh_key
            
            # For auto mode, always use single mesh
            if chunk_props.auto_mesh_from_name:
                mesh_key = (mesh_name,)
            else:
                mesh_key = (mesh_name,) if isinstance(mesh_name, str) else mesh_name
            
            key = (mesh_key, props.material_name)
            
            if key not in mesh_groups:
                mesh_groups[key] = []
            
            # Build instance data
            instance = self.build_instance_data(obj, props)
            mesh_groups[key].append(instance)
        
        # Build final structure
        chunk_data = {
            "chunk_id": chunk_props.chunk_id,
            "mesh_groups": []
        }
        
        for (mesh_key, material), instances in mesh_groups.items():
            # Build mesh group with proper order: mesh, material, instances
            mesh_group = {}
            
            # Add mesh first (as string or array)
            if len(mesh_key) == 1:
                mesh_group["mesh"] = mesh_key[0]
            else:
                mesh_group["mesh"] = list(mesh_key)
            
            # Then material
            mesh_group["material"] = material
            
            # Finally instances
            mesh_group["instances"] = instances
            
            chunk_data["mesh_groups"].append(mesh_group)
        
        return chunk_data
    
    def extract_base_name(self, obj_name):
        """Extract base name from object name, removing numeric suffixes"""
        import re
        
        # Remove Blender's default suffixes like .001, .002, etc.
        name = re.sub(r'\.\d+$', '', obj_name)
        
        # Remove underscore + number suffixes like _1, _2, _001, etc.
        name = re.sub(r'_\d+$', '', name)
        
        # Convert to lowercase for consistency
        name = name.lower()
        
        return name
    
    def build_instance_data(self, obj, props):
        # Get transform
        loc = obj.location
        rot = obj.rotation_euler
        scale = obj.scale
        
        # Convert rotation to degrees
        rot_deg = [math.degrees(r) for r in rot]
        
        instance = {
            "transform": {
                "position": [loc.x, loc.z, -loc.y],  # Blender to engine coords
                "rotation": [rot_deg[0], rot_deg[2], -rot_deg[1]],
                "scale": [scale.x, scale.z, scale.y]
            },
            "tags": {},
            "culling": {
                "radius": props.culling_radius
            }
        }
        
        # Add tags
        if props.tag_type:
            instance["tags"]["type"] = props.tag_type
        if props.tag_name:
            instance["tags"]["name"] = props.tag_name
        else:
            instance["tags"]["name"] = obj.name
        
        # Add custom tags
        for tag in props.tags:
            if tag.key:
                instance["tags"][tag.key] = tag.value
        
        # Add material params
        if props.use_custom_color:
            instance["material_params"] = {
                "color": list(props.material_color)
            }
        
        # Add LOD
        if props.use_lod and len(props.lod_levels) > 0:
            instance["lod"] = {
                "levels": [{"distance": lod.distance} for lod in props.lod_levels]
            }
        
        return instance

class SECRETENGINE_OT_ImportChunk(bpy.types.Operator, ImportHelper):
    bl_idname = "secret_engine.import_chunk"
    bl_label = "Import Chunk"
    bl_description = "Import level chunk from JSON"
    
    filename_ext = ".json"
    filter_glob: StringProperty(default="*.json", options={'HIDDEN'})
    
    def execute(self, context):
        with open(self.filepath, 'r') as f:
            chunk_data = json.load(f)
        
        self.import_chunk_data(context, chunk_data)
        
        self.report({'INFO'}, f"Imported chunk from {self.filepath}")
        return {'FINISHED'}
    
    def import_chunk_data(self, context, chunk_data):
        scene = context.scene
        scene.secret_engine.chunk_id = chunk_data.get("chunk_id", "chunk_0")
        
        for mesh_group in chunk_data.get("mesh_groups", []):
            mesh_data = mesh_group.get("mesh")
            material_name = mesh_group.get("material", "default.mat")
            
            # Handle both string and array mesh formats
            if isinstance(mesh_data, list):
                mesh_names = mesh_data
            else:
                mesh_names = [mesh_data] if mesh_data else ["cube.meshbin"]
            
            for instance in mesh_group.get("instances", []):
                obj = self.create_object_from_instance(context, instance, mesh_names, material_name)
        
        # Deselect all
        bpy.ops.object.select_all(action='DESELECT')
    
    def create_object_from_instance(self, context, instance, mesh_names, material_name):
        # Create a cube as placeholder
        bpy.ops.mesh.primitive_cube_add()
        obj = context.active_object
        
        # Set transform (engine to Blender coords)
        transform = instance.get("transform", {})
        pos = transform.get("position", [0, 0, 0])
        rot = transform.get("rotation", [0, 0, 0])
        scale = transform.get("scale", [1, 1, 1])
        
        obj.location = (pos[0], -pos[2], pos[1])
        obj.rotation_euler = (math.radians(rot[0]), math.radians(-rot[2]), math.radians(rot[1]))
        obj.scale = (scale[0], scale[2], scale[1])
        
        # Set properties
        props = obj.secret_engine
        
        # Set mesh mode and meshes
        if len(mesh_names) == 1:
            props.mesh_mode = 'SINGLE'
            props.mesh_name = mesh_names[0]
        else:
            props.mesh_mode = 'ARRAY'
            props.meshes.clear()
            for mesh_name in mesh_names:
                mesh_item = props.meshes.add()
                mesh_item.mesh_name = mesh_name
        
        props.material_name = material_name
        
        # Set tags
        tags = instance.get("tags", {})
        props.tag_type = tags.get("type", "mesh")
        props.tag_name = tags.get("name", obj.name)
        
        # Set name
        obj.name = props.tag_name
        
        # Import custom tags
        for key, value in tags.items():
            if key not in ["type", "name"]:
                tag = props.tags.add()
                tag.key = key
                tag.value = str(value)
        
        # Set material params
        material_params = instance.get("material_params", {})
        if "color" in material_params:
            props.use_custom_color = True
            props.material_color = material_params["color"]
        
        # Set culling
        culling = instance.get("culling", {})
        props.culling_radius = culling.get("radius", 1.0)
        
        # Set LOD
        lod = instance.get("lod", {})
        if "levels" in lod:
            props.use_lod = True
            for lod_level in lod["levels"]:
                lod_item = props.lod_levels.add()
                lod_item.distance = lod_level.get("distance", 25.0)
        
        return obj

class SECRETENGINE_OT_AddTag(bpy.types.Operator):
    bl_idname = "secret_engine.add_tag"
    bl_label = "Add Tag"
    bl_description = "Add a custom tag"
    
    def execute(self, context):
        obj = context.active_object
        if obj:
            obj.secret_engine.tags.add()
        return {'FINISHED'}

class SECRETENGINE_OT_RemoveTag(bpy.types.Operator):
    bl_idname = "secret_engine.remove_tag"
    bl_label = "Remove Tag"
    bl_description = "Remove selected tag"
    
    index: bpy.props.IntProperty()
    
    def execute(self, context):
        obj = context.active_object
        if obj:
            obj.secret_engine.tags.remove(self.index)
        return {'FINISHED'}

class SECRETENGINE_OT_AddLODLevel(bpy.types.Operator):
    bl_idname = "secret_engine.add_lod_level"
    bl_label = "Add LOD Level"
    bl_description = "Add a LOD level"
    
    def execute(self, context):
        obj = context.active_object
        if obj:
            obj.secret_engine.lod_levels.add()
        return {'FINISHED'}

class SECRETENGINE_OT_RemoveLODLevel(bpy.types.Operator):
    bl_idname = "secret_engine.remove_lod_level"
    bl_label = "Remove LOD Level"
    bl_description = "Remove selected LOD level"
    
    index: bpy.props.IntProperty()
    
    def execute(self, context):
        obj = context.active_object
        if obj:
            obj.secret_engine.lod_levels.remove(self.index)
        return {'FINISHED'}

class SECRETENGINE_OT_AddMesh(bpy.types.Operator):
    bl_idname = "secret_engine.add_mesh"
    bl_label = "Add Mesh"
    bl_description = "Add a mesh to the array"
    
    def execute(self, context):
        obj = context.active_object
        if obj:
            obj.secret_engine.meshes.add()
        return {'FINISHED'}

class SECRETENGINE_OT_RemoveMesh(bpy.types.Operator):
    bl_idname = "secret_engine.remove_mesh"
    bl_label = "Remove Mesh"
    bl_description = "Remove selected mesh"
    
    index: bpy.props.IntProperty()
    
    def execute(self, context):
        obj = context.active_object
        if obj:
            obj.secret_engine.meshes.remove(self.index)
        return {'FINISHED'}

class SECRETENGINE_OT_SelectMeshGroup(bpy.types.Operator):
    bl_idname = "secret_engine.select_mesh_group"
    bl_label = "Select Mesh Group"
    bl_description = "Select all objects in this mesh group"
    
    mesh_key: bpy.props.StringProperty()
    material: bpy.props.StringProperty()
    
    def execute(self, context):
        # Deselect all first
        bpy.ops.object.select_all(action='DESELECT')
        
        # Parse mesh key
        import ast
        try:
            mesh_tuple = ast.literal_eval(self.mesh_key)
        except:
            mesh_tuple = (self.mesh_key,)
        
        # Select objects matching this mesh group
        for obj in context.scene.objects:
            if obj.type != 'MESH':
                continue
            
            props = obj.secret_engine
            
            # Get object's mesh key
            if props.mesh_mode == 'ARRAY' and len(props.meshes) > 0:
                obj_mesh_key = tuple(m.mesh_name for m in props.meshes)
            else:
                obj_mesh_key = (props.mesh_name,)
            
            # Check if matches
            if obj_mesh_key == mesh_tuple and props.material_name == self.material:
                obj.select_set(True)
        
        self.report({'INFO'}, f"Selected {len(context.selected_objects)} objects")
        return {'FINISHED'}
