// ============================================================================
//  LINKED LIST ARENA  -  an interactive singly-linked-list visualizer game.
//
//  main.cpp : window, main loop, control panel wiring, demo mode.
//
//  Build (Windows 11 + MinGW):
//     g++ -std=c++17 -O2 src/*.cpp -o LinkedListArena.exe ^
//         -I C:\raylib\raylib-5.5_win64_mingw-w64\include ^
//         -L C:\raylib\raylib-5.5_win64_mingw-w64\lib ^
//         -lraylib -lopengl32 -lgdi32 -lwinmm
//  See README.md for the full setup.
// ============================================================================
#include "raylib.h"
#include "LinkedList.h"
#include "Visualizer.h"
#include "Operation.h"
#include "Animation.h"
#include "UI.h"

#include <string>
#include <vector>
#include <cmath>

static const int SCREEN_W = 1280;
static const int SCREEN_H = 800;

// Every clickable control in the left panel.
enum ButtonId
{
    B_INS_FRONT = 0, B_INS_END, B_INS_POS,
    B_DEL_FRONT, B_DEL_END, B_DEL_POS, B_DEL_VALUE,
    B_SEARCH, B_TRAVERSE, B_COUNT, B_CLEAR,
    B_PAUSE, B_NEXT, B_STEPMODE, B_DEMO,
    B_SLOWER, B_FASTER, B_RESET,
    B_TOTAL
};

struct DemoAction { OpType type; int value; int pos; };

