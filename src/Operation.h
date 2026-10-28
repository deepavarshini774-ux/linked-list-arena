// ============================================================================
//  Operation.h  -  the "step machine" that turns one linked-list operation
//                  into a sequence of narrated, animated steps.
//
//  HOW IT WORKS (this is the heart of the project):
//
//   1. The user clicks e.g. "INSERT AT POS".
//   2. Operation::begin() *plans* the whole operation up front. Planning
//      validates the request against the REAL list. If it is impossible, the
//      plan becomes a teaching animation that walks a cursor through the list,
//      hits NULL, and explains which precondition failed. The list is never
//      touched in that case.
//   3. If it is possible, the plan is a list of Steps. Exactly one of those
//      steps (the Commit step) carries an onBegin() callback that performs the
//      real pointer surgery by calling LinkedList methods.
//   4. update(dt) advances through the steps using frame timing. Pause,
//      Resume, Next Step and the speed slider all just modulate this.
//
//  So the animation can never disagree with memory: the drawing code renders
//  whatever the real list contains, and the mutation happens inside the step
//  that the animation is currently showing.
// ============================================================================
#ifndef OPERATION_H
#define OPERATION_H

#include "LinkedList.h"
#include <string>
#include <vector>
#include <functional>

enum class OpType
{
    InsertFront, InsertBack, InsertAt,
    DeleteFront, DeleteBack, DeleteAt, DeleteValue,
    Search, Traverse, Count, ClearAll
};

enum class StepKind
{
    Message,   // narration only
    Walk,      // cursor travels from one anchor to another (spark on the arrow)
    Inspect,   // cursor sits on a node, node lights up
    Shoot,     // projectile flies from the cannon to the insertion slot
    Commit,    // THE real pointer change happens here (onBegin)
    Relink,    // show the new wiring settle
    Arm,       // red warning / countdown before an explosion
    Explode,   // boom
    Success,   // green banner
    Fail       // red banner + explanation, list untouched
};

// Where the little cursor character should be.
struct Anchor
{
    enum Kind { None, HeadMarker, NodeRef, NullMarker } kind = None;
    Node* node = nullptr;

    static Anchor none()            { Anchor a; a.kind = None;       return a; }
    static Anchor headMarker()      { Anchor a; a.kind = HeadMarker; return a; }
    static Anchor at(Node* n)       { Anchor a; a.kind = NodeRef; a.node = n; return a; }
    static Anchor nullMarker()      { Anchor a; a.kind = NullMarker; return a; }
};

struct Step
{
    StepKind    kind     = StepKind::Message;
    Anchor      from     = Anchor::none();
    Anchor      to       = Anchor::none();
    int         slot     = -1;      // 0-based layout slot (used by Shoot)
    float       duration = 0.8f;
    std::string line;               // short headline for the explain panel
    std::string detail;             // beginner-friendly paragraph
    std::string code;               // the C++ line(s) this step corresponds to
    std::function<void()> onBegin;  // runs once when the step starts
};

class Operation
{
public:
    Operation() = default;
    ~Operation() { cancel(); }
    Operation(const Operation&)            = delete;
    Operation& operator=(const Operation&) = delete;

    // Plan + start an operation. Safe to call while another one is running.
    void begin(LinkedList& list, OpType type, int value, int position);

    void update(float dt);
    void cancel();          // stop now, free an unlinked pending node

    // ---- state -------------------------------------------------------------
    bool  active()    const { return m_active; }
    bool  failed()    const { return m_failed; }
    bool  hasPlan()   const { return !m_steps.empty(); }
    const Step* current() const;
    float stepT()     const;                 // 0..1 progress inside this step
    int   stepIndex() const { return m_index; }
    int   stepCount() const { return (int)m_steps.size(); }
    bool  consumeStepBegan();                // true once per new step

    // ---- transport controls -------------------------------------------------
    void  togglePause()          { m_paused = !m_paused; }
    bool  paused() const         { return m_paused; }
    void  setStepMode(bool on)   { m_stepMode = on; }
    bool  stepMode() const       { return m_stepMode; }
    void  requestNext()          { m_nextRequested = true; m_paused = false; }
    void  setSpeed(float s)      { m_speed = s; }
    float speed() const          { return m_speed; }

    // ---- info for the panels ------------------------------------------------
    const std::string& title()      const { return m_title; }
    const std::string& complexity() const { return m_complexity; }
    const std::string& banner()     const { return m_banner; }
    const std::vector<int>& output()const { return m_output; }
    const std::string& outputLabel()const { return m_outputLabel; }
    int   counter() const                 { return m_counter; }

    // ---- insertion projectile ----------------------------------------------
    Node* pending() const { return m_pending; }
    bool  pendingLinked() const { return m_pendingLinked; }
    // The visualizer reports where the flying node currently is, so the Commit
    // step can drop the real node exactly there before it eases into place.
    void  setProjectilePos(Vector2 p) { m_projectile = p; }

private:
    void  resetPlan();
    Step& push(StepKind kind, float duration, const std::string& line,
               const std::string& detail, const std::string& code = "");
    // Adds cursor-walk steps from HEAD through node 1..upto. Returns node 'upto'.
    Node* walkTo(LinkedList& list, int upto, const std::string& goal);
    void  walkToNullForFailure(LinkedList& list, const std::string& goal);
    void  beginStep();

    // planners
    void planInsertAt   (LinkedList& list, int value, int pos,
                         const std::string& title, const std::string& complexity);
    void planDelete     (LinkedList& list, OpType type, int value, int pos);
    void planSearch     (LinkedList& list, int value);
    void planTraverse   (LinkedList& list);
    void planCount      (LinkedList& list);
    void planClear      (LinkedList& list);

    std::vector<Step> m_steps;
    int    m_index         = 0;
    float  m_t             = 0.0f;
    bool   m_active        = false;
    bool   m_failed        = false;
    bool   m_paused        = false;
    bool   m_stepMode      = false;
    bool   m_nextRequested = false;
    bool   m_stepBegan     = false;
    float  m_speed         = 1.0f;

    Node*   m_pending       = nullptr;   // allocated, not yet linked
    bool    m_pendingLinked = false;
    Vector2 m_projectile    = { 0.0f, 0.0f };

    std::string      m_title, m_complexity, m_banner, m_outputLabel;
    std::vector<int> m_output;
    int              m_counter = 0;
};

#endif // OPERATION_H
