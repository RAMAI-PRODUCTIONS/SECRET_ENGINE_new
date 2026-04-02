bl_info = {
    "name": "Secret Engine Level Editor",
    "author": "Secret Engine Team",
    "version": (1, 0, 0),
    "blender": (3, 0, 0),
    "location": "View3D > Sidebar > Secret Engine",
    "description": "Create and edit level chunks for Secret Engine",
    "category": "Import-Export",
}

import bpy
import importlib

# Handle module reloading for development
if "properties" in locals():
    importlib.reload(properties)
    importlib.reload(operators)
    importlib.reload(panels)
else:
    from . import properties
    from . import operators
    from . import panels

classes = (
    properties.SecretEngineTag,
    properties.SecretEngineLODLevel,
    properties.SecretEngineMeshItem,
    properties.SecretEngineInstanceProperties,
    properties.SecretEngineMeshGroupProperties,
    properties.SecretEngineChunkProperties,
    operators.SECRETENGINE_OT_ExportChunk,
    operators.SECRETENGINE_OT_ImportChunk,
    operators.SECRETENGINE_OT_AddTag,
    operators.SECRETENGINE_OT_RemoveTag,
    operators.SECRETENGINE_OT_AddLODLevel,
    operators.SECRETENGINE_OT_RemoveLODLevel,
    operators.SECRETENGINE_OT_AddMesh,
    operators.SECRETENGINE_OT_RemoveMesh,
    operators.SECRETENGINE_OT_SelectMeshGroup,
    panels.SECRETENGINE_PT_MainPanel,
    panels.SECRETENGINE_PT_HierarchyPanel,
    panels.SECRETENGINE_PT_ObjectPanel,
    panels.SECRETENGINE_PT_ChunkPanel,
)

def register():
    for cls in classes:
        bpy.utils.register_class(cls)
    
    bpy.types.Scene.secret_engine = bpy.props.PointerProperty(type=properties.SecretEngineChunkProperties)
    bpy.types.Object.secret_engine = bpy.props.PointerProperty(type=properties.SecretEngineInstanceProperties)

def unregister():
    # Remove properties first
    if hasattr(bpy.types.Scene, "secret_engine"):
        del bpy.types.Scene.secret_engine
    if hasattr(bpy.types.Object, "secret_engine"):
        del bpy.types.Object.secret_engine
    
    # Unregister classes in reverse order
    for cls in reversed(classes):
        try:
            bpy.utils.unregister_class(cls)
        except RuntimeError:
            pass  # Already unregistered

if __name__ == "__main__":
    register()
