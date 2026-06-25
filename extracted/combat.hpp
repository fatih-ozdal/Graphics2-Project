// =============================================================================
//  combat.hpp  —  Self-contained combat-flight-sim game logic
// -----------------------------------------------------------------------------
//  Extracted from the HW2 terrain renderer so it can be dropped into any
//  codebase. The ONLY dependency is glm. No OpenGL, no GLFW, no threads.
//
//  Header-only: drop this file in, `#include "combat.hpp"`, done. (If you'd
//  rather have a .cpp, move the function bodies out and strip `inline`.)
//
//  Provenance (what each part was ported from):
//    * Missile struct + spawn ............ src/main.cpp  (struct Missile,
//                                          find_inactive_missile, missile
//                                          dispatch block ~L1192-1238)
//    * Missile integrate + terrain hit ... working_dir/shaders/missile.comp
//                                          (main(), traverse_grid,
//                                           get_surrounding_vertices, get_vertex)
//    * Plane kinematics / fire direction . src/main.cpp  (render-loop quaternion
//                                          math ~L911-979, spawn at L1207-1212;
//                                          missile.comp L295,L304)
//
//  NEW scaffolding (planned features, not present in the original repo — written
//  in the same conventions so they slot in cleanly):
//    * Enemy plane scripted trajectories
//    * Missile-vs-enemy (swept-sphere) hit detection
//    * Explosion sprite-sheet frame animation
//    * Terrain crater deformation
//
//  Coordinate conventions (match the original project):
//    * Y is up. Terrain height is a function of world (x, z).
//    * A plane's forward is orientation * (1,0,0); up is orientation * (0,1,0).
// =============================================================================
#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp>   // glm::quat, glm::quat_cast (core, non-experimental)
#include <cstdint>
#include <functional>
#include <vector>
#include <optional>
#include <cmath>

namespace combat
{

// =============================================================================
//  0.  Small math helpers
// =============================================================================

inline float smoothstep01(float x)
{
    x = glm::clamp(x, 0.0f, 1.0f);
    return x * x * (3.0f - 2.0f * x);
}

// Build an orientation whose forward (+X) points along `forward`.
// Matches the project's basis: forward = q*(1,0,0), up = q*(0,1,0).
inline glm::quat orientationFromForward(const glm::vec3& forward,
                                        const glm::vec3& worldUp = glm::vec3(0, 1, 0))
{
    glm::vec3 f = glm::normalize(forward);
    // Orthonormal frame whose local +X is `f` and local +Y stays near worldUp.
    glm::vec3 up = worldUp - f * glm::dot(worldUp, f);
    if (glm::dot(up, up) < 1e-8f)                  // forward ~parallel to worldUp
        up = glm::vec3(0, 0, 1) - f * f.z;
    up = glm::normalize(up);
    glm::vec3 side = glm::normalize(glm::cross(f, up));   // right-handed 3rd axis
    // Columns are the images of local (X,Y,Z): X->f, Y->up, Z->side.
    glm::mat3 basis(f, up, side);
    return glm::normalize(glm::quat_cast(basis));
}

// Swept test: segment p0->p1 (a fast-moving point) against sphere (c, r).
// Returns the earliest parametric time t in [0,1] of contact. Avoids the
// tunnelling you'd get from a plain point-in-sphere test on fast missiles.
inline bool segmentSphere(const glm::vec3& p0, const glm::vec3& p1,
                          const glm::vec3& c, float r, float& tHit)
{
    glm::vec3 d = p1 - p0;
    glm::vec3 m = p0 - c;
    float cc = glm::dot(m, m) - r * r;
    if (cc <= 0.0f) { tHit = 0.0f; return true; }      // started inside
    float a = glm::dot(d, d);
    if (a < 1e-12f) return false;                       // not moving
    float b = glm::dot(m, d);
    if (b > 0.0f) return false;                         // moving away
    float disc = b * b - a * cc;
    if (disc < 0.0f) return false;                      // misses
    float t = (-b - std::sqrt(disc)) / a;
    if (t < 0.0f || t > 1.0f) return false;
    tHit = t;
    return true;
}

// =============================================================================
//  1.  Terrain access (decoupled from any particular renderer)
// -----------------------------------------------------------------------------
//  The original collision read the tessellated terrain SSBO directly inside a
//  compute shader. To stay engine-agnostic we instead require a height lookup.
//  Plug in whatever your terrain is (heightmap, B-spline eval, raycast, ...).
//
//  `cellSize` is the world distance between adjacent ground samples; it controls
//  the DDA step. In the original this was 1/(tesplus-1) terrain units.
// =============================================================================
struct TerrainSampler
{
    std::function<float(float x, float z)> heightAt;
    std::function<glm::vec3(float x, float z)> normalAt;   // optional
    float cellSize = 1.0f;

