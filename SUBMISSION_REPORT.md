# TowerClimbDX12 Submission Report

## Project Members

- `[Fill in member name]` - `[Fill in student ID]`
- `[Fill in member name]` - `[Fill in student ID]`

## Implemented Environment

TowerClimbDX12 is a Windows desktop application built with Visual Studio, Win32, C++, and DirectX 12 on top of the Frank Luna framework. The project renders a 3D interior tower environment with toggleable free-fly and third-person camera modes, rotating climbable platforms, imported car and guardian models, a custom octagonal summit pedestal, generated textures applied through a shader-visible SRV heap, an animated stencil-masked portal on the tower floor, dynamic multi-light shading, alpha-blended goal and torch effects, geometry-shader-generated particles and billboards, and a small in-window control overlay for presentation. The scene also includes gameplay-style systems such as gravity, jumping, platform collision, and a win condition when the player reaches the orb at the top of the tower.

## Feature List

| Feature Group | Feature | Marks | Implemented? | Brief description |
| --- | --- | ---: | --- | --- |
| DirectX 12 | 3D environment successfully rendered using DirectX 12 | 10% | Yes | Tower interior, platforms, orb, car, lights, and effects are rendered through the DirectX 12 pipeline. |
| Camera | Movable camera (unchanged from demos) | 5% | No |  |
| Camera | Movable camera (first-person camera from demo) | 10% | No |  |
| Camera | Movable camera (different from demos) | 10% | Yes | Uses a toggleable hybrid camera system with a free-fly environment demo mode and a third-person follow camera, both with mouse look. |
| Geometric Objects | Using one of geometric objects directly from demos | 5% | No |  |
| Geometric Objects | Using a geometric object different than those in the demos | 10% | Yes | Adds a custom octagonal summit pedestal and custom portal meshes built manually in code. |
| 3D Models | Using an imported 3D model (e.g., skull, car, etc.) | 5% | Yes | Imports and renders `models/car.txt` and `models/guardian.txt` as external mesh assets loaded from disk. |
| Timing | Implementing a feature based on the total time | 5% | Yes | Platform rotation, orb pulsing, and torch flicker all depend on total elapsed time. |
| Movable Objects | One or more objects are movable (e.g., by key presses, mouse clicks, mouse drags, etc.) | 10% | No |  |
| Movable Objects | One or more objects move automatically | 15% | Yes | Floating platforms rotate continuously around the tower over time. |
| Lighting | Implement lighting (unchanged from demos) | 5% | No |  |
| Lighting | Implement lighting (different from demos) | 10% | Yes | Uses ambient light, a directional sun light, a camera-relative point light term, and 48 wall-mounted point lights. |
| Texture | Apply textures (unchanged from demos) | 5% | No |  |
| Texture | Apply textures (different from demos) | 10% | Yes | Uses generated GPU textures for walls, floor, platforms, car, pedestal, and portal effects. |
| Blending | Apply blending (unchanged from demos) | 5% | No |  |
| Blending | Apply blending (different from demos) | 10% | Yes | Uses alpha blending for the glowing goal orb, spark particles, and torch billboard flames. |
| Stenciling | Apply stenciling (unchanged from demos) | 5% | No |  |
| Stenciling | Apply stenciling (different from demos) | 10% | Yes | Uses a stencil mask plus a separate blended pass to render a circular animated portal on the floor. |
| Geometric Shader | Use geometric shader (different from demos) | 10% | Yes | Geometry shaders expand point primitives into sparks and upright torch billboards. |
| Others | Any other features (not seen in demos) | TBD (>10%) | Possible, not counted | Physics-based jumping, gravity, platform carry movement, collision, and a win condition are implemented, but this should only be counted with instructor approval. |

## Sources and Libraries Used

- Frank Luna, *Introduction to 3D Game Programming with DirectX 12*.
- Microsoft DirectX 12, DXGI, D3DCompiler, and DirectXMath.
- Frank Luna helper framework files in `Common/`, including `d3dApp`, `d3dUtil`, `GeometryGenerator`, and `Camera`.
- Microsoft `DDSTextureLoader` source included in `Common/`.
- Imported model assets: `models/car.txt` and `models/guardian.txt`.
- `models/guardian.txt` was authored for this project as a simple external mesh asset and loaded through the same imported-model pipeline as the car.
- Procedurally generated textures authored in code within `TowerGameApp.cpp`.
