// ============================================================================
//  Visualizer.cpp
// ============================================================================
#include "Visualizer.h"
#include "UI.h"
#include <cmath>
#include <string>

static const float NODE_W = 86.0f;
static const float NODE_H = 56.0f;
static const float GAP_X  = 54.0f;
static const float ROW_H  = 96.0f;
static const float PAD_L  = 124.0f;   // room for the HEAD marker
static const float PAD_T  = 74.0f;

static float clampf(float v, float lo, float hi)
{
    return (v < lo) ? lo : ((v > hi) ? hi : v);
}

float Visualizer::nodeWidth()  const { return NODE_W; }
float Visualizer::nodeHeight() const { return NODE_H; }

// ---------------------------------------------------------------- geometry
Vector2 Visualizer::slotPos(int index) const
{
    int row = (m_perRow > 0) ? index / m_perRow : 0;
    int col = (m_perRow > 0) ? index % m_perRow : 0;
    return Vector2{ m_startX + col * (NODE_W + GAP_X) + NODE_W * 0.5f,
                    m_startY + row * ROW_H + NODE_H * 0.5f };
}

Vector2 Visualizer::offset() const
{
    return Vector2{ m_shake.x, m_shake.y - m_scroll };
}

Vector2 Visualizer::nodeCenter(const Node* n) const
{
    if (n == nullptr) return Vector2{ 0.0f, 0.0f };
    Vector2 o = offset();
    return Vector2{ n->vis.pos.x + o.x, n->vis.pos.y + o.y };
}

Vector2 Visualizer::headMarkerPos() const
{
    Vector2 o = offset();
    return Vector2{ m_startX - 66.0f + o.x, m_startY + NODE_H * 0.5f + o.y };
}

Vector2 Visualizer::nullMarkerPos(int nodeCount) const
{
    Vector2 p = slotPos(nodeCount);
    Vector2 o = offset();
    return Vector2{ p.x + o.x, p.y + o.y };
}

Vector2 Visualizer::cannonPos() const
{
    return Vector2{ m_arena.x + m_arena.width * 0.5f, m_arena.y + m_arena.height - 34.0f };
}

Vector2 Visualizer::anchorPos(const Anchor& a, const LinkedList& list) const
{
    switch (a.kind)
    {
        case Anchor::HeadMarker: return headMarkerPos();
        case Anchor::NodeRef:    return nodeCenter(a.node);
        case Anchor::NullMarker: return nullMarkerPos(list.count());
        default:                 return Vector2{ 0.0f, 0.0f };
    }
}

// ------------------------------------------------------------------ update
void Visualizer::update(LinkedList& list, float dt)
{
    m_startX = m_arena.x + PAD_L;
    m_startY = m_arena.y + PAD_T;
    m_perRow = (int)((m_arena.width - PAD_L - 70.0f) / (NODE_W + GAP_X));
    if (m_perRow < 1) m_perRow = 1;

    int i = 0;
    for (Node* cur = list.head(); cur != nullptr; cur = cur->next, i++)
    {
        Vector2 t = slotPos(i);
        cur->vis.target = t;

        if (!cur->vis.placed)
        {
            cur->vis.pos    = t;
            cur->vis.placed = true;
        }
        else
        {
            float a = 1.0f - expf(-11.0f * dt);
            cur->vis.pos.x += (t.x - cur->vis.pos.x) * a;
            cur->vis.pos.y += (t.y - cur->vis.pos.y) * a;
        }

        float s = 1.0f - expf(-9.0f * dt);
        cur->vis.scale += (1.0f - cur->vis.scale) * s;
        cur->vis.glow   = clampf(cur->vis.glow   - dt * 1.10f, 0.0f, 1.0f);
        cur->vis.danger = clampf(cur->vis.danger - dt * 0.75f, 0.0f, 1.0f);
        cur->vis.arrow  = clampf(cur->vis.arrow  - dt * 1.40f, 0.0f, 1.0f);
    }
    m_count = i;

    // scrolling when the list outgrows the arena
    int rows = (m_count / m_perRow) + 1;               // +1 keeps NULL visible
    float contentH = rows * ROW_H + 30.0f;
    float viewH    = m_arena.height - PAD_T - 70.0f;
    m_maxScroll    = (contentH > viewH) ? (contentH - viewH) : 0.0f;
    m_scrollTo     = clampf(m_scrollTo, 0.0f, m_maxScroll);
    m_scroll      += (m_scrollTo - m_scroll) * (1.0f - expf(-12.0f * dt));
}