    glm::vec3 normalOrUp(float x, float z) const
    {
        if (normalAt) return normalAt(x, z);
        // Central-difference fallback if no analytic normal was supplied.
        float e = cellSize;
        float hL = heightAt(x - e, z), hR = heightAt(x + e, z);
        float hD = heightAt(x, z - e), hU = heightAt(x, z + e);
        return glm::normalize(glm::vec3(hL - hR, 2.0f * e, hD - hU));
    }
};

// 2D DDA across terrain cells in XZ, testing whether the segment p0->p1 drops
// below the sampled ground. Faithful port of missile.comp:traverse_grid — walks
// every cell the segment crosses (so it can't skip a thin ridge), interpolating
// the missile's Y across each cell and comparing to ground height.
struct TerrainHit { glm::vec3 point; glm::vec3 normal; };

inline std::optional<TerrainHit> raycastTerrain(const TerrainSampler& terrain,
                                                const glm::vec3& p0,
                                                const glm::vec3& p1)
{
    const float cell = terrain.cellSize;
    glm::vec2 start = glm::vec2(p0.x, p0.z) / cell;
    glm::vec2 end   = glm::vec2(p1.x, p1.z) / cell;
    glm::vec2 dir   = end - start;

    glm::ivec2 cur  = glm::ivec2(glm::floor(start));
    glm::ivec2 last = glm::ivec2(glm::floor(end));
    glm::ivec2 step = glm::ivec2(glm::sign(dir));
    glm::vec2  ds   = glm::max(glm::abs(dir), glm::vec2(1e-6f));

    glm::vec2 tMax;
    tMax.x = (step.x > 0) ? (float(cur.x) + 1.0f - start.x) / ds.x
                          : (start.x - float(cur.x)) / ds.x;
    tMax.y = (step.y > 0) ? (float(cur.y) + 1.0f - start.y) / ds.y
                          : (start.y - float(cur.y)) / ds.y;
    glm::vec2 tDelta = 1.0f / ds;

    int nSteps = std::abs(last.x - cur.x) + std::abs(last.y - cur.y);
    float tEnter = 0.0f;

    for (int i = 0; i <= nSteps; ++i)
    {
        float wx = (float(cur.x) + 0.5f) * cell;
        float wz = (float(cur.y) + 0.5f) * cell;
        float ground = terrain.heightAt(wx, wz);

        float tExit = glm::min(glm::min(tMax.x, tMax.y), 1.0f);
        float tMid  = (tEnter + tExit) * 0.5f;
        float missileY = glm::mix(p0.y, p1.y, tMid);

        if (missileY <= ground)                          // BOOM (the L254 test)
        {
            glm::vec3 hit = glm::mix(p0, p1, tMid);
            return TerrainHit{ hit, terrain.normalOrUp(wx, wz) };
        }

        if (tMax.x < tMax.y) { tMax.x += tDelta.x; cur.x += step.x; }
        else                 { tMax.y += tDelta.y; cur.y += step.y; }
        tEnter = tExit;
    }
    return std::nullopt;
}

// =============================================================================
//  2.  Player plane + missiles
// =============================================================================
struct Plane
{
    glm::vec3 position{ 0.0f };
    glm::quat orientation{ 1, 0, 0, 0 };
    float     speed = 0.0f;
    float     hitRadius = 5.0f;          // for being shot at

