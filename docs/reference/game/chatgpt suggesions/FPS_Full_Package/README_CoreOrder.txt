CORE LOAD ORDER:

1. InputPlugin
2. CameraPlugin
3. GameModePlugin
4. WeaponPlugin
5. BotPlugin
6. RendererPlugin

Reason:
- Camera needs input
- Weapon needs camera + input
- Bots need weapons
- Renderer last