void Visualizer::handleScroll(Vector2 mouse)
{
    if (!CheckCollisionPointRec(mouse, m_arena)) return;
    float w = GetMouseWheelMove();
    if (w != 0.0f) m_scrollTo = clampf(m_scrollTo - w * 48.0f, 0.0f, m_maxScroll);
}

void Visualizer::ensureVisible(Vector2 layoutPos)
{
    float top    = m_arena.y + PAD_T - 40.0f;
    float bottom = m_arena.y + m_arena.height - 86.0f;
    float screenY = layoutPos.y - m_scrollTo;
    if (screenY < top)    m_scrollTo = clampf(m_scrollTo - (top - screenY),    0.0f, m_maxScroll);
    if (screenY > bottom) m_scrollTo = clampf(m_scrollTo + (screenY - bottom), 0.0f, m_maxScroll);
}

// ----------------------------------------------------------------- drawing
void Visualizer::drawLinkArrow(Vector2 a, Vector2 b, bool wraps, Color c,
                               float flash, float time) const
{
    Color col = (flash > 0.01f)
              ? Color{ (unsigned char)(c.r + (Theme::AMBER.r - c.r) * flash),
                       (unsigned char)(c.g + (Theme::AMBER.g - c.g) * flash),
                       (unsigned char)(c.b + (Theme::AMBER.b - c.b) * flash), 255 }
              : c;
    float thick = 2.4f + 2.6f * flash;

    Vector2 pts[6];
    int n = 0;
    if (!wraps)
    {
        pts[n++] = a;
        pts[n++] = b;
    }
    else
    {
        float rightX = m_arena.x + m_arena.width - 26.0f;
        float leftX  = m_startX - 46.0f;
        float midY   = a.y + ROW_H * 0.5f;
        pts[n++] = a;
        pts[n++] = Vector2{ rightX, a.y };
        pts[n++] = Vector2{ rightX, midY };
        pts[n++] = Vector2{ leftX,  midY };
        pts[n++] = Vector2{ leftX,  b.y };
        pts[n++] = b;
    }

    for (int i = 0; i < n - 1; i++) DrawLineEx(pts[i], pts[i + 1], thick, col);

    // arrow head at the end
    Vector2 p0 = pts[n - 2], p1 = pts[n - 1];
    float ang = atan2f(p1.y - p0.y, p1.x - p0.x);
    float hs  = 9.0f + 3.0f * flash;
    Vector2 t1 = { p1.x, p1.y };
    Vector2 t2 = { p1.x - cosf(ang - 0.45f) * hs * 1.6f, p1.y - sinf(ang - 0.45f) * hs * 1.6f };
    Vector2 t3 = { p1.x - cosf(ang + 0.45f) * hs * 1.6f, p1.y - sinf(ang + 0.45f) * hs * 1.6f };
    DrawTriangle(t1, t3, t2, col);

    // travelling spark while the pointer is highlighted
    if (flash > 0.01f)
    {
        float k = fmodf(time * 1.6f, 1.0f);
        // walk the polyline by length fraction
        float total = 0.0f;
        for (int i = 0; i < n - 1; i++)
            total += sqrtf((pts[i+1].x-pts[i].x)*(pts[i+1].x-pts[i].x) +
                           (pts[i+1].y-pts[i].y)*(pts[i+1].y-pts[i].y));
        float want = total * k, acc = 0.0f;
        Vector2 sp = pts[0];
        for (int i = 0; i < n - 1; i++)
        {
            float seg = sqrtf((pts[i+1].x-pts[i].x)*(pts[i+1].x-pts[i].x) +
                              (pts[i+1].y-pts[i].y)*(pts[i+1].y-pts[i].y));
            if (acc + seg >= want && seg > 0.0f)
            {
                float u = (want - acc) / seg;
                sp = Vec2Lerp(pts[i], pts[i+1], u);
                break;
            }
            acc += seg;
        }
        DrawCircleV(sp, 7.0f * flash, Fade(Theme::AMBER, 0.35f * flash));
        DrawCircleV(sp, 3.5f * flash, Fade(WHITE, flash));
    }
}