    glm::vec3 forward() const { return orientation * glm::vec3(0, 0, 1); } // +Z
    glm::vec3 up()      const { return orientation * glm::vec3(0, 1, 0); }
};

struct MissileConfig
{
    float baseSpeed   = 450.0f;   // original: 50 + ...(plane) + 400  -> ~450 floor
    float planeBoost  = 8.0f;     // missile.comp L304: + plane_speed * 8
    float blastRadius = 5.0f;     // missile.comp L302
    float maxRangeSq  = 25'000'000.0f; // missile.comp L321: far-cull beyond ~5000u
    float fuseSeconds = 3.0f;     // missile.comp L340: explosion lifetime
};

struct Missile
{
    glm::vec3 position{ 0.0f };
    glm::vec3 direction{ 1, 0, 0 };  // unit
    glm::vec3 origin{ 0.0f };        // spawn point, used for range culling
    float speed = 0.0f;
    float blastRadius = 5.0f;

    bool  active   = false;          // in flight
    bool  exploded = false;          // currently playing its explosion
    float timeSinceExplosion = 0.0f;

    glm::vec3 explosionPoint{ 0.0f };
    glm::vec3 surfaceNormal{ 0, 1, 0 };
};

// Port of the "activate_this" branch in missile.comp (L294-308) + main.cpp spawn.
inline Missile fireMissile(const Plane& plane, const MissileConfig& cfg)
{
    glm::vec3 f = plane.forward();
    glm::vec3 u = plane.up();

    Missile m;
    m.position    = plane.position + 5.0f * f - 2.0f * u;   // spawn under the nose
    m.origin      = m.position;
    m.direction   = f;
    m.speed       = cfg.baseSpeed + plane.speed * cfg.planeBoost;
    m.blastRadius = cfg.blastRadius;
    m.active      = true;
    return m;
}

// What a single missile update produced this frame.
struct MissileEvent
{
    enum class Kind { None, HitTerrain, HitEnemy, Expired } kind = Kind::None;
    glm::vec3 point{ 0.0f };
    glm::vec3 normal{ 0, 1, 0 };
    int       enemyIndex = -1;       // valid when kind == HitEnemy
};

// Forward decl (enemies live below).
struct EnemyPlane;

// Advance one missile by dt. Mirrors missile.comp main():
//   - integrate position along direction
//   - far-cull if it has flown too far from where it started
//   - swept-sphere test against every alive enemy (NEW)
//   - DDA terrain raycast (ported)
//   - when exploded, count up the fuse and retire after fuseSeconds
// Returns the event so the caller can spawn explosions / score / deform terrain.
inline MissileEvent updateMissile(Missile& m, float dt,
                                  const TerrainSampler& terrain,
                                  std::vector<EnemyPlane>& enemies,
                                  const MissileConfig& cfg);

// =============================================================================
//  3.  Enemy planes on scripted paths   (NEW — planned feature)
// -----------------------------------------------------------------------------
//  The enemy follows a Lorenz attractor (chaotic, looping-but-never-repeating
//  flight). We integrate the attractor each frame (RK4), map its coordinates
//  into the play volume with `scale`/`translate`, and slerp the orientation
//  toward the direction of travel so the model banks smoothly.
//
//  Lorenz system:  x' = sigma*(y-x)
//                  y' = x*(rho-z) - y
//                  z' = x*y - beta*z
// =============================================================================
struct EnemyPath
{
    float sigma = 10.0f;             // classic chaotic regime
    float rho   = 28.0f;
    float beta  = 8.0f / 3.0f;
    float timeScale = 1.0f;          // attractor-seconds advanced per real second
    glm::vec3 scale     { 1.0f };    // attractor units -> world units
    glm::vec3 translate { 0.0f };    // recenter the attractor into the play volume
    float turnRate = 4.0f;           // orientation slerp responsiveness (per sec)
};

struct EnemyPlane
{
    EnemyPath path;
    glm::vec3 state { 0.1f, 0.0f, 0.0f };   // Lorenz (x,y,z); must be nonzero seed
    glm::vec3 position{ 0.0f };
    glm::quat orientation{ 1, 0, 0, 0 };
    float     hitRadius = 6.0f;             // bounding-sphere radius
    bool      alive = true;
    float     hitTimer = 0.0f;              // >0 = dying: keep drawing with a red flash
    bool      initialized = false;          // first frame seeds position
};

namespace detail
{
    inline glm::vec3 lorenzDeriv(const glm::vec3& s, const EnemyPath& p)
    {
        return glm::vec3(p.sigma * (s.y - s.x),
                         s.x * (p.rho - s.z) - s.y,
                         s.x * s.y - p.beta * s.z);
    }
}

inline void updateEnemy(EnemyPlane& e, float dt)
{
    if (!e.alive) return;

    // --- RK4 integrate the Lorenz attractor (stable for the chaotic regime) ---
    float h = dt * e.path.timeScale;
    glm::vec3 s  = e.state;
    glm::vec3 k1 = detail::lorenzDeriv(s, e.path);
    glm::vec3 k2 = detail::lorenzDeriv(s + 0.5f * h * k1, e.path);
    glm::vec3 k3 = detail::lorenzDeriv(s + 0.5f * h * k2, e.path);
    glm::vec3 k4 = detail::lorenzDeriv(s + h * k3, e.path);
    e.state = s + (h / 6.0f) * (k1 + 2.0f * k2 + 2.0f * k3 + k4);

    // --- Map attractor coords into the play volume ---
    glm::vec3 newPos = e.state * e.path.scale + e.path.translate;
    if (!e.initialized) { e.position = newPos; e.initialized = true; }

    // --- Slerp orientation toward the direction of travel ---
    glm::vec3 vel = newPos - e.position;
    if (glm::dot(vel, vel) > 1e-8f)
    {
        glm::quat target = orientationFromForward(glm::normalize(vel));
        float t = glm::clamp(e.path.turnRate * dt, 0.0f, 1.0f);
        e.orientation = glm::slerp(e.orientation, target, t);
    }
    e.position = newPos;
}

// ---- updateMissile definition (needs the full EnemyPlane type) --------------
inline MissileEvent updateMissile(Missile& m, float dt,
                                  const TerrainSampler& terrain,
                                  std::vector<EnemyPlane>& enemies,
                                  const MissileConfig& cfg)
{
    MissileEvent ev;
    if (!m.active) return ev;

    if (m.exploded)
    {
        m.timeSinceExplosion += dt;
        if (m.timeSinceExplosion > cfg.fuseSeconds)   // retire after the blast
        {
            m.active = m.exploded = false;
            ev.kind = MissileEvent::Kind::Expired;
        }
        return ev;
    }

    glm::vec3 oldPos = m.position;
    glm::vec3 newPos = oldPos + m.direction * m.speed * dt;

    // Far-cull (missile.comp L321): give up once it's strayed too far.
    glm::vec3 fromOrigin = newPos - m.origin;
    if (glm::dot(fromOrigin, fromOrigin) > cfg.maxRangeSq)
    {
        m.active = false;
        ev.kind = MissileEvent::Kind::Expired;
        return ev;
    }

    // --- Enemy hit first (swept sphere), keep the earliest along the segment.
    float bestT = 2.0f;
    int   bestEnemy = -1;
    for (int i = 0; i < (int)enemies.size(); ++i)
    {
        if (!enemies[i].alive) continue;
        float t;
        if (segmentSphere(oldPos, newPos, enemies[i].position,
                          enemies[i].hitRadius + m.blastRadius, t) && t < bestT)
        {
            bestT = t; bestEnemy = i;
        }
    }

    // --- Terrain hit along the same segment.
    std::optional<TerrainHit> terrainHit = raycastTerrain(terrain, oldPos, newPos);
    float terrainT = 2.0f;
    if (terrainHit)
    {
        glm::vec3 seg = newPos - oldPos;
        float segLen2 = glm::dot(seg, seg);
        terrainT = (segLen2 > 1e-9f)
                 ? glm::dot(terrainHit->point - oldPos, seg) / segLen2 : 0.0f;
    }

    // Whichever comes first wins.
    if (bestEnemy >= 0 && bestT <= terrainT)
    {
        m.position = glm::mix(oldPos, newPos, bestT);
        m.explosionPoint = m.position;
        m.surfaceNormal = glm::normalize(m.position - enemies[bestEnemy].position);
        enemies[bestEnemy].alive = false;
        m.exploded = true; m.timeSinceExplosion = 0.0f;
        ev.kind = MissileEvent::Kind::HitEnemy;
        ev.point = m.position; ev.normal = m.surfaceNormal; ev.enemyIndex = bestEnemy;
        return ev;
    }
    if (terrainHit)
    {
        m.position = terrainHit->point;
        m.explosionPoint = terrainHit->point;
        m.surfaceNormal = terrainHit->normal;
        m.exploded = true; m.timeSinceExplosion = 0.0f;
        ev.kind = MissileEvent::Kind::HitTerrain;
        ev.point = terrainHit->point; ev.normal = terrainHit->normal;
        return ev;
    }

    m.position = newPos;        // free flight
    return ev;
}

// =============================================================================
//  4.  Explosion sprite-sheet animation   (NEW — planned feature)
// -----------------------------------------------------------------------------
//  Drive a frame-by-frame sprite sheet from a missile's `timeSinceExplosion`.
//  Returns the cell index and its UV rectangle for a cols x rows atlas.
// =============================================================================
struct SpriteSheet
{
    int   cols = 1, rows = 1;
    float duration = 1.0f;     // seconds for the whole sequence
    bool  loop = false;
};

struct SpriteFrame
{
    int       index = 0;
    glm::vec2 uvMin{ 0.0f };
    glm::vec2 uvMax{ 1.0f };
    bool      finished = false;
};

inline SpriteFrame sampleSprite(const SpriteSheet& s, float elapsed)
{
    int frames = glm::max(1, s.cols * s.rows);
    float t = (s.duration > 0.0f) ? elapsed / s.duration : 1.0f;

    SpriteFrame f;
    if (s.loop) t = t - std::floor(t);
    else if (t >= 1.0f) { t = 1.0f; f.finished = true; }

    f.index = glm::clamp(int(t * frames), 0, frames - 1);
    int cx = f.index % s.cols;
    int cy = f.index / s.cols;
    glm::vec2 cell = glm::vec2(1.0f / s.cols, 1.0f / s.rows);
    f.uvMin = glm::vec2(cx, cy) * cell;
    f.uvMax = f.uvMin + cell;
    return f;
}

// =============================================================================
//  5.  Terrain crater deformation   (NEW — planned feature)
// -----------------------------------------------------------------------------
//  Lower a heightmap around an impact to carve a crater (optional raised rim).
//  Works on a row-major height array; returns the indices it touched so you can
//  re-upload just that region to the GPU (matches the original's static VBO).
// =============================================================================
struct HeightGrid
{
    std::vector<float>* heights = nullptr;  // row-major: heights[z*width + x]
    uint32_t  width = 0, depth = 0;
    float     cellSize = 1.0f;              // world units between samples
    glm::vec2 origin{ 0.0f };               // world XZ of sample (0,0)

