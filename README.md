# 3D_Dice_Simulator
A 3D dice simulator with physics. Built using C++ and the Qt framework!

# 🎲 3D Dice Simulator

## Features
- Realistic 3D physics
- Click to add dice
- Sound effects
- Interactive camera controls

## Requirements 
- C++
- Qt6

## How to Build
```bash
g++ Dice_simulator.cpp -o dice_sim \
    -I/usr/include/x86_64-linux-gnu/qt6 \
    -lQt6Core -lQt6Widgets -lQt6Gui -lQt6OpenGL \
    -lQt63DCore -lQt63DExtras -lQt63DRender