void Visualizer::drawNode(const Node* n, int index, float time) const
{
    Vector2 c = nodeCenter(n);
    float s   = n->vis.scale;
    float w   = NODE_W * s, h = NODE_H * s;
    Rectangle box = { c.x - w * 0.5f, c.y - h * 0.5f, w, h };

    Color border = Theme::CYAN;
    if (n->vis.glow   > 0.01f) border = Theme::MINT;
    if (n->vis.danger > 0.01f) border = Theme::DANGER;

    float pulse = 0.0f;
    if (n->vis.danger > 0.01f) pulse = (sinf(time * 18.0f) * 0.5f + 0.5f) * n->vis.danger;

    // shadow + body
    DrawRectangleRounded(Rectangle{ box.x, box.y + 4.0f, box.width, box.height },
                         0.28f, 8, Fade(BLACK, 0.45f));
    DrawRectangleRounded(box, 0.28f, 8, Theme::PANEL_HI);

    // lit-bulb wash when visited
    if (n->vis.glow > 0.01f)
        DrawRectangleRounded(box, 0.28f, 8, Fade(Theme::MINT, 0.22f * n->vis.glow));
    if (n->vis.danger > 0.01f)
        DrawRectangleRounded(box, 0.28f, 8, Fade(Theme::DANGER, 0.25f * n->vis.danger));

    UIGlowRect(box, 0.28f, border, (n->vis.glow + n->vis.danger + pulse) * 1.2f);
    UIRoundedLines(box, 0.28f, 8, 2.2f + 1.6f * (n->vis.glow + pulse), border);

    // divider: [ data | next ]
    float splitX = box.x + box.width * 0.62f;
    DrawLineEx(Vector2{ splitX, box.y + 4.0f }, Vector2{ splitX, box.y + box.height - 4.0f },
               1.6f, Fade(border, 0.55f));

    // value
    std::string v = std::to_string(n->data);
    int fs = 24;
    int tw = MeasureText(v.c_str(), fs);
    while (tw > box.width * 0.56f && fs > 12) { fs -= 2; tw = MeasureText(v.c_str(), fs); }
    DrawText(v.c_str(), (int)(box.x + (box.width * 0.62f - tw) * 0.5f),
             (int)(c.y - fs * 0.5f), fs, Theme::TEXT);

    // the 'next' cell: a dot when it points somewhere, a slash when nullptr
    float nx = (splitX + box.x + box.width) * 0.5f;
    DrawText("next", (int)(nx - MeasureText("next", 9) * 0.5f),
             (int)(box.y + 7.0f), 9, Fade(Theme::MUTED, 0.95f));
    if (n->next != nullptr)
    {
        DrawCircleV(Vector2{ nx, c.y + 7.0f }, 5.0f, border);
        DrawCircleV(Vector2{ nx, c.y + 7.0f }, 2.0f, Theme::PANEL);
    }
    else
    {
        DrawText("0", (int)(nx - MeasureText("0", 14) * 0.5f), (int)(c.y - 1.0f), 14,
                 Fade(Theme::MUTED, 0.95f));
        DrawLineEx(Vector2{ nx - 7.0f, c.y + 10.0f }, Vector2{ nx + 7.0f, c.y - 4.0f },
                   1.6f, Fade(Theme::MUTED, 0.95f));
    }

    // position label above the box
    std::string tag = "#" + std::to_string(index + 1);
    DrawText(tag.c_str(), (int)(box.x + 2.0f), (int)(box.y - 15.0f), 12,
             Fade(Theme::MUTED, 0.95f));
}

