// ============================================================================
//  Animation.cpp
// ============================================================================
#include "Animation.h"
#include <cmath>

// ------------------------------------------------------------- easing maths
namespace Ease
{
    static float clamp01(float t) { return (t < 0.0f) ? 0.0f : (t > 1.0f ? 1.0f : t); }

    float Linear(float t)   { return clamp01(t); }
    float OutCubic(float t) { t = clamp01(t); float u = 1.0f - t; return 1.0f - u * u * u; }
    float InCubic(float t)  { t = clamp01(t); return t * t * t; }

    float InOutQuad(float t)
    {
        t = clamp01(t);
        return (t < 0.5f) ? (2.0f * t * t) : (1.0f - 2.0f * (1.0f - t) * (1.0f - t));
    }

    float OutBack(float t)
    {
        t = clamp01(t);
        const float c1 = 1.70158f, c3 = c1 + 1.0f;
        float u = t - 1.0f;
        return 1.0f + c3 * u * u * u + c1 * u * u;
    }

    float OutElastic(float t)
    {
        t = clamp01(t);
        if (t <= 0.0f) return 0.0f;
        if (t >= 1.0f) return 1.0f;
        const float c4 = (2.0f * PI) / 3.0f;
        return powf(2.0f, -10.0f * t) * sinf((t * 10.0f - 0.75f) * c4) + 1.0f;
    }
}

Vector2 Vec2Lerp(Vector2 a, Vector2 b, float t)
{
    return Vector2{ a.x + (b.x - a.x) * t, a.y + (b.y - a.y) * t };
}

Vector2 Vec2Add(Vector2 a, Vector2 b)
{
    return Vector2{ a.x + b.x, a.y + b.y };
}

Vector2 Vec2Bezier(Vector2 a, Vector2 c, Vector2 b, float t)
{
    float u = 1.0f - t;
    return Vector2{ u * u * a.x + 2.0f * u * t * c.x + t * t * b.x,
                    u * u * a.y + 2.0f * u * t * c.y + t * t * b.y };
}

// ---------------------------------------------------------------- particles
static float randRange(float lo, float hi)
{
    return lo + (hi - lo) * ((float)GetRandomValue(0, 10000) / 10000.0f);
}

void ParticleSystem::update(float dt)
{
    for (size_t i = 0; i < m_parts.size(); )
    {
        Particle& p = m_parts[i];
        p.life -= dt;
        if (p.life <= 0.0f)
        {
            m_parts[i] = m_parts.back();   // cheap swap-and-pop removal
            m_parts.pop_back();
            continue;
        }
        p.vel.y += p.gravity * dt;
        p.vel.x *= powf(p.drag, dt * 60.0f);
        p.vel.y *= powf(p.drag, dt * 60.0f);
        p.pos.x += p.vel.x * dt;
        p.pos.y += p.vel.y * dt;
        p.angle += p.spin * dt;
        i++;
    }
}

void ParticleSystem::draw(Vector2 offset) const
{
    for (size_t i = 0; i < m_parts.size(); i++)
    {
        const Particle& p = m_parts[i];
        float k = p.life / p.maxLife;              // 1 -> 0 over the lifetime
        if (k < 0.0f) k = 0.0f;
        Color c = Fade(p.color, k);
        Vector2 at = { p.pos.x + offset.x, p.pos.y + offset.y };

        if (p.square)
        {
            Rectangle r = { at.x, at.y, p.size * k + 1.0f, p.size * k + 1.0f };
            Vector2 org = { r.width * 0.5f, r.height * 0.5f };
            DrawRectanglePro(r, org, p.angle, c);
        }
        else
        {
            DrawCircleV(at, p.size * k + 0.5f, c);
        }
    }
}

void ParticleSystem::clear() { m_parts.clear(); }

void ParticleSystem::explosion(Vector2 at, Color hot, Color cool, int amount)
{
    for (int i = 0; i < amount; i++)
    {
        Particle p;
        float ang   = randRange(0.0f, 2.0f * PI);
        float speed = randRange(60.0f, 420.0f);
        p.pos     = at;
        p.vel     = Vector2{ cosf(ang) * speed, sinf(ang) * speed };
        p.maxLife = randRange(0.35f, 1.1f);
        p.life    = p.maxLife;
        p.size    = randRange(3.0f, 9.0f);
        p.gravity = randRange(120.0f, 420.0f);
        p.drag    = 0.96f;
        p.square  = (GetRandomValue(0, 1) == 1);
        p.spin    = randRange(-540.0f, 540.0f);
        p.color   = (GetRandomValue(0, 100) < 55) ? hot : cool;
        m_parts.push_back(p);
    }
}

