# 🌃 Cyberpunk City Demo - Neon Rain

A procedurally generated cyberpunk cityscape with atmospheric rain effects, inspired by the Three.js demo you provided.

## ✨ Features

### Visual Effects
- **Procedural City Generation**: Buildings with varied heights, widths, and neon accents
- **Particle Systems**: 
  - 8,000 rain particles falling from the sky
  - 600 sparks floating through the air
  - 2,000 dust particles for atmosphere
- **Neon Lighting**: 
  - Glowing window strips on buildings
  - Rooftop beacons
  - Animated billboards
  - Dynamic colored lights moving through the scene
- **Atmospheric Effects**:
  - Fog with cyberpunk color palette (dark blue)
  - Metallic reflective ground
  - Emissive materials for neon glow

### Controls
- **Touch Controls** (Android):
  - Left joystick: Move (WASD equivalent)
  - Right side swipe: Look around (mouse look)
  - Jump button: Jump/fly up
- **First-Person Camera**: Smooth pitch/yaw rotation with movement

### Technical Implementation
- **Z-Up Coordinate System**: Proper vertical axis handling
- **Instanced Rendering**: Efficient rendering of repeated geometry
- **Component-Based Architecture**: Modular plugin system
- **Fast Protocol**: High-performance input handling

## 🚀 Building the Demo

### 1. Compile Shaders
```bash
./compile_shaders.bat
```

This compiles:
- Mega geometry shaders (buildings)
- Particle shaders (rain, sparks, dust)
- UI shaders (touch controls)

### 2. Build for Android
```bash
cd android
./gradlew assembleDebug
```

### 3. Install on Device
```bash
adb install app/build/outputs/apk/debug/app-debug.apk
```

## 📁 Key Files

### Plugins
- `plugins/ProceduralCityPlugin/` - Generates cyberpunk buildings
- `plugins/ParticleSystemPlugin/` - Rain and atmospheric effects
- `plugins/CameraPlugin/` - First-person camera controller
- `plugins/AndroidInput/` - Touch controls

### Assets
- `Assets/levels/cyberpunk_city.json` - Level configuration
- `Assets/ui_cyberpunk.json` - Touch UI layout
- `Assets/shaders/particle_*.spv` - Compiled particle shaders

### Shaders
- `plugins/VulkanRenderer/shaders/particle_vert.glsl` - Particle vertex shader
- `plugins/VulkanRenderer/shaders/particle_frag.glsl` - Particle fragment shader with soft edges
- `plugins/VulkanRenderer/shaders/mega_geometry_*.glsl` - Building rendering

## 🎨 Customization

### Adjust City Size
Edit `plugins/ProceduralCityPlugin/src/ProceduralCityPlugin.cpp`:
```cpp
const float spacing = 6.5f;  // Distance between buildings
const float radius = 42.0f;  // City radius
```

### Modify Particle Count
Edit `plugins/ParticleSystemPlugin/src/ParticleSystemPlugin.cpp`:
```cpp
for (int i = 0; i < 8000; i++) { // Rain count
for (int i = 0; i < 600; i++) {  // Spark count
for (int i = 0; i < 2000; i++) { // Dust count
```

### Change Colors
Edit `Assets/levels/cyberpunk_city.json`:
```json
"ambient_light": {
  "color": [0.07, 0.07, 0.13],  // RGB values
  "intensity": 0.3
}
```

## 🎮 Gameplay

The demo is an exploration experience:
1. Launch the app
2. Touch the left joystick to move through the city
3. Swipe the right side to look around
4. Explore the procedurally generated buildings
5. Watch the rain fall and neon lights pulse

## 🔧 Performance Notes

- **Target**: 60 FPS on mid-range Android devices
- **Particle Count**: Optimized to 10,600 total particles
- **Culling**: Frustum culling for buildings outside view
- **LOD**: Buildings use simple cube geometry for performance

## 📊 Comparison to Three.js Demo

| Feature | Three.js Demo | SecretEngine Demo |
|---------|---------------|-------------------|
| Buildings | Procedural boxes | Procedural boxes ✅ |
| Rain | 18,000 particles | 8,000 particles (optimized) |
| Sparks | 1,200 particles | 600 particles (optimized) |
| Dust | 4,000 particles | 2,000 particles (optimized) |
| Neon Lights | Dynamic colors | Dynamic colors ✅ |
| First-Person | Mouse + WASD | Touch + Joystick ✅ |
| Fog | Exponential | Exponential ✅ |
| Platform | Web (WebGL) | Android (Vulkan) ✅ |

## 🐛 Known Limitations

1. **Particle Rendering**: Currently uses small cubes instead of point sprites (will be optimized)
2. **Bloom Effect**: Post-processing not yet implemented
3. **Reflections**: Ground reflections simplified
4. **Building Variety**: Limited to boxes (torus/cylinder shapes coming)

## 🚧 Future Enhancements

- [ ] GPU-based particle system (compute shaders)
- [ ] Bloom post-processing effect
- [ ] Screen-space reflections
- [ ] More building shapes (cylinders, torus, complex geometry)
- [ ] Animated hologram effects
- [ ] Sound effects (rain, ambient city noise)
- [ ] Day/night cycle
- [ ] Weather transitions

## 📝 Notes

This demo showcases SecretEngine's capabilities:
- Plugin architecture for modular features
- Vulkan rendering with modern effects
- Component-based entity system
- Cross-platform input handling
- Procedural content generation

The experience is similar to the Three.js demo but optimized for mobile devices with touch controls!

---

**Built with SecretEngine** 🎮
*A modern C++ game engine with Vulkan rendering*