void Visualizer::drawArena(const LinkedList& list, float time)
{
    BeginScissorMode((int)m_arena.x, (int)m_arena.y + 34,
                     (int)m_arena.width, (int)m_arena.height - 34);

    // subtle grid
    for (float x = m_arena.x + 20.0f; x < m_arena.x + m_arena.width; x += 40.0f)
        DrawLineEx(Vector2{ x, m_arena.y + 36.0f }, Vector2{ x, m_arena.y + m_arena.height - 2.0f },
                   1.0f, Fade(Theme::BG_GRID, 0.55f));
    for (float y = m_arena.y + 40.0f; y < m_arena.y + m_arena.height; y += 40.0f)
        DrawLineEx(Vector2{ m_arena.x + 2.0f, y }, Vector2{ m_arena.x + m_arena.width - 2.0f, y },
                   1.0f, Fade(Theme::BG_GRID, 0.55f));

    const int   n    = list.count();
    const float half = NODE_W * 0.5f;

    // ---- arrows first, so the boxes sit on top ----------------------------
    {
        int i = 0;
        for (Node* cur = list.head(); cur != nullptr; cur = cur->next, i++)
        {
            Vector2 c = nodeCenter(cur);
            Vector2 a = { c.x + half, c.y };
            Vector2 b;
            bool wraps = ((i + 1) % m_perRow == 0);
            if (cur->next != nullptr) { Vector2 d = nodeCenter(cur->next); b = Vector2{ d.x - half - 11.0f, d.y }; }
            else                      { Vector2 d = nullMarkerPos(n);      b = Vector2{ d.x - 30.0f, d.y }; }
            drawLinkArrow(a, b, wraps, Fade(Theme::CYAN, 0.75f), cur->vis.arrow, time);
        }
    }

    // HEAD -> first node (or straight to NULL when empty)
    {
        Vector2 hp = headMarkerPos();
        Vector2 a  = { hp.x + 34.0f, hp.y };
        Vector2 b;
        if (list.head() != nullptr) { Vector2 d = nodeCenter(list.head()); b = Vector2{ d.x - half - 11.0f, d.y }; }
        else                        { Vector2 d = nullMarkerPos(0);        b = Vector2{ d.x - 30.0f, d.y }; }
        drawLinkArrow(a, b, false, Fade(Theme::HOT, 0.9f), 0.0f, time);
    }

    // ---- nodes -------------------------------------------------------------
    {
        int i = 0;
        for (Node* cur = list.head(); cur != nullptr; cur = cur->next, i++)
            drawNode(cur, i, time);
    }

    // ---- HEAD marker -------------------------------------------------------
    {
        Vector2 hp = headMarkerPos();
        Rectangle r = { hp.x - 34.0f, hp.y - 19.0f, 68.0f, 38.0f };
        DrawRectangleRounded(r, 0.4f, 8, Fade(Theme::HOT, 0.16f));
        UIRoundedLines(r, 0.4f, 8, 2.0f, Theme::HOT);
        UITextCentered("HEAD", hp.x, hp.y, 16, Theme::HOT);
    }

    // ---- NULL marker -------------------------------------------------------
    {
        Vector2 np = nullMarkerPos(n);
        Rectangle r = { np.x - 30.0f, np.y - 18.0f, 62.0f, 36.0f };
        DrawRectangleRounded(r, 0.4f, 8, Fade(Theme::LILAC, 0.12f));
        UIRoundedLines(r, 0.4f, 8, 1.8f, Fade(Theme::LILAC, 0.9f));
        UITextCentered("NULL", np.x + 1.0f, np.y, 15, Theme::LILAC);
    }

    if (n == 0)
    {
        DrawText("The list is empty:  head = nullptr",
                 (int)(m_arena.x + 40.0f), (int)(m_arena.y + m_arena.height - 108.0f),
                 15, Fade(Theme::MUTED, 0.95f));
    }

    EndScissorMode();

    // scrollbar hint
    if (m_maxScroll > 1.0f)
    {
        float trackH = m_arena.height - 80.0f;
        float barH   = trackH * (trackH / (trackH + m_maxScroll));
        float t      = m_scroll / m_maxScroll;
        float x      = m_arena.x + m_arena.width - 8.0f;
        DrawRectangleRounded(Rectangle{ x, m_arena.y + 44.0f, 4.0f, trackH }, 1.0f, 4,
                             Fade(Theme::EDGE, 0.5f));
        DrawRectangleRounded(Rectangle{ x, m_arena.y + 44.0f + t * (trackH - barH), 4.0f, barH },
                             1.0f, 4, Fade(Theme::CYAN, 0.8f));
    }
}