void ParticleSystem::smoke(Vector2 at, int amount)
{
    for (int i = 0; i < amount; i++)
    {
        Particle p;
        p.pos     = Vector2{ at.x + randRange(-18.0f, 18.0f), at.y + randRange(-12.0f, 12.0f) };
        p.vel     = Vector2{ randRange(-40.0f, 40.0f), randRange(-110.0f, -35.0f) };
        p.maxLife = randRange(0.8f, 1.8f);
        p.life    = p.maxLife;
        p.size    = randRange(10.0f, 22.0f);
        p.gravity = -12.0f;
        p.drag    = 0.97f;
        unsigned char g = (unsigned char)GetRandomValue(90, 150);
        p.color   = Color{ g, g, (unsigned char)(g + 20), 180 };
        m_parts.push_back(p);
    }
}

void ParticleSystem::impact(Vector2 at, Color c, int amount)
{
    for (int i = 0; i < amount; i++)
    {
        Particle p;
        float ang   = randRange(0.0f, 2.0f * PI);
        float speed = randRange(40.0f, 230.0f);
        p.pos     = at;
        p.vel     = Vector2{ cosf(ang) * speed, sinf(ang) * speed };
        p.maxLife = randRange(0.25f, 0.6f);
        p.life    = p.maxLife;
        p.size    = randRange(2.0f, 6.0f);
        p.drag    = 0.92f;
        p.color   = c;
        m_parts.push_back(p);
    }
}

void ParticleSystem::sparkle(Vector2 at, Color c, int amount)
{
    for (int i = 0; i < amount; i++)
    {
        Particle p;
        p.pos     = Vector2{ at.x + randRange(-26.0f, 26.0f), at.y + randRange(-22.0f, 22.0f) };
        p.vel     = Vector2{ randRange(-30.0f, 30.0f), randRange(-70.0f, -20.0f) };
        p.maxLife = randRange(0.3f, 0.8f);
        p.life    = p.maxLife;
        p.size    = randRange(2.0f, 5.0f);
        p.drag    = 0.95f;
        p.color   = c;
        m_parts.push_back(p);
    }
}

void ParticleSystem::confetti(Vector2 at, int amount)
{
    const Color palette[5] = { Color{ 0, 229, 255, 255 }, Color{ 255, 0, 153, 255 },
                               Color{ 0, 255, 153, 255 }, Color{ 255, 214, 0, 255 },
                               Color{ 170, 120, 255, 255 } };
    for (int i = 0; i < amount; i++)
    {
        Particle p;
        float ang   = randRange(-PI, 0.0f);
        float speed = randRange(120.0f, 380.0f);
        p.pos     = at;
        p.vel     = Vector2{ cosf(ang) * speed, sinf(ang) * speed };
        p.maxLife = randRange(0.7f, 1.6f);
        p.life    = p.maxLife;
        p.size    = randRange(4.0f, 9.0f);
        p.gravity = 380.0f;
        p.drag    = 0.99f;
        p.square  = true;
        p.spin    = randRange(-720.0f, 720.0f);
        p.color   = palette[GetRandomValue(0, 4)];
        m_parts.push_back(p);
    }
}

void ParticleSystem::trail(Vector2 at, Color c)
{
    Particle p;
    p.pos     = Vector2{ at.x + randRange(-4.0f, 4.0f), at.y + randRange(-4.0f, 4.0f) };
    p.vel     = Vector2{ randRange(-25.0f, 25.0f), randRange(-25.0f, 25.0f) };
    p.maxLife = 0.35f;
    p.life    = p.maxLife;
    p.size    = randRange(3.0f, 6.0f);
    p.drag    = 0.9f;
    p.color   = c;
    m_parts.push_back(p);
}

// -------------------------------------------------------------- screenshake
void ScreenShake::add(float magnitude)
{
    if (magnitude > m_mag) m_mag = magnitude;
}

void ScreenShake::update(float dt)
{
    if (m_mag > 0.0f)
    {
        m_mag -= 34.0f * dt;                     // decay
        if (m_mag < 0.0f) m_mag = 0.0f;
        m_offset.x = randRange(-m_mag, m_mag);
        m_offset.y = randRange(-m_mag, m_mag);
    }
    else
    {
        m_offset = Vector2{ 0.0f, 0.0f };
    }
}
