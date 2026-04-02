import bpy
from bpy.props import (
    StringProperty,
    FloatProperty,
    FloatVectorProperty,
    BoolProperty,
    CollectionProperty,
    IntProperty,
    EnumProperty,
)

class SecretEngineTag(bpy.types.PropertyGroup):
    key: StringProperty(name="Key", default="")
    value: StringProperty(name="Value", default="")

class SecretEngineLODLevel(bpy.types.PropertyGroup):
    distance: FloatProperty(name="Distance", default=25.0, min=0.0)

class SecretEngineMeshItem(bpy.types.PropertyGroup):
    mesh_name: StringProperty(
        name="Mesh",
        description="Mesh file name (e.g., cube.meshbin)",
        default="cube.meshbin"
    )

class SecretEngineInstanceProperties(bpy.types.PropertyGroup):
    # Mesh mode: single or array
    mesh_mode: EnumProperty(
        name="Mesh Mode",
        description="Use single mesh or array of meshes",
        items=[
            ('SINGLE', 'Single', 'Use a single mesh'),
            ('ARRAY', 'Array', 'Use multiple meshes (for LOD or variations)'),
        ],
        default='SINGLE'
    )
    
    # Single mesh (legacy/simple mode)
    mesh_name: StringProperty(
        name="Mesh",
        description="Mesh file name (e.g., cube.meshbin)",
        default="cube.meshbin"
    )
    
    # Multiple meshes (array mode)
    meshes: CollectionProperty(type=SecretEngineMeshItem)
    
    material_name: StringProperty(
        name="Material",
        description="Material file name (e.g., default.mat)",
        default="default.mat"
    )
    
    # Tags
    tags: CollectionProperty(type=SecretEngineTag)
    tag_type: StringProperty(
        name="Type",
        description="Object type tag",
        default="mesh"
    )
    tag_name: StringProperty(
        name="Name",
        description="Object name tag",
        default=""
    )
    
    # Material Parameters
    use_custom_color: BoolProperty(
        name="Use Custom Color",
        description="Override material color",
        default=False
    )
    material_color: FloatVectorProperty(
        name="Color",
        subtype='COLOR',
        default=(0.8, 0.8, 0.8),
        min=0.0,
        max=1.0
    )
    
    # Culling
    culling_radius: FloatProperty(
        name="Culling Radius",
        description="Frustum culling radius",
        default=1.0,
        min=0.0
    )
    
    # LOD
    use_lod: BoolProperty(
        name="Use LOD",
        description="Enable Level of Detail",
        default=False
    )
    lod_levels: CollectionProperty(type=SecretEngineLODLevel)

class SecretEngineMeshGroupProperties(bpy.types.PropertyGroup):
    mesh: StringProperty(name="Mesh", default="")
    material: StringProperty(name="Material", default="")

class SecretEngineChunkProperties(bpy.types.PropertyGroup):
    chunk_id: StringProperty(
        name="Chunk ID",
        description="Unique identifier for this chunk",
        default="chunk_0"
    )
    export_path: StringProperty(
        name="Export Path",
        description="Path to export the chunk JSON",
        default="//chunk.json",
        subtype='FILE_PATH'
    )
    auto_mesh_from_name: BoolProperty(
        name="Auto Mesh from Name",
        description="Automatically set mesh name from object name (e.g., cube_1 → cube.meshbin)",
        default=True
    )
    mesh_extension: StringProperty(
        name="Mesh Extension",
        description="File extension for mesh files",
        default=".meshbin"
    )