    float worldHeight(uint32_t ix, uint32_t iz) const
    {
        return (*heights)[iz * width + ix];
    }
};

// Carve a crater of given radius/depth centred at `impact` (world space).
// Profile: smooth bowl that is deepest at the centre, with an optional rim.
inline std::vector<uint32_t> deformCrater(HeightGrid& g, const glm::vec3& impact,
                                          float radius, float depth,
                                          float rimHeight = 0.0f)
{
    std::vector<uint32_t> touched;
    if (!g.heights || radius <= 0.0f) return touched;

    glm::vec2 local = (glm::vec2(impact.x, impact.z) - g.origin) / g.cellSize;
    int cx = int(std::round(local.x));
    int cz = int(std::round(local.y));
    int cellRad = int(std::ceil(radius / g.cellSize));

    for (int dz = -cellRad; dz <= cellRad; ++dz)
    for (int dx = -cellRad; dx <= cellRad; ++dx)
    {
        int ix = cx + dx, iz = cz + dz;
        if (ix < 0 || iz < 0 || ix >= int(g.width) || iz >= int(g.depth)) continue;

        glm::vec2 wp = g.origin + glm::vec2(ix, iz) * g.cellSize;
        float dist = glm::length(wp - glm::vec2(impact.x, impact.z));
        if (dist > radius) continue;

        float n = dist / radius;                       // 0 centre .. 1 edge
        float bowl = -depth * (1.0f - smoothstep01(n)); // deepest at centre
        // Small raised lip near the rim (0 in the middle, peaks ~0.8r).
        float rim = rimHeight * smoothstep01(glm::clamp((n - 0.6f) / 0.3f, 0.0f, 1.0f))
                              * (1.0f - smoothstep01(glm::clamp((n - 0.9f) / 0.1f, 0.0f, 1.0f)));

        uint32_t idx = uint32_t(iz) * g.width + uint32_t(ix);
        (*g.heights)[idx] += bowl + rim;
        touched.push_back(idx);
    }
    return touched;
}

} // namespace combat