void Visualizer::drawCannon(float time, float recoil, float aimX)
{
    Vector2 base = cannonPos();
    Vector2 muzzleTarget = { aimX, base.y - 120.0f };
    float ang = atan2f(muzzleTarget.y - base.y, muzzleTarget.x - base.x);

    float len = 52.0f - recoil * 12.0f;
    Vector2 tip = { base.x + cosf(ang) * len, base.y + sinf(ang) * len };

    DrawLineEx(base, tip, 20.0f, Color{ 60, 70, 120, 255 });
    DrawLineEx(base, tip, 13.0f, Color{ 96, 112, 180, 255 });
    DrawCircleV(tip, 9.0f, Fade(Theme::CYAN, 0.85f));

    DrawCircleV(base, 22.0f, Color{ 42, 50, 88, 255 });
    DrawCircleV(base, 22.0f, Fade(Theme::CYAN, 0.0f));
    DrawRing(base, 20.0f, 23.0f, 0.0f, 360.0f, 32, Fade(Theme::CYAN, 0.75f));
    DrawCircleV(base, 7.0f + sinf(time * 4.0f) * 1.2f, Fade(Theme::CYAN, 0.55f));

    if (recoil > 0.01f)
        DrawCircleV(tip, 16.0f * recoil, Fade(Theme::AMBER, 0.5f * recoil));
}

void Visualizer::drawCursor(Vector2 p, float time, Color c, const char* label) const
{
    float bob = sinf(time * 6.0f) * 3.0f;
    Vector2 at = { p.x, p.y - NODE_H * 0.5f - 30.0f + bob };

    DrawCircleV(at, 17.0f, Fade(c, 0.18f));
    DrawCircleV(at, 12.0f, c);
    DrawCircleV(Vector2{ at.x - 4.0f, at.y - 2.0f }, 2.6f, Theme::BG);
    DrawCircleV(Vector2{ at.x + 4.0f, at.y - 2.0f }, 2.6f, Theme::BG);
    // little pointer triangle aiming at the node below
    DrawTriangle(Vector2{ at.x - 6.0f, at.y + 9.0f },
                 Vector2{ at.x + 6.0f, at.y + 9.0f },
                 Vector2{ at.x,        at.y + 20.0f }, c);

    int w = MeasureText(label, 11);
    Rectangle r = { at.x - w * 0.5f - 6.0f, at.y - 34.0f, (float)w + 12.0f, 17.0f };
    DrawRectangleRounded(r, 0.5f, 6, Fade(Theme::BG, 0.85f));
    UIRoundedLines(r, 0.5f, 6, 1.2f, Fade(c, 0.9f));
    DrawText(label, (int)(at.x - w * 0.5f), (int)(at.y - 31.0f), 11, c);
}

