# Runtime UI Configuration System

## Overview

The UI system now supports runtime configuration through a JSON file (`ui_config.json`). You can modify the UI layout, colors, positions, and behavior without recompiling the code.

## Features

- **Hot Reload**: Changes to `ui_config.json` are automatically detected and applied every 2 seconds
- **Configurable Buttons**: Add, remove, or modify UI buttons (labels, colors, actions)
- **Joystick Customization**: Adjust position, size, colors, and sensitivity
- **Touch Input Settings**: Configure sensitivity for joystick and camera look
- **Screen Zones**: Enable/disable different touch input areas
- **Layout Control**: Fine-tune spacing, sizing, and visual appearance

## Configuration File: `ui_config.json`

### Screen Zones

Controls which areas of the screen are active for input:

```json
"screen_zones": {
  "ui_buttons_zone": {
    "enabled": true,
    "position": "top",
    "height_percent": 15.0
  },
  "joystick_zone": {
    "enabled": true,
    "side": "left"
  },
  "look_zone": {
    "enabled": true,
    "side": "right"
  }
}
```

- `height_percent`: Percentage of screen height for UI buttons (0-100)
- Set `enabled: false` to disable any zone

### UI Buttons

Define buttons that appear at the top of the screen:

```json
"ui_buttons": [
  {
    "id": "btn_menu",
    "label": "MENU",
    "action": "MainMenu",
    "position_index": 0,
    "color": [0.2, 0.3, 0.5],
    "text_color": [1.0, 1.0, 1.0],
    "visible": true
  }
]
```

- `id`: Unique identifier for the button
- `label`: Text displayed on the button
- `action`: Level/scene name to load when pressed
- `position_index`: Order of the button (0 = leftmost)
- `color`: RGB values (0.0 to 1.0) for button background
- `text_color`: RGB values for button text
- `visible`: Set to `false` to hide the button

**To hide all buttons**: Set `visible: false` for each button, or set `screen_zones.ui_buttons_zone.enabled: false`

### Joystick Configuration

Customize the movement joystick appearance and behavior:

```json
"joystick": {
  "enabled": true,
  "position": {
    "x_ndc": -0.7,
    "y_ndc": 0.5
  },
  "size": {
    "base_size": 0.3,
    "stick_size": 0.1,
    "max_offset_multiplier": 0.5
  },
  "colors": {
    "base": [0.2, 0.2, 0.2],
    "stick": [1.0, 1.0, 1.0]
  },
  "touch_radius_pixels": 100.0,
  "visible": true
}
```

- `x_ndc`, `y_ndc`: Position in NDC coordinates (-1.0 to 1.0)
  - X: -1.0 = left edge, 1.0 = right edge
  - Y: -1.0 = top edge, 1.0 = bottom edge
- `base_size`: Size of the joystick base circle
- `stick_size`: Size of the inner stick
- `max_offset_multiplier`: How far the stick can move (0.0 to 1.0)
- `touch_radius_pixels`: Touch detection radius in pixels
- `visible`: Set to `false` to hide the joystick

**To hide the joystick**: Set `visible: false` or `enabled: false`

### Touch Input Settings

Adjust input sensitivity:

```json
"touch_input": {
  "joystick_sensitivity": 1.0,
  "look_sensitivity": 10.0,
  "fire_on_tap": true
}
```

- `joystick_sensitivity`: Movement speed multiplier (default: 1.0)
- `look_sensitivity`: Camera rotation speed multiplier (default: 10.0)
- `fire_on_tap`: Enable/disable shooting when tapping right side of screen

### Layout Settings

Fine-tune visual spacing:

```json
"layout": {
  "button_inset": 0.01,
  "button_text_scale": 0.035,
  "button_text_spacing": 0.045,
  "button_text_y_offset": 0.3
}
```

## Common Use Cases

### 1. Hide All UI Buttons (Touch Input and Joystick Only)

Set all buttons to invisible:

```json
"ui_buttons": [
  {
    "id": "btn_menu",
    "visible": false
  },
  {
    "id": "btn_scene",
    "visible": false
  },
  {
    "id": "btn_arena",
    "visible": false
  },
  {
    "id": "btn_race",
    "visible": false
  }
]
```

Or disable the entire button zone:

```json
"screen_zones": {
  "ui_buttons_zone": {
    "enabled": false
  }
}
```

### 2. Joystick Only (No Buttons, No Look Controls)

```json
"screen_zones": {
  "ui_buttons_zone": {
    "enabled": false
  },
  "joystick_zone": {
    "enabled": true
  },
  "look_zone": {
    "enabled": false
  }
}
```

### 3. Move Joystick to Bottom Right

```json
"joystick": {
  "position": {
    "x_ndc": 0.7,
    "y_ndc": 0.7
  }
}
```

### 4. Add a New Button

Add a new button object to the `ui_buttons` array:

```json
{
  "id": "btn_custom",
  "label": "CUSTOM",
  "action": "CustomLevel",
  "position_index": 4,
  "color": [0.8, 0.2, 0.8],
  "text_color": [1.0, 1.0, 1.0],
  "visible": true
}
```

### 5. Increase Joystick Sensitivity

```json
"touch_input": {
  "joystick_sensitivity": 2.0,
  "look_sensitivity": 15.0
}
```

## Testing Changes

1. Edit `ui_config.json` in your project root
2. Save the file
3. Wait up to 2 seconds for hot reload
4. Changes will be applied automatically (check logs for "🔄 UI config reloaded")

## File Location

The configuration file should be placed at:
- **Project Root**: `ui_config.json`
- The file is loaded when the AndroidInput plugin initializes
- Hot reload checks for file changes every 2 seconds

## Troubleshooting

- **Changes not applying**: Check the log for "UI config reloaded" message
- **Buttons not showing**: Verify `visible: true` and `enabled: true` in screen zones
- **Joystick not responding**: Check `enabled: true` and `visible: true` in joystick config
- **Invalid JSON**: Use a JSON validator to check syntax errors

## Technical Details

- **Coordinate System**: NDC (Normalized Device Coordinates)
  - X: -1.0 (left) to 1.0 (right)
  - Y: -1.0 (top) to 1.0 (bottom)
- **Colors**: RGB format with values from 0.0 to 1.0
- **Hot Reload**: File modification time is checked every 2 seconds
- **Default Values**: If config file is missing, hardcoded defaults are used

## Architecture

The UI configuration system consists of:

1. **UIConfig.h/cpp**: Configuration loader and parser
2. **InputPlugin.h**: Touch input handling with config integration
3. **RendererPlugin.cpp**: UI rendering using config data
4. **ui_config.json**: Runtime configuration file

All code modifications preserve existing plugin functionality while adding runtime configurability.
