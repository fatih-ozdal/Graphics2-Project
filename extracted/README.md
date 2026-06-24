# Combat game logic — extracted module

`combat.hpp` is a **header-only, engine-agnostic** module containing the game
logic for the combat-flight-sim extension. Only dependency: **glm**. No OpenGL,
GLFW, threads, or anything from this repo's renderer.

```cpp
#include "combat.hpp"   // that's it
```

## What's inside

| Area | Status | Ported from |
|------|--------|-------------|
| Missile struct, spawn (`fireMissile`) | **extracted** | `src/main.cpp` `struct Missile`, spawn block ~L1192-1238; `missile.comp` L295/L304 |
| Missile integration + **terrain hit** (`updateMissile`, `raycastTerrain`) | **extracted** | `working_dir/shaders/missile.comp` `main()`, `traverse_grid`, `get_surrounding_vertices` |
| Player plane kinematics (`Plane`, `orientationFromForward`) | **extracted** | `src/main.cpp` render-loop quaternion math ~L911-979 |
| Enemy scripted trajectories (`EnemyPlane`, `updateEnemy`) | **new scaffold** | — (planned feature) |
| Missile-vs-enemy hit (`segmentSphere`) | **new scaffold** | — (planned feature) |
| Explosion sprite-sheet animation (`sampleSprite`) | **new scaffold** | — (planned feature) |
| Terrain crater deformation (`deformCrater`) | **new scaffold** | — (planned feature) |

The terrain collision algorithm is the real IP here: a 2D DDA (Amanatides–Woo)
grid walk in XZ that compares the missile's interpolated height against the
ground at every cell it crosses. In the original this ran in a compute shader
reading the terrain SSBO directly; here it's decoupled behind a `TerrainSampler`
height callback so it works against any terrain representation.

## The one thing you must provide: a height lookup

Everything terrain-related goes through `TerrainSampler`. Point it at your
terrain however you store it:

```cpp
combat::TerrainSampler terrain;
terrain.cellSize = 1.0f;                 // world units between ground samples
terrain.heightAt = [&](float x, float z) { return myTerrain.height(x, z); };
// optional: terrain.normalAt = ...  (else a central-difference normal is used)
```

## Minimal per-frame wiring

```cpp
std::vector<combat::Missile>     missiles;
std::vector<combat::EnemyPlane>  enemies;
combat::MissileConfig            cfg;

// fire (e.g. on spacebar): from your player's plane
combat::Plane player{ playerPos, playerOrientationQuat, playerSpeed };
missiles.push_back(combat::fireMissile(player, cfg));

// each frame:
for (auto& e : enemies) combat::updateEnemy(e, dt);

for (auto& m : missiles) {
    combat::MissileEvent ev = combat::updateMissile(m, dt, terrain, enemies, cfg);
    switch (ev.kind) {
        case combat::MissileEvent::Kind::HitTerrain:
            combat::deformCrater(heightGrid, ev.point, 12.0f, 6.0f, /*rim*/1.0f);
            // -> re-upload the touched vertices to your terrain VBO
            break;
        case combat::MissileEvent::Kind::HitEnemy:
            // spawn explosion at ev.point; enemies[ev.enemyIndex] is now dead
            break;
        default: break;
    }
}

// draw explosions for any exploded missile:
combat::SpriteSheet boom{ /*cols*/8, /*rows*/8, /*seconds*/0.6f, /*loop*/false };
for (auto& m : missiles) if (m.exploded) {
    combat::SpriteFrame f = combat::sampleSprite(boom, m.timeSinceExplosion);
    // billboard a quad at m.explosionPoint, sample atlas with f.uvMin/f.uvMax
}
```

## Notes / knobs

- **Rendering is yours.** This module computes *what* happens (positions, hits,
  frame indices, which vertices to lower). You still draw the meshes, billboards
  and terrain. `deformCrater` returns the indices it modified so you can
  re-upload just that sub-region of your height buffer.
- **Enemy paths**: set `EnemyPath::waypoints` and pick `smooth` (Catmull-Rom) or
  straight segments. Linear mode is exact constant-speed; smooth mode keeps speed
  approximately constant.
- **Tuning constants** (`MissileConfig`) mirror the originals: base speed ~450,
  `+ plane.speed*8`, blast radius 5, ~5000-unit range cull, 3 s explosion fuse.
- **Fast missiles don't tunnel**: enemy hits use a swept segment-vs-sphere test,
  terrain hits use the per-cell DDA, so nothing is skipped between frames.
- To split into `.cpp`, move the `inline` function bodies into a translation
  unit and drop the `inline` keyword.

A compile-tested usage example lives in the scratchpad `test_combat.cpp` used to
validate the header (`g++ -std=c++20 -I <glm> -I . test_combat.cpp`).
