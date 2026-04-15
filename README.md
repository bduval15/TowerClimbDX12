# TowerClimbDX12

## Project Description

TowerClimbDX12 is a Windows desktop environment demo built with Win32, C++, DirectX 12, and the Frank Luna framework. The scene is a large interior tower filled with rotating climb platforms, dynamic lighting, a glowing goal orb, an animated stencil portal, geometry-shader particles, generated textures, and imported 3D models.

## Build And Run

1. Open `TowerClimbDX12.vcxproj` or `TowerClimbDX12.slnx` in Visual Studio.
2. Use the `Debug` configuration and `x64` platform.
3. Build and run with `F5`.

If Visual Studio asks to retarget the project, accept the suggested Windows SDK update.

## Controls

The application shows the active controls in small text at the top-left corner of the window.

Default startup mode is `Free-Fly Demo`.

- `Mouse`: look around
- `F`: toggle between free-fly and third-person mode
- `Esc`: quit

Free-fly mode:
- `W/A/S/D`: move
- `Q/E`: move down/up
- `Shift`: faster fly speed

Third-person mode:
- `W/A/S/D`: move the character
- `Space`: jump
- `Q/E`: zoom out/in
- `Shift`: faster movement

## Main Features

- DirectX 12 tower environment with custom render items and multiple pipeline states
- Toggleable free-fly and third-person camera system
- Automatic moving platforms driven by elapsed time
- 48 point lights plus custom fog and atmospheric shading
- Generated textures for the environment, portal, and character
- Stencil-masked animated portal effect
- Geometry shader sparks and torch billboards
- Imported car model from `models/car.txt`
- Imported guardian character model from `models/guardian.txt`

## Project Structure

- `TowerGameApp.cpp` and `TowerGameApp.h`: main app logic, rendering, input, and gameplay systems
- `FrameResource.cpp` and `FrameResource.h`: frame resources and constant buffers
- `shaders/Default.hlsl`: lighting, fog, portal, and material shading
- `models/`: imported text-based mesh assets
- `Common/`: Frank Luna helper framework files

## Libraries And Sources

- Frank Luna, *Introduction to 3D Game Programming with DirectX 12*
- Microsoft DirectX 12, DXGI, D3DCompiler, and DirectXMath
- Frank Luna helper files in `Common/`
- Microsoft `DDSTextureLoader` source in `Common/`
- Imported mesh assets in `models/car.txt` and `models/guardian.txt`