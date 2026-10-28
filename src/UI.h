// ============================================================================
//  UI.h  -  arcade-flavoured widgets: buttons, text fields, panels, text wrap.
//           Pure presentation; knows nothing about linked lists.
// ============================================================================
#ifndef UI_H
#define UI_H

#include "raylib.h"
#include <string>

// ------------------------------------------------------------------- colours
namespace Theme
{
    const Color BG        = Color{  10,  12,  26, 255 };
    const Color BG_GRID   = Color{  22,  26,  52, 255 };
    const Color PANEL     = Color{  20,  24,  46, 255 };
    const Color PANEL_HI  = Color{  32,  38,  70, 255 };
    const Color EDGE      = Color{  52,  62, 110, 255 };
    const Color TEXT      = Color{ 228, 234, 255, 255 };
    const Color MUTED     = Color{ 132, 146, 190, 255 };
    const Color CYAN      = Color{   0, 229, 255, 255 };
    const Color HOT       = Color{ 255,  50, 170, 255 };
    const Color MINT      = Color{   0, 255, 153, 255 };
    const Color AMBER     = Color{ 255, 206,  40, 255 };
    const Color DANGER    = Color{ 255,  70,  70, 255 };
    const Color LILAC     = Color{ 160, 120, 255, 255 };
    const Color TANGERINE = Color{ 255, 140,  40, 255 };
}

// -------------------------------------------------------------------- button
struct Button
{
    Rectangle   rect    = { 0, 0, 0, 0 };
    std::string label;
    std::string hotkey;             // small grey text in the corner
    Color       accent  = Theme::CYAN;
    bool        enabled = true;
    bool        toggled = false;    // draws as "lit"
    float       hover   = 0.0f;     // animated 0..1
    float       press   = 0.0f;     // animated 0..1
};

// Input handling ONLY (hit test + hover/press animation). Returns true on a
// left click inside the button. Must be called BEFORE BeginDrawing().
bool UIButtonUpdate(Button& b, Vector2 mouse, float dt);
// Rendering ONLY. Must be called between BeginDrawing() and EndDrawing().
void UIButtonDraw(const Button& b);

// ---------------------------------------------------------------- text field
struct TextField
{
    Rectangle   rect    = { 0, 0, 0, 0 };
    std::string label;
    std::string text;
    bool        focused = false;
    int         maxLen  = 5;
    bool        allowNegative = true;
    float       shake   = 0.0f;     // nudge when the user forgot to fill it in

    bool  hasValue() const { return !text.empty() && text != "-"; }
    int   value(int fallback = 0) const;
    void  clear() { text.clear(); }
};

void UITextFieldUpdate(TextField& f, Vector2 mouse, float dt);  // input handling
void UITextFieldDraw  (const TextField& f, float time);

// ------------------------------------------------------------------- helpers
void  UIPanel(Rectangle r, const char* title, Color accent);
void  UIBadge(Rectangle r, const char* text, Color accent);
// Word-wrapped text (honours '\n'). Returns the height actually used.
float UITextWrapped(const std::string& text, Rectangle box, int fontSize,
                    int lineSpacing, Color color);
void  UITextCentered(const char* text, float cx, float cy, int fontSize, Color color);
// Rounded outline that works on both raylib 5.0 and 5.5+ (the signature of
// DrawRectangleRoundedLines changed between those versions).
void  UIRoundedLines(Rectangle r, float roundness, int segments, float thick, Color c);
void  UIGlowRect(Rectangle r, float roundness, Color c, float strength);

#endif // UI_H
