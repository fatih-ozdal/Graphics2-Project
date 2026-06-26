# Combat Flight Simulator — CENG 469 Term Project

A small combat flight simulator built on top of the HW2 environment-mapped terrain
renderer. You fly a plane over a B-spline terrain, fire missiles, shoot down enemy
planes that follow chaotic Lorenz-attractor flight paths, and blow craters in the
ground. A write-up of the implementation and the bugs fought along the way is in
[blog/JOURNEY.md](blog/JOURNEY.md).

## Demo Video

The latest version of the project is demonstrated in the video below:

<p align="center">
  <a href="https://youtu.be/6n8BxB0Q7zs">
    <img src="https://img.youtube.com/vi/6n8BxB0Q7zs/0.jpg" alt="Demo Video"/>
  </a>
</p>

## Dependencies

Everything is vendored under `ext/` (GLFW, GLAD, GLM, stb_image), so the only system
requirements are:

- A C++20 compiler and CMake (≥ 3.10)
- OpenGL 4.3+ (compute shaders are used)
- On Linux: X11

## Building

The build uses the same CMake setup as the previous CENG 469 assignments. Use the same
build procedure you used for those — the executable lands in `build/working_dir/TerrainRenderer`
and should be run from the `working_dir/` directory.

## Controls

### Combat

| Key | Action |
| --- | --- |
| `Space` | Fire a missile |
| `N` | Spawn a new enemy plane (above the player plane) |
| `X` | Spawn a debug explosion at the player |
| `B` | Toggle the enemy bounding-sphere wireframes |

### Flight

| Input | Action |
| --- | --- |
| `W` / `S` | Accelerate / decelerate |
| `Q` / `E` | Roll (bank) left / right |
| Mouse left-drag | Orbit the camera |
| Mouse right-drag | Steer the plane (yaw / pitch) |
| Mouse scroll | Zoom the camera in / out |

### View & debug

| Key | Action |
| --- | --- |
| `←` / `→` | Decrease / increase terrain height exaggeration |
| `J` / `K` | Decrease / increase tessellation rate |
| `L` | Toggle wireframe rendering |
| `H` | Toggle HDR |
| `P` / `O` | Cycle the render/debug mode forward / back |
| `Enter` | Toggle fullscreen |

## Notes

- Run the executable from `working_dir/` — shaders, textures, meshes, and the explosion
  sprite sheet are all loaded with paths relative to that directory.
- If the window shows "Terrain Renderer is not responding" shortly after launch, just wait —
  the program is still working, not crashed.
- The project runs cleanly on the department lab machines (the inek machines), which is where
  it was developed and tested. On my own laptop (Mesa Intel drivers) it ran out of memory at
  startup, likely due to a driver-specific interaction with the large terrain SSBOs or my
  hardware. If the project does not run on your evaluation machine, please try it on an inek
  machine — it has been verified to work there.
