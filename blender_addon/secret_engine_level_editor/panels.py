import bpy

class SECRETENGINE_PT_MainPanel(bpy.types.Panel):
    bl_label = "Secret Engine Level Editor"
    bl_idname = "SECRETENGINE_PT_main_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Secret Engine'
    
    def draw(self, context):
        layout = self.layout
        scene = context.scene
        
        layout.label(text="Level Chunk Export/Import", icon='WORLD')
        
        row = layout.row()
        row.operator("secret_engine.export_chunk", icon='EXPORT')
        row.operator("secret_engine.import_chunk", icon='IMPORT')

class SECRETENGINE_PT_HierarchyPanel(bpy.types.Panel):
    bl_label = "Level Hierarchy"
    bl_idname = "SECRETENGINE_PT_hierarchy_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Secret Engine'
    bl_parent_id = "SECRETENGINE_PT_main_panel"
    bl_options = {'DEFAULT_CLOSED'}
    
    def draw(self, context):
        layout = self.layout
        scene = context.scene
        
        # Build hierarchy data
        mesh_groups = {}
        total_objects = 0
        
        for obj in scene.objects:
            if obj.type != 'MESH':
                continue
            
            total_objects += 1
            props = obj.secret_engine
            
            # Determine mesh key
            if props.mesh_mode == 'ARRAY' and len(props.meshes) > 0:
                mesh_key = tuple(m.mesh_name for m in props.meshes)
            else:
                mesh_key = (props.mesh_name,)
            
            key = (mesh_key, props.material_name)
            
            if key not in mesh_groups:
                mesh_groups[key] = []
            
            mesh_groups[key].append(obj)
        
        # Display summary
        box = layout.box()
        box.label(text=f"Total Objects: {total_objects}", icon='OBJECT_DATA')
        box.label(text=f"Mesh Groups: {len(mesh_groups)}", icon='GROUP')
        
        # Display each mesh group
        if len(mesh_groups) == 0:
            layout.label(text="No mesh objects in scene", icon='INFO')
            return
        
        layout.separator()
        
        for (mesh_key, material), objects in sorted(mesh_groups.items(), key=lambda x: len(x[1]), reverse=True):
            box = layout.box()
            
            # Header with mesh group info
            row = box.row()
            row.label(text=f"Group ({len(objects)} instances)", icon='MESH_DATA')
            
            # Select button
            op = row.operator("secret_engine.select_mesh_group", text="", icon='RESTRICT_SELECT_OFF')
            op.mesh_key = str(mesh_key)
            op.material = material
            
            # Mesh info
            if len(mesh_key) == 1:
                box.label(text=f"Mesh: {mesh_key[0]}", icon='MESH_CUBE')
            else:
                box.label(text=f"Meshes: {len(mesh_key)} (Array)", icon='MESH_CUBE')
                for i, mesh_name in enumerate(mesh_key):
                    box.label(text=f"  [{i}] {mesh_name}")
            
            # Material info
            box.label(text=f"Material: {material}", icon='MATERIAL')
            
            # Instance list (collapsible)
            col = box.column(align=True)
            
            # Show first 5 instances, then summary
            display_count = min(5, len(objects))
            for i, obj in enumerate(objects[:display_count]):
                row = col.row(align=True)
                
                # Object name with icon
                icon = 'OUTLINER_OB_MESH' if obj.select_get() else 'MESH_DATA'
                row.label(text=f"  • {obj.name}", icon=icon)
                
                # Quick select button
                op = row.operator("object.select_all", text="", icon='RESTRICT_SELECT_OFF')
                op.action = 'DESELECT'
            
            if len(objects) > display_count:
                col.label(text=f"  ... and {len(objects) - display_count} more")
            
            layout.separator()

class SECRETENGINE_PT_ChunkPanel(bpy.types.Panel):
    bl_label = "Chunk Settings"
    bl_idname = "SECRETENGINE_PT_chunk_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Secret Engine'
    bl_parent_id = "SECRETENGINE_PT_main_panel"
    
    def draw(self, context):
        layout = self.layout
        scene = context.scene
        props = scene.secret_engine
        
        layout.prop(props, "chunk_id")
        
        layout.separator()
        
        box = layout.box()
        box.label(text="Mesh Naming", icon='MESH_DATA')
        box.prop(props, "auto_mesh_from_name")
        
        if props.auto_mesh_from_name:
            box.label(text="Auto mode enabled:", icon='INFO')
            box.label(text="  cube_1 → cube.meshbin")
            box.label(text="  Sphere.001 → sphere.meshbin")
            box.prop(props, "mesh_extension")
        else:
            box.label(text="Manual mode:", icon='INFO')
            box.label(text="Set mesh per object")

class SECRETENGINE_PT_ObjectPanel(bpy.types.Panel):
    bl_label = "Object Properties"
    bl_idname = "SECRETENGINE_PT_object_panel"
    bl_space_type = 'VIEW_3D'
    bl_region_type = 'UI'
    bl_category = 'Secret Engine'
    bl_parent_id = "SECRETENGINE_PT_main_panel"
    
    @classmethod
    def poll(cls, context):
        return context.active_object is not None and context.active_object.type == 'MESH'
    
    def draw(self, context):
        layout = self.layout
        obj = context.active_object
        props = obj.secret_engine
        
        # Mesh and Material
        box = layout.box()
        box.label(text="Mesh & Material", icon='MESH_DATA')
        
        # Mesh mode selector
        box.prop(props, "mesh_mode", text="Mode")
        
        if props.mesh_mode == 'SINGLE':
            # Single mesh mode
            box.prop(props, "mesh_name")
        else:
            # Array mode
            box.label(text="Meshes (Array):")
            for i, mesh_item in enumerate(props.meshes):
                row = box.row()
                row.prop(mesh_item, "mesh_name", text=f"Mesh {i}")
                op = row.operator("secret_engine.remove_mesh", text="", icon='X')
                op.index = i
            
            box.operator("secret_engine.add_mesh", icon='ADD')
        
        box.prop(props, "material_name")
        
        # Tags
        box = layout.box()
        box.label(text="Tags", icon='BOOKMARKS')
        box.prop(props, "tag_type")
        box.prop(props, "tag_name")
        
        # Custom Tags
        box.label(text="Custom Tags:")
        for i, tag in enumerate(props.tags):
            row = box.row()
            row.prop(tag, "key", text="")
            row.prop(tag, "value", text="")
            op = row.operator("secret_engine.remove_tag", text="", icon='X')
            op.index = i
        
        box.operator("secret_engine.add_tag", icon='ADD')
        
        # Material Parameters
        box = layout.box()
        box.label(text="Material Parameters", icon='MATERIAL')
        box.prop(props, "use_custom_color")
        if props.use_custom_color:
            box.prop(props, "material_color")
        
        # Culling
        box = layout.box()
        box.label(text="Culling", icon='SPHERE')
        box.prop(props, "culling_radius")
        
        # LOD
        box = layout.box()
        box.label(text="Level of Detail", icon='OUTLINER_DATA_MESH')
        box.prop(props, "use_lod")
        
        if props.use_lod:
            for i, lod in enumerate(props.lod_levels):
                row = box.row()
                row.prop(lod, "distance", text=f"LOD {i}")
                op = row.operator("secret_engine.remove_lod_level", text="", icon='X')
                op.index = i
            
            box.operator("secret_engine.add_lod_level", icon='ADD')
