// ============================================================================
//  Visualizer.h  -  layout + rendering of the list, arrows, markers, cannon
//                   and the per-step animation overlay.
//
//  This module READS the real list and draws what it finds. It never inserts
//  or deletes anything. The only thing it writes is the cosmetic NodeVisual
//  data (screen position, glow) stored inside each node.
// ============================================================================
#ifndef VISUALIZER_H
#define VISUALIZER_H

#include "raylib.h"
#include "LinkedList.h"
#include "Operation.h"
#include "Animation.h"

class Visualizer
{
public:
    void setArena(Rectangle r) { m_arena = r; }
    Rectangle arena() const    { return m_arena; }

    // Advance layout easing + glow decay. Call once per frame.
    void update(LinkedList& list, float dt);
    void setShake(Vector2 s)   { m_shake = s; }
    void handleScroll(Vector2 mouse);
    void ensureVisible(Vector2 layoutPos);

    // ---- geometry (layout space; add offset() for screen space) -------------
    Vector2 slotPos(int index) const;              // centre of 0-based slot
    Vector2 offset() const;                        // shake + scroll
    Vector2 nodeCenter(const Node* n) const;       // screen space
    Vector2 headMarkerPos() const;                 // screen space
    Vector2 nullMarkerPos(int nodeCount) const;    // screen space
    Vector2 cannonPos() const;                     // screen space
    Vector2 anchorPos(const Anchor& a, const LinkedList& list) const;
    float   nodeWidth()  const;
    float   nodeHeight() const;

    // ---- drawing -----------------------------------------------------------
    void drawArena(const LinkedList& list, float time);
    void drawCannon(float time, float recoil, float aimX);
    // Step-specific animation; may emit particles and reports the projectile
    // position back to the operation so Commit can drop the node there.
    void drawOverlay(const LinkedList& list, Operation& op,
                     ParticleSystem& fx, float dt, float time);

private:
    void  drawNode(const Node* n, int index, float time) const;
    void  drawLinkArrow(Vector2 a, Vector2 b, bool wraps, Color c,
                        float flash, float time) const;
    void  drawCursor(Vector2 p, float time, Color c, const char* label) const;

    Rectangle m_arena      = { 0, 0, 100, 100 };
    Vector2   m_shake      = { 0.0f, 0.0f };
    float     m_scroll     = 0.0f;
    float     m_scrollTo   = 0.0f;
    float     m_maxScroll  = 0.0f;
    int       m_perRow     = 6;
    int       m_count      = 0;
    float     m_startX     = 0.0f;
    float     m_startY     = 0.0f;
};

#endif // VISUALIZER_H
