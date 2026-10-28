// ============================================================================
//  UI.cpp
// ============================================================================
#include "UI.h"
#include <cmath>
#include <cstdlib>
#include <vector>

// raylib changed this signature between 5.0 and 5.5, so wrap it once.
void UIRoundedLines(Rectangle r, float roundness, int segments, float thick, Color c)
{
#if defined(RAYLIB_VERSION_MAJOR) && ((RAYLIB_VERSION_MAJOR > 5) || \
    (RAYLIB_VERSION_MAJOR == 5 && RAYLIB_VERSION_MINOR >= 5))
    DrawRectangleRoundedLinesEx(r, roundness, segments, thick, c);
#else
    DrawRectangleRoundedLines(r, roundness, segments, thick, c);
#endif
}

void UIGlowRect(Rectangle r, float roundness, Color c, float strength)
{
    if (strength <= 0.0f) return;
    for (int i = 3; i >= 1; i--)
    {
        Rectangle g = { r.x - i * 3.0f, r.y - i * 3.0f,
                        r.width + i * 6.0f, r.height + i * 6.0f };
        UIRoundedLines(g, roundness, 8, 2.0f, Fade(c, 0.12f * strength / (float)i));
    }
}

void UITextCentered(const char* text, float cx, float cy, int fontSize, Color color)
{
    int w = MeasureText(text, fontSize);
    DrawText(text, (int)(cx - w * 0.5f), (int)(cy - fontSize * 0.5f), fontSize, color);
}

// -------------------------------------------------------------------- button
bool UIButtonUpdate(Button& b, Vector2 mouse, float dt)
{
    bool inside  = CheckCollisionPointRec(mouse, b.rect);
    bool clicked = false;

    float hoverTarget = (inside && b.enabled) ? 1.0f : 0.0f;
    b.hover += (hoverTarget - b.hover) * (1.0f - expf(-14.0f * dt));

    if (inside && b.enabled && IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        clicked = true;
        b.press = 1.0f;
    }
    b.press -= dt * 4.0f;
    if (b.press < 0.0f) b.press = 0.0f;

    return clicked;
}

void UIButtonDraw(const Button& b)
{
    Rectangle r = b.rect;
    r.y += b.press * 2.0f;

    Color base = b.enabled ? Theme::PANEL_HI : Color{ 24, 26, 40, 255 };
    if (b.toggled) base = Color{ (unsigned char)(b.accent.r / 4 + 20),
                                 (unsigned char)(b.accent.g / 4 + 20),
                                 (unsigned char)(b.accent.b / 4 + 26), 255 };

    // soft shadow
    DrawRectangleRounded(Rectangle{ r.x, r.y + 3.0f, r.width, r.height }, 0.35f, 8,
                         Fade(BLACK, 0.35f));
    DrawRectangleRounded(r, 0.35f, 8, base);

    Color edge = b.enabled ? Fade(b.accent, 0.45f + 0.55f * b.hover) : Fade(Theme::EDGE, 0.6f);
    UIRoundedLines(r, 0.35f, 8, b.toggled ? 2.5f : 1.6f, edge);
    if (b.hover > 0.02f && b.enabled) UIGlowRect(r, 0.35f, b.accent, b.hover);

    Color txt = b.enabled ? Theme::TEXT : Color{ 90, 98, 130, 255 };
    if (b.toggled) txt = b.accent;

    int fs = 15;
    int tw = MeasureText(b.label.c_str(), fs);
    while (tw > r.width - 14 && fs > 10) { fs--; tw = MeasureText(b.label.c_str(), fs); }
    DrawText(b.label.c_str(), (int)(r.x + (r.width - tw) * 0.5f),
             (int)(r.y + (r.height - fs) * 0.5f), fs, txt);

    if (!b.hotkey.empty())
    {
        DrawText(b.hotkey.c_str(), (int)(r.x + r.width - MeasureText(b.hotkey.c_str(), 9) - 5),
                 (int)(r.y + 3), 9, Fade(Theme::MUTED, 0.9f));
    }
}

// ---------------------------------------------------------------- text field
int TextField::value(int fallback) const
{
    if (!hasValue()) return fallback;
    return atoi(text.c_str());
}

