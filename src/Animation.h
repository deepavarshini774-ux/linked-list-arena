// ============================================================================
//  Animation.h  -  frame-based effects: particles, screen shake, easing.
//
//  Nothing here blocks: everything is advanced by dt (GetFrameTime()).
//  There is no Sleep() anywhere in this project.
// ============================================================================
#ifndef ANIMATION_H
#define ANIMATION_H

#include "raylib.h"
#include <vector>

// ------------------------------------------------------------- easing maths
namespace Ease
{
    float Linear  (float t);
    float OutCubic(float t);
    float InCubic (float t);
    float InOutQuad(float t);
    float OutBack (float t);   // slight overshoot, feels "poppy"
    float OutElastic(float t);
}

Vector2 Vec2Lerp (Vector2 a, Vector2 b, float t);
Vector2 Vec2Add  (Vector2 a, Vector2 b);
// Quadratic bezier: used for the cannon projectile arc.
Vector2 Vec2Bezier(Vector2 a, Vector2 control, Vector2 b, float t);

// ---------------------------------------------------------------- particles
struct Particle
{
    Vector2 pos      = { 0.0f, 0.0f };
    Vector2 vel      = { 0.0f, 0.0f };
    float   life     = 0.0f;
    float   maxLife  = 1.0f;
    float   size     = 4.0f;
    float   gravity  = 0.0f;
    float   drag     = 1.0f;
    float   spin     = 0.0f;
    float   angle    = 0.0f;
    bool    square   = false;
    Color   color    = WHITE;
};

class ParticleSystem
{
public:
    void update(float dt);
    void draw(Vector2 offset) const;     // offset = screen-shake / scroll
    void clear();
    int  count() const { return (int)m_parts.size(); }

    void explosion(Vector2 at, Color hot, Color cool, int amount = 70);
    void smoke    (Vector2 at, int amount = 18);
    void impact   (Vector2 at, Color c, int amount = 26);
    void sparkle  (Vector2 at, Color c, int amount = 8);
    void confetti (Vector2 at, int amount = 40);
    void trail    (Vector2 at, Color c);

private:
    std::vector<Particle> m_parts;
};

// -------------------------------------------------------------- screenshake
class ScreenShake
{
public:
    void  add(float magnitude);          // call once, e.g. on an explosion
    void  update(float dt);
    Vector2 offset() const { return m_offset; }

private:
    float   m_mag    = 0.0f;
    Vector2 m_offset = { 0.0f, 0.0f };
};

#endif // ANIMATION_H