// ------------------------------------------------------------- step overlay
void Visualizer::drawOverlay(const LinkedList& list, Operation& op,
                             ParticleSystem& fx, float dt, float time)
{
    (void)dt;
    const Step* s = op.current();
    if (s == nullptr) return;

    BeginScissorMode((int)m_arena.x, (int)m_arena.y + 34,
                     (int)m_arena.width, (int)m_arena.height - 34);

    float t = op.stepT();

    switch (s->kind)
    {
        case StepKind::Walk:
        {
            Vector2 a = anchorPos(s->from, list);
            Vector2 b = anchorPos(s->to,   list);
            if (s->from.kind == Anchor::None) a = b;
            Vector2 p = Vec2Lerp(a, b, Ease::InOutQuad(t));
            if (s->to.kind == Anchor::NodeRef && s->to.node != nullptr)
                ensureVisible(s->to.node->vis.target);
            drawCursor(p, time, Theme::AMBER, "current");
            if (GetRandomValue(0, 2) == 0) fx.trail(p, Fade(Theme::AMBER, 0.8f));
            break;
        }
        case StepKind::Inspect:
        {
            Vector2 b = anchorPos(s->to, list);
            if (s->to.kind == Anchor::NodeRef && s->to.node != nullptr)
            {
                ensureVisible(s->to.node->vis.target);
                float r = NODE_W * (0.55f + 0.25f * (1.0f - t));
                DrawRing(b, r, r + 3.0f, 0.0f, 360.0f, 48,
                         Fade(Theme::MINT, 0.55f * (1.0f - t)));
            }
            drawCursor(b, time, Theme::AMBER, "current");
            break;
        }
        case StepKind::Shoot:
        {
            Node* pend = op.pending();
            Vector2 from = cannonPos();
            Vector2 land = slotPos(s->slot);
            Vector2 o    = offset();
            land = Vector2{ land.x, land.y + o.y - 76.0f };
            Vector2 ctrl = { (from.x + land.x) * 0.5f, land.y - 110.0f };
            Vector2 p    = Vec2Bezier(from, ctrl, land, Ease::OutCubic(t));

            op.setProjectilePos(Vector2{ p.x - m_shake.x, p.y - o.y });   // layout space
            fx.trail(p, Theme::AMBER);

            if (pend != nullptr)
            {
                float sc = 0.55f + 0.45f * t;
                float w = NODE_W * sc, h = NODE_H * sc;
                Rectangle box = { p.x - w * 0.5f, p.y - h * 0.5f, w, h };
                DrawCircleV(p, w * 0.75f, Fade(Theme::AMBER, 0.22f));
                DrawRectangleRounded(box, 0.3f, 8, Theme::PANEL_HI);
                UIRoundedLines(box, 0.3f, 8, 2.4f, Theme::AMBER);
                std::string v = std::to_string(pend->data);
                UITextCentered(v.c_str(), p.x, p.y, 22, Theme::AMBER);
            }
            // landing marker
            DrawRing(land, 20.0f + 10.0f * sinf(time * 8.0f), 23.0f + 10.0f * sinf(time * 8.0f),
                     0.0f, 360.0f, 32, Fade(Theme::AMBER, 0.45f));
            break;
        }
        case StepKind::Arm:
        {
            Vector2 b = anchorPos(s->to, list);
            float blink = (sinf(time * 16.0f) * 0.5f + 0.5f);
            DrawRing(b, 46.0f - 10.0f * t, 50.0f - 10.0f * t, 0.0f, 360.0f, 48,
                     Fade(Theme::DANGER, 0.35f + 0.5f * blink));
            drawCursor(b, time, Theme::DANGER, "delete me");
            break;
        }
        case StepKind::Explode:
        {
            Vector2 b = anchorPos(s->to, list);
            float k = 1.0f - t;
            DrawCircleV(b, 10.0f + 70.0f * t, Fade(Theme::AMBER, 0.35f * k));
            DrawCircleV(b, 6.0f + 40.0f * t, Fade(WHITE, 0.5f * k));
            break;
        }
        case StepKind::Commit:
        case StepKind::Relink:
        {
            Vector2 b = anchorPos(s->to, list);
            if (s->to.kind != Anchor::None)
            {
                DrawRing(b, 34.0f, 37.0f, 0.0f, 360.0f * t, 48, Fade(Theme::MINT, 0.75f));
                drawCursor(b, time, Theme::MINT, (s->kind == StepKind::Commit) ? "rewiring" : "linked");
            }
            break;
        }
        case StepKind::Success:
        {
            Vector2 b = anchorPos(s->to, list);
            if (s->to.kind == Anchor::NodeRef && s->to.node != nullptr)
                DrawRing(b, 30.0f + 40.0f * t, 33.0f + 40.0f * t, 0.0f, 360.0f, 48,
                         Fade(Theme::MINT, 0.6f * (1.0f - t)));
            break;
        }
        case StepKind::Fail:
        {
            Vector2 b = anchorPos(s->to, list);
            if (s->to.kind != Anchor::None)
            {
                float blink = (sinf(time * 9.0f) * 0.5f + 0.5f);
                DrawRing(b, 28.0f, 31.0f, 0.0f, 360.0f, 48,
                         Fade(Theme::DANGER, 0.4f + 0.4f * blink));
                drawCursor(b, time, Theme::DANGER, "stuck here");
            }
            break;
        }
        case StepKind::Message:
        default:
        {
            if (s->to.kind != Anchor::None)
                drawCursor(anchorPos(s->to, list), time, Theme::AMBER, "current");
            break;
        }
    }

    EndScissorMode();
}