// ---------------------------------------------------------------------------
int main(void)
{
    SetConfigFlags(FLAG_MSAA_4X_HINT | FLAG_VSYNC_HINT);
    InitWindow(SCREEN_W, SCREEN_H, "LINKED LIST ARENA - C++ / raylib");
    SetTargetFPS(60);
    SetExitKey(KEY_NULL);           // ESC should not kill the app by accident

    // ---- the real data structure + the machinery around it -----------------
    LinkedList     list;
    Visualizer     vis;
    Operation      op;
    ParticleSystem fx;
    ScreenShake    shake;

    const Rectangle PANEL_LEFT  = {  12.0f,  66.0f, 248.0f, 722.0f };
    const Rectangle PANEL_ARENA = { 272.0f,  66.0f, 996.0f, 404.0f };
    const Rectangle PANEL_EXPL  = { 272.0f, 482.0f, 640.0f, 306.0f };
    const Rectangle PANEL_STAT  = { 924.0f, 482.0f, 344.0f, 150.0f };
    const Rectangle PANEL_OUT   = { 924.0f, 642.0f, 344.0f, 146.0f };
    vis.setArena(PANEL_ARENA);

    // ---- input fields -------------------------------------------------------
    TextField fValue;
    fValue.rect = { 26.0f, 122.0f, 104.0f, 36.0f };
    fValue.label = "VALUE";
    fValue.maxLen = 5;

    TextField fPos;
    fPos.rect = { 140.0f, 122.0f, 100.0f, 36.0f };
    fPos.label = "POSITION";
    fPos.maxLen = 3;
    fPos.allowNegative = false;

    // ---- buttons ------------------------------------------------------------
    std::vector<Button> btn(B_TOTAL);
    struct SectionLabel { std::string text; float y; };
    std::vector<SectionLabel> sections;

    {
        float y = 178.0f;
        auto header = [&](const char* t) { sections.push_back({ t, y }); y += 21.0f; };
        auto two = [&](int a, int b) {
            btn[a].rect = {  26.0f, y, 104.0f, 32.0f };
            btn[b].rect = { 136.0f, y, 104.0f, 32.0f };
            y += 37.0f;
        };
        auto one = [&](int a) {
            btn[a].rect = { 26.0f, y, 214.0f, 32.0f };
            y += 37.0f;
        };

        header("INSERT  (shoot it in)");
        two(B_INS_FRONT, B_INS_END);
        one(B_INS_POS);

        header("DELETE  (blow it up)");
        two(B_DEL_FRONT, B_DEL_END);
        two(B_DEL_POS, B_DEL_VALUE);

        header("EXPLORE");
        two(B_SEARCH, B_TRAVERSE);
        two(B_COUNT, B_CLEAR);

        header("PLAYBACK");
        two(B_PAUSE, B_NEXT);
        two(B_STEPMODE, B_DEMO);
        two(B_SLOWER, B_FASTER);
        one(B_RESET);
    }

    btn[B_INS_FRONT].label = "INS FRONT";  btn[B_INS_FRONT].hotkey = "Q";
    btn[B_INS_END  ].label = "INS END";    btn[B_INS_END  ].hotkey = "W";
    btn[B_INS_POS  ].label = "INSERT AT POSITION"; btn[B_INS_POS].hotkey = "E";
    btn[B_DEL_FRONT].label = "DEL FRONT";  btn[B_DEL_FRONT].hotkey = "A";
    btn[B_DEL_END  ].label = "DEL END";    btn[B_DEL_END  ].hotkey = "S";
    btn[B_DEL_POS  ].label = "DEL AT POS"; btn[B_DEL_POS  ].hotkey = "D";
    btn[B_DEL_VALUE].label = "DEL VALUE";  btn[B_DEL_VALUE].hotkey = "F";
    btn[B_SEARCH   ].label = "SEARCH";     btn[B_SEARCH   ].hotkey = "Z";
    btn[B_TRAVERSE ].label = "TRAVERSE";   btn[B_TRAVERSE ].hotkey = "X";
    btn[B_COUNT    ].label = "COUNT";      btn[B_COUNT    ].hotkey = "C";
    btn[B_CLEAR    ].label = "CLEAR LIST"; btn[B_CLEAR    ].hotkey = "V";
    btn[B_PAUSE    ].label = "PAUSE";      btn[B_PAUSE    ].hotkey = "SPC";
    btn[B_NEXT     ].label = "NEXT STEP";  btn[B_NEXT     ].hotkey = "N";
    btn[B_STEPMODE ].label = "STEP MODE";  btn[B_STEPMODE ].hotkey = "M";
    btn[B_DEMO     ].label = "DEMO MODE";  btn[B_DEMO     ].hotkey = "G";
    btn[B_SLOWER   ].label = "SPEED -";    btn[B_SLOWER   ].hotkey = "-";
    btn[B_FASTER   ].label = "SPEED +";    btn[B_FASTER   ].hotkey = "+";
    btn[B_RESET    ].label = "RESET LIST (instant)"; btn[B_RESET].hotkey = "R";

    for (int i = B_INS_FRONT; i <= B_INS_POS;   i++) btn[i].accent = Theme::MINT;
    for (int i = B_DEL_FRONT; i <= B_DEL_VALUE; i++) btn[i].accent = Theme::DANGER;
    for (int i = B_SEARCH;    i <= B_CLEAR;     i++) btn[i].accent = Theme::CYAN;
    btn[B_CLEAR].accent    = Theme::TANGERINE;
    btn[B_PAUSE].accent    = Theme::AMBER;
    btn[B_NEXT].accent     = Theme::AMBER;
    btn[B_STEPMODE].accent = Theme::LILAC;
    btn[B_DEMO].accent     = Theme::HOT;
    btn[B_SLOWER].accent   = Theme::MUTED;
    btn[B_FASTER].accent   = Theme::MUTED;
    btn[B_RESET].accent    = Theme::DANGER;

    // ---- misc state ---------------------------------------------------------
    float speed      = 1.0f;
    bool  stepMode   = false;
    float recoil     = 0.0f;
    std::string status = "Type a value, then press INS FRONT. Press G for the demo.";
    float statusTimer  = 6.0f;

    const DemoAction demoScript[] = {
        { OpType::InsertFront, 10,  0 },
        { OpType::InsertBack,  20,  0 },
        { OpType::InsertBack,  30,  0 },
        { OpType::InsertAt,    99,  2 },
        { OpType::Traverse,     0,  0 },
        { OpType::Search,      30,  0 },
        { OpType::Search,      77,  0 },   // fails: value not present
        { OpType::InsertAt,    55,  8 },   // fails: position out of range
        { OpType::DeleteValue, 99,  0 },
        { OpType::DeleteBack,   0,  0 },
        { OpType::DeleteAt,     0,  9 },   // fails: position does not exist
        { OpType::Count,        0,  0 },
        { OpType::ClearAll,     0,  0 }
    };
    const int demoCount = (int)(sizeof(demoScript) / sizeof(demoScript[0]));
    bool  demoMode  = false;
    int   demoIndex = 0;
    float demoTimer = 0.0f;
    bool  wasActive = false;

    // helper: start an operation, complaining if the inputs are missing
    auto needValue = [&](void) -> bool
    {
        if (!fValue.hasValue()) { fValue.shake = 1.0f; status = "Enter a VALUE first."; statusTimer = 3.0f; return false; }
        return true;
    };
    auto needPos = [&](void) -> bool
    {
        if (!fPos.hasValue()) { fPos.shake = 1.0f; status = "Enter a POSITION first (1-based)."; statusTimer = 3.0f; return false; }
        return true;
    };
    auto launch = [&](OpType t, int v, int p)
    {
        demoMode = false;
        op.begin(list, t, v, p);
        op.setSpeed(speed);
        op.setStepMode(stepMode);
    };

    // =======================================================================
    while (!WindowShouldClose())
    {
        const float dt   = GetFrameTime();
        const float time = (float)GetTime();
        Vector2 mouse    = GetMousePosition();

        // -------------------------------------------------------- input
        UITextFieldUpdate(fValue, mouse, dt);
        UITextFieldUpdate(fPos,   mouse, dt);
        if (IsKeyPressed(KEY_TAB))
        {
            bool v = fValue.focused;
            fValue.focused = !v && !fPos.focused ? true : !v;
            fPos.focused   = v;
        }
        vis.handleScroll(mouse);

        const bool busy = op.active();
        for (int i = 0; i < B_TOTAL; i++) btn[i].enabled = true;
        for (int i = B_INS_FRONT; i <= B_CLEAR; i++) btn[i].enabled = !busy;
        btn[B_PAUSE].enabled = busy;
        btn[B_NEXT].enabled  = busy;
        btn[B_PAUSE].toggled    = op.paused();
        btn[B_STEPMODE].toggled = stepMode;
        btn[B_DEMO].toggled     = demoMode;
        btn[B_PAUSE].label      = op.paused() ? "RESUME" : "PAUSE";

        // buttons + keyboard shortcuts share the same handlers
        bool typing = fValue.focused || fPos.focused;
        auto key = [&](int k) { return !typing && IsKeyPressed(k); };

        bool doInsFront = UIButtonUpdate(btn[B_INS_FRONT], mouse, dt) || (key(KEY_Q) && !busy);
        bool doInsEnd   = UIButtonUpdate(btn[B_INS_END],   mouse, dt) || (key(KEY_W) && !busy);
        bool doInsPos   = UIButtonUpdate(btn[B_INS_POS],   mouse, dt) || (key(KEY_E) && !busy);
        bool doDelFront = UIButtonUpdate(btn[B_DEL_FRONT], mouse, dt) || (key(KEY_A) && !busy);
        bool doDelEnd   = UIButtonUpdate(btn[B_DEL_END],   mouse, dt) || (key(KEY_S) && !busy);
        bool doDelPos   = UIButtonUpdate(btn[B_DEL_POS],   mouse, dt) || (key(KEY_D) && !busy);
        bool doDelVal   = UIButtonUpdate(btn[B_DEL_VALUE], mouse, dt) || (key(KEY_F) && !busy);
        bool doSearch   = UIButtonUpdate(btn[B_SEARCH],    mouse, dt) || (key(KEY_Z) && !busy);
        bool doTrav     = UIButtonUpdate(btn[B_TRAVERSE],  mouse, dt) || (key(KEY_X) && !busy);
        bool doCount    = UIButtonUpdate(btn[B_COUNT],     mouse, dt) || (key(KEY_C) && !busy);
        bool doClear    = UIButtonUpdate(btn[B_CLEAR],     mouse, dt) || (key(KEY_V) && !busy);
        bool doPause    = UIButtonUpdate(btn[B_PAUSE],     mouse, dt) || (key(KEY_SPACE) && busy);
        bool doNext     = UIButtonUpdate(btn[B_NEXT],      mouse, dt) || (key(KEY_N) && busy);
        bool doStepMode = UIButtonUpdate(btn[B_STEPMODE],  mouse, dt) || key(KEY_M);
        bool doDemo     = UIButtonUpdate(btn[B_DEMO],      mouse, dt) || key(KEY_G);
        bool doSlower   = UIButtonUpdate(btn[B_SLOWER],    mouse, dt) || key(KEY_MINUS);
        bool doFaster   = UIButtonUpdate(btn[B_FASTER],    mouse, dt) || key(KEY_EQUAL);
        bool doReset    = UIButtonUpdate(btn[B_RESET],     mouse, dt) || key(KEY_R);

        if (doInsFront && needValue()) launch(OpType::InsertFront, fValue.value(), 0);
        if (doInsEnd   && needValue()) launch(OpType::InsertBack,  fValue.value(), 0);
        if (doInsPos   && needValue() && needPos())
                                       launch(OpType::InsertAt,    fValue.value(), fPos.value(1));
        if (doDelFront)                launch(OpType::DeleteFront, 0, 0);
        if (doDelEnd)                  launch(OpType::DeleteBack,  0, 0);
        if (doDelPos   && needPos())   launch(OpType::DeleteAt,    0, fPos.value(1));
        if (doDelVal   && needValue()) launch(OpType::DeleteValue, fValue.value(), 0);
        if (doSearch   && needValue()) launch(OpType::Search,      fValue.value(), 0);
        if (doTrav)                    launch(OpType::Traverse,    0, 0);
        if (doCount)                   launch(OpType::Count,       0, 0);
        if (doClear)                   launch(OpType::ClearAll,    0, 0);

        if (doPause) op.togglePause();
        if (doNext)  op.requestNext();
        if (doStepMode) { stepMode = !stepMode; op.setStepMode(stepMode);
                          status = stepMode ? "STEP MODE on - press NEXT STEP (N) to advance."
                                            : "STEP MODE off - animations play automatically.";
                          statusTimer = 4.0f; }
        if (doSlower) { speed = (speed <= 0.3f) ? 0.25f : speed - 0.25f; op.setSpeed(speed); }
        if (doFaster) { speed = (speed >= 2.9f) ? 3.0f  : speed + 0.25f; op.setSpeed(speed); }

        if (doDemo)
        {
            demoMode = !demoMode;
            if (demoMode)
            {
                op.cancel();
                list.clear();
                fx.clear();
                demoIndex = 0;
                demoTimer = 0.5f;
                status = "DEMO MODE: watch a full tour of the operations.";
                statusTimer = 5.0f;
            }
            else status = "Demo stopped.";
        }

        if (doReset)
        {
            op.cancel();          // frees an in-flight (unlinked) node
            list.clear();         // frees every remaining node
            fx.clear();
            demoMode = false;
            status = "List reset: head = nullptr.";
            statusTimer = 3.0f;
        }

        // ------------------------------------------------------- demo driver
        if (demoMode)
        {
            if (op.active()) demoTimer = 1.3f;
            else
            {
                demoTimer -= dt;
                if (demoTimer <= 0.0f)
                {
                    if (demoIndex < demoCount)
                    {
                        const DemoAction& a = demoScript[demoIndex++];
                        op.begin(list, a.type, a.value, a.pos);
                        op.setSpeed(speed);
                        op.setStepMode(false);
                    }
                    else { demoMode = false; status = "Demo finished!"; statusTimer = 4.0f; }
                }
            }
        }

        // ------------------------------------------------------------ update
        op.update(dt);

        if (op.consumeStepBegan())
        {
            const Step* s = op.current();
            if (s != nullptr)
            {
                Vector2 p = vis.anchorPos(s->to, list);
                switch (s->kind)
                {
                    case StepKind::Shoot:
                        recoil = 1.0f;
                        break;
                    case StepKind::Explode:
                        fx.explosion(p, Theme::AMBER, Theme::DANGER, 80);
                        fx.smoke(p, 20);
                        shake.add(13.0f);
                        break;
                    case StepKind::Commit:
                        if (s->to.kind != Anchor::None) { fx.impact(p, Theme::MINT, 30); shake.add(4.0f); }
                        break;
                    case StepKind::Success:
                        fx.confetti((s->to.kind != Anchor::None)
                                    ? p
                                    : Vector2{ PANEL_ARENA.x + PANEL_ARENA.width * 0.5f,
                                               PANEL_ARENA.y + 120.0f }, 46);
                        break;
                    case StepKind::Fail:
                        shake.add(6.0f);
                        break;
                    default: break;
                }
            }
        }

        if (wasActive && !op.active()) demoTimer = 1.2f;
        wasActive = op.active();

        recoil -= dt * 3.0f; if (recoil < 0.0f) recoil = 0.0f;
        shake.update(dt);
        vis.setShake(shake.offset());
        vis.update(list, dt);
        fx.update(dt);
        if (statusTimer > 0.0f) statusTimer -= dt;

        // ============================================================ DRAW
        BeginDrawing();
        ClearBackground(Theme::BG);

        // ---- title bar --------------------------------------------------
        DrawRectangleGradientH(0, 0, SCREEN_W, 56, Color{ 22, 14, 48, 255 }, Color{ 10, 30, 56, 255 });
        DrawRectangle(0, 54, SCREEN_W, 2, Fade(Theme::CYAN, 0.55f));
        DrawText("LINKED LIST", 22, 14, 30, Theme::CYAN);
        DrawText("ARENA", 22 + MeasureText("LINKED LIST ", 30), 14, 30, Theme::HOT);
        DrawText("singly linked list - real nodes, real pointers, real memory",
                 22 + MeasureText("LINKED LIST ARENA  ", 30), 24, 14, Fade(Theme::MUTED, 0.95f));
        {
            std::string fpsTxt = std::to_string(GetFPS()) + " FPS";
            DrawText(fpsTxt.c_str(), SCREEN_W - MeasureText(fpsTxt.c_str(), 14) - 18, 21, 14,
                     Fade(Theme::MUTED, 0.8f));
        }

        // ---- arena ------------------------------------------------------
        UIPanel(PANEL_ARENA, "ARENA", Theme::CYAN);
        vis.drawArena(list, time);
        {
            const Step* s = op.current();
            float aimX = PANEL_ARENA.x + PANEL_ARENA.width * 0.5f;
            if (op.active() && s != nullptr && s->kind == StepKind::Shoot && s->slot >= 0)
                aimX = vis.slotPos(s->slot).x;
            vis.drawCannon(time, recoil, aimX);
        }
        vis.drawOverlay(list, op, fx, dt, time);
        fx.draw(Vector2{ 0.0f, 0.0f });

        // ---- left control panel ------------------------------------------
        UIPanel(PANEL_LEFT, "CONTROL PANEL", Theme::HOT);
        UITextFieldDraw(fValue, time);
        UITextFieldDraw(fPos,   time);
        for (size_t i = 0; i < sections.size(); i++)
            DrawText(sections[i].text.c_str(), 26, (int)sections[i].y, 12,
                     Fade(Theme::MUTED, 0.95f));
        // Buttons are drawn HERE, inside BeginDrawing/EndDrawing. Drawing them
        // during the input phase would put them on the screen before
        // ClearBackground() wiped it, so they never appeared at all.
        for (int i = 0; i < B_TOTAL; i++) UIButtonDraw(btn[i]);

        {
            float y = PANEL_LEFT.y + PANEL_LEFT.height - 96.0f;
            DrawLineEx(Vector2{ 26.0f, y - 12.0f }, Vector2{ 240.0f, y - 12.0f }, 1.0f,
                       Fade(Theme::EDGE, 0.8f));
            DrawText("TAB  switch field", 26, (int)y, 11, Fade(Theme::MUTED, 0.9f));
            DrawText("SPACE pause / N next step", 26, (int)y + 16, 11, Fade(Theme::MUTED, 0.9f));
            DrawText("mouse wheel scrolls the arena", 26, (int)y + 32, 11, Fade(Theme::MUTED, 0.9f));
            std::string sp = "speed x" + std::to_string(speed).substr(0, 4);
            DrawText(sp.c_str(), 26, (int)y + 52, 13, Theme::AMBER);
        }

        // ---- explanation panel -------------------------------------------
        UIPanel(PANEL_EXPL, "WHAT IS HAPPENING INSIDE", Theme::MINT);
        {
            const Step* s = op.current();
            float x = PANEL_EXPL.x + 18.0f;
            float w = PANEL_EXPL.width - 36.0f;
            float y = PANEL_EXPL.y + 40.0f;

            if (s == nullptr)
            {
                DrawText("No operation yet.", (int)x, (int)y, 18, Theme::TEXT);
                UITextWrapped("Pick an operation on the left. Every step of the algorithm will be "
                              "explained here, and the boxes in the arena are the actual nodes that "
                              "exist on the heap right now. Nothing on screen is faked: when a node "
                              "is drawn, a real 'new Node' happened; when it explodes, a real "
                              "'delete' happened.",
                              Rectangle{ x, y + 28.0f, w, 120.0f }, 14, 5, Theme::MUTED);
            }
            else
            {
                // title + complexity badge
                DrawText(op.title().c_str(), (int)x, (int)y, 19,
                         op.failed() ? Theme::DANGER : Theme::CYAN);
                if (!op.complexity().empty())
                {
                    std::string cx = "TIME: " + op.complexity();
                    float bw = (float)MeasureText(cx.c_str(), 13) + 22.0f;
                    UIBadge(Rectangle{ PANEL_EXPL.x + PANEL_EXPL.width - bw - 18.0f, y - 4.0f,
                                       bw, 26.0f }, cx.c_str(), Theme::AMBER);
                }
                y += 30.0f;

                // step counter + progress bar
                std::string prog = "step " + std::to_string(op.stepIndex() + 1) + " / " +
                                   std::to_string(op.stepCount());
                DrawText(prog.c_str(), (int)x, (int)y, 12, Theme::MUTED);
                {
                    float barW = w - 110.0f;
                    Rectangle bg = { x + 100.0f, y + 2.0f, barW, 8.0f };
                    DrawRectangleRounded(bg, 1.0f, 6, Fade(Theme::EDGE, 0.7f));
                    float f = (op.stepCount() > 1)
                            ? ((float)op.stepIndex() + op.stepT()) / (float)op.stepCount()
                            : op.stepT();
                    DrawRectangleRounded(Rectangle{ bg.x, bg.y, barW * f, 8.0f }, 1.0f, 6,
                                         op.failed() ? Theme::DANGER : Theme::MINT);
                }
                y += 24.0f;

                // headline for this step
                Color headColor = Theme::TEXT;
                if (s->kind == StepKind::Fail)    headColor = Theme::DANGER;
                if (s->kind == StepKind::Success) headColor = Theme::MINT;
                y += UITextWrapped(s->line, Rectangle{ x, y, w, 56.0f }, 18, 5, headColor);
                y += 6.0f;

                // the beginner-friendly paragraph
                y += UITextWrapped(s->detail, Rectangle{ x, y, w, 130.0f }, 14, 6, Theme::MUTED);

                // the C++ the step corresponds to
                if (!s->code.empty())
                {
                    float boxY = PANEL_EXPL.y + PANEL_EXPL.height - 84.0f;
                    Rectangle cb = { x, boxY, w, 70.0f };
                    DrawRectangleRounded(cb, 0.12f, 8, Color{ 12, 14, 30, 255 });
                    UIRoundedLines(cb, 0.12f, 8, 1.2f, Fade(Theme::CYAN, 0.35f));
                    UITextWrapped(s->code, Rectangle{ cb.x + 12.0f, cb.y + 10.0f,
                                                      cb.width - 24.0f, 54.0f },
                                  14, 4, Theme::CYAN);
                }
            }
        }

        // ---- status panel --------------------------------------------------
        UIPanel(PANEL_STAT, "STATUS", Theme::AMBER);
        {
            float x = PANEL_STAT.x + 18.0f;
            float y = PANEL_STAT.y + 42.0f;
            int   n = list.count();

            std::string nodes = "NODES: " + std::to_string(n);
            DrawText(nodes.c_str(), (int)x, (int)y, 22, Theme::TEXT);

            std::string headTxt = list.empty()
                                ? "head = nullptr"
                                : ("head -> [" + std::to_string(list.head()->data) + "]");
            DrawText(headTxt.c_str(), (int)x, (int)y + 30, 15, Theme::HOT);

            const char* st = op.active() ? (op.paused() ? "PAUSED"
                                            : (stepMode ? "WAITING FOR NEXT STEP" : "RUNNING"))
                                         : (op.hasPlan() ? "FINISHED" : "IDLE");
            Color stc = op.active() ? (op.paused() ? Theme::AMBER : Theme::MINT) : Theme::MUTED;
            DrawText(st, (int)x, (int)y + 54, 15, stc);

            if (!op.banner().empty())
            {
                Rectangle br = { PANEL_STAT.x + 14.0f, PANEL_STAT.y + PANEL_STAT.height - 40.0f,
                                 PANEL_STAT.width - 28.0f, 28.0f };
                UIBadge(br, op.banner().c_str(), op.failed() ? Theme::DANGER : Theme::MINT);
            }
            else if (statusTimer > 0.0f)
            {
                UITextWrapped(status, Rectangle{ x, PANEL_STAT.y + PANEL_STAT.height - 44.0f,
                                                 PANEL_STAT.width - 36.0f, 34.0f },
                              12, 3, Fade(Theme::AMBER, 0.95f));
            }
        }

        // ---- output panel ----------------------------------------------------
        UIPanel(PANEL_OUT, op.outputLabel().empty() ? "OUTPUT" : op.outputLabel().c_str(),
                Theme::LILAC);
        {
            float x = PANEL_OUT.x + 18.0f;
            float y = PANEL_OUT.y + 42.0f;
            const std::vector<int>& out = op.output();

            if (out.empty())
            {
                DrawText("- run TRAVERSE, SEARCH or COUNT -", (int)x, (int)y, 12,
                         Fade(Theme::MUTED, 0.9f));
            }
            else
            {
                float cx = x, cy = y;
                for (size_t i = 0; i < out.size(); i++)
                {
                    std::string v = std::to_string(out[i]);
                    float cw = (float)MeasureText(v.c_str(), 14) + 18.0f;
                    if (cx + cw > PANEL_OUT.x + PANEL_OUT.width - 18.0f)
                    {
                        cx = x; cy += 30.0f;
                        if (cy > PANEL_OUT.y + PANEL_OUT.height - 40.0f) break;
                    }
                    Rectangle chip = { cx, cy, cw, 24.0f };
                    DrawRectangleRounded(chip, 0.5f, 6, Fade(Theme::LILAC, 0.18f));
                    UIRoundedLines(chip, 0.5f, 6, 1.2f, Fade(Theme::LILAC, 0.85f));
                    DrawText(v.c_str(), (int)(cx + 9.0f), (int)(cy + 5.0f), 14, Theme::TEXT);
                    cx += cw + 8.0f;

                    if (i + 1 < out.size())
                    {
                        DrawText("->", (int)cx, (int)(cy + 6.0f), 12, Fade(Theme::MUTED, 0.9f));
                        cx += 20.0f;
                    }
                }
            }

            if (op.counter() > 0)
            {
                std::string c = "counter = " + std::to_string(op.counter());
                DrawText(c.c_str(), (int)x, (int)(PANEL_OUT.y + PANEL_OUT.height - 26.0f), 14,
                         Theme::AMBER);
            }
        }

        EndDrawing();
    }

    // Shutdown: cancel() frees a node that was planned but never linked,
    // and ~LinkedList() frees every node still in the list.
    op.cancel();
    list.clear();
    CloseWindow();
    return 0;
}