void UITextFieldUpdate(TextField& f, Vector2 mouse, float dt)
{
    if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
    {
        f.focused = CheckCollisionPointRec(mouse, f.rect);
    }

    if (f.shake > 0.0f)
    {
        f.shake -= dt * 3.0f;
        if (f.shake < 0.0f) f.shake = 0.0f;
    }

    if (!f.focused) return;

    int key = GetCharPressed();
    while (key > 0)
    {
        if ((int)f.text.size() < f.maxLen)
        {
            if (key >= '0' && key <= '9') f.text += (char)key;
            else if (key == '-' && f.allowNegative && f.text.empty()) f.text += '-';
        }
        key = GetCharPressed();
    }

    if (IsKeyPressed(KEY_BACKSPACE) && !f.text.empty()) f.text.pop_back();
    if (IsKeyPressed(KEY_DELETE)) f.text.clear();
}

void UITextFieldDraw(const TextField& f, float time)
{
    Rectangle r = f.rect;
    if (f.shake > 0.0f) r.x += sinf(time * 45.0f) * 5.0f * f.shake;

    DrawText(f.label.c_str(), (int)r.x, (int)(r.y - 15), 12, Theme::MUTED);
    DrawRectangleRounded(r, 0.3f, 8, Color{ 14, 16, 32, 255 });
    UIRoundedLines(r, 0.3f, 8, f.focused ? 2.4f : 1.4f,
                   f.focused ? Theme::CYAN : Theme::EDGE);
    if (f.focused) UIGlowRect(r, 0.3f, Theme::CYAN, 1.0f);

    const char* shown = f.text.empty() ? "" : f.text.c_str();
    DrawText(shown, (int)(r.x + 10), (int)(r.y + (r.height - 20) * 0.5f), 20, Theme::TEXT);

    if (f.text.empty() && !f.focused)
        DrawText("--", (int)(r.x + 10), (int)(r.y + (r.height - 20) * 0.5f), 20,
                 Fade(Theme::MUTED, 0.6f));

    if (f.focused && fmodf(time, 1.0f) < 0.5f)
    {
        int w = MeasureText(shown, 20);
        DrawRectangle((int)(r.x + 12 + w), (int)(r.y + 7), 2, (int)r.height - 14, Theme::CYAN);
    }
}

// -------------------------------------------------------------------- panels
void UIPanel(Rectangle r, const char* title, Color accent)
{
    DrawRectangleRounded(r, 0.045f, 10, Theme::PANEL);
    UIRoundedLines(r, 0.045f, 10, 1.6f, Fade(accent, 0.5f));
    if (title != nullptr && title[0] != '\0')
    {
        DrawRectangle((int)r.x + 14, (int)r.y + 13, 4, 14, accent);
        DrawText(title, (int)r.x + 26, (int)r.y + 13, 14, Fade(accent, 0.95f));
    }
}

void UIBadge(Rectangle r, const char* text, Color accent)
{
    DrawRectangleRounded(r, 0.5f, 8, Fade(accent, 0.18f));
    UIRoundedLines(r, 0.5f, 8, 1.4f, Fade(accent, 0.8f));
    UITextCentered(text, r.x + r.width * 0.5f, r.y + r.height * 0.5f, 13, accent);
}

// --------------------------------------------------------------- text wrap
float UITextWrapped(const std::string& text, Rectangle box, int fontSize,
                    int lineSpacing, Color color)
{
    float y = box.y;
    std::string word, line;

    auto flush = [&](bool force)
    {
        (void)force;
        if (!line.empty())
        {
            if (y + fontSize <= box.y + box.height)
                DrawText(line.c_str(), (int)box.x, (int)y, fontSize, color);
            y += fontSize + lineSpacing;
            line.clear();
        }
    };

    for (size_t i = 0; i <= text.size(); i++)
    {
        char c = (i < text.size()) ? text[i] : ' ';
        if (c == ' ' || c == '\n' || i == text.size())
        {
            if (!word.empty())
            {
                std::string test = line.empty() ? word : (line + " " + word);
                if (MeasureText(test.c_str(), fontSize) > box.width && !line.empty())
                {
                    flush(false);
                    line = word;
                }
                else line = test;
                word.clear();
            }
            if (c == '\n') flush(false);
        }
        else word += c;
    }
    flush(true);
    return y - box.y;
}
