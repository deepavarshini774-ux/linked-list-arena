// ============================================================================
//  Operation.cpp  -  planning + running one animated linked-list operation.
// ============================================================================
#include "Operation.h"
#include <string>

static std::string S(int v) { return std::to_string(v); }

// ---------------------------------------------------------------- lifecycle
void Operation::resetPlan()
{
    m_steps.clear();
    m_index = 0;
    m_t = 0.0f;
    m_active = false;
    m_failed = false;
    m_paused = false;
    m_nextRequested = false;
    m_stepBegan = false;
    m_pending = nullptr;
    m_pendingLinked = false;
    m_title.clear();
    m_complexity.clear();
    m_banner.clear();
    m_outputLabel.clear();
    m_output.clear();
    m_counter = 0;
}

void Operation::cancel()
{
    // If an insertion was planned but never committed, the new node is still
    // owned by us. Free it so nothing leaks.
    if (m_pending != nullptr && !m_pendingLinked)
    {
        delete m_pending;
    }
    m_pending = nullptr;
    m_pendingLinked = false;
    m_active = false;
    m_steps.clear();
    m_index = 0;
}

Step& Operation::push(StepKind kind, float duration, const std::string& line,
                      const std::string& detail, const std::string& code)
{
    Step s;
    s.kind     = kind;
    s.duration = duration;
    s.line     = line;
    s.detail   = detail;
    s.code     = code;
    m_steps.push_back(s);
    return m_steps.back();
}

const Step* Operation::current() const
{
    if (m_steps.empty()) return nullptr;
    int i = m_index;
    if (i >= (int)m_steps.size()) i = (int)m_steps.size() - 1;
    return &m_steps[i];
}

float Operation::stepT() const
{
    const Step* s = current();
    if (s == nullptr || s->duration <= 0.0f) return 1.0f;
    float t = m_t / s->duration;
    return (t > 1.0f) ? 1.0f : t;
}

bool Operation::consumeStepBegan()
{
    bool b = m_stepBegan;
    m_stepBegan = false;
    return b;
}

void Operation::beginStep()
{
    m_t = 0.0f;
    m_stepBegan = true;
    if (m_index < (int)m_steps.size() && m_steps[m_index].onBegin)
    {
        m_steps[m_index].onBegin();
    }
}

void Operation::update(float dt)
{
    if (!m_active || m_steps.empty()) return;
    if (m_paused) return;

    m_t += dt * m_speed;

    const Step& s = m_steps[m_index];
    if (m_t >= s.duration)
    {
        if (m_stepMode && !m_nextRequested)
        {
            m_t = s.duration;        // hold here until the user presses NEXT
            return;
        }
        m_nextRequested = false;
        m_index++;
        if (m_index >= (int)m_steps.size())
        {
            m_index = (int)m_steps.size() - 1;
            m_t     = m_steps[m_index].duration;
            m_active = false;        // done; the final panel text stays visible
            return;
        }
        beginStep();
    }
}

// ------------------------------------------------------------------ walking
Node* Operation::walkTo(LinkedList& list, int upto, const std::string& goal)
{
    Node* prev = nullptr;
    Node* cur  = nullptr;
    for (int i = 1; i <= upto; i++)
    {
        Node* n = list.nodeAt(i);
        if (n == nullptr) break;

        Step& st = push(StepKind::Walk, 0.55f,
            "current = node " + S(i) + "   (value " + S(n->data) + ")",
            goal + "  A temporary pointer walks the list one link at a time. "
                   "It cannot jump: a singly linked list only knows where 'next' is. "
                   "Now standing on position " + S(i) + ".",
            (i == 1) ? "Node* current = head;" : "current = current->next;");
        st.from = (i == 1) ? Anchor::headMarker() : Anchor::at(prev);
        st.to   = Anchor::at(n);
        st.onBegin = [n]() { n->vis.glow = 1.0f; };

        prev = n;
        cur  = n;
    }
    return cur;
}

void Operation::walkToNullForFailure(LinkedList& list, const std::string& goal)
{
    int n = list.count();
    Node* lastNode = walkTo(list, n, goal);

    Step& st = push(StepKind::Walk, 0.9f,
        "current = NULL   (the walk is over)",
        "After node " + S(n) + " the next pointer is nullptr, so there is nowhere "
        "else to go. The list simply does not extend that far.",
        "current = current->next;   // now nullptr");
    st.from = (lastNode != nullptr) ? Anchor::at(lastNode) : Anchor::headMarker();
    st.to   = Anchor::nullMarker();
}

// ------------------------------------------------------------------- begin()
void Operation::begin(LinkedList& list, OpType type, int value, int position)
{
    cancel();        // frees a leftover pending node, if any
    resetPlan();

    switch (type)
    {
        case OpType::InsertFront:
            planInsertAt(list, value, 1, "INSERT AT BEGINNING", "O(1)");
            break;
        case OpType::InsertBack:
            planInsertAt(list, value, list.count() + 1, "INSERT AT END",
                         "O(n)  (no tail pointer, so we must walk to the end)");
            break;
        case OpType::InsertAt:
            planInsertAt(list, value, position, "INSERT AT POSITION " + S(position),
                         (position == 1) ? "O(1)" : "O(n)");
            break;
        case OpType::DeleteFront:
        case OpType::DeleteBack:
        case OpType::DeleteAt:
        case OpType::DeleteValue:
            planDelete(list, type, value, position);
            break;
        case OpType::Search:   planSearch(list, value); break;
        case OpType::Traverse: planTraverse(list);      break;
        case OpType::Count:    planCount(list);         break;
        case OpType::ClearAll: planClear(list);         break;
    }

    m_active = !m_steps.empty();
    m_index  = 0;
    if (m_active) beginStep();
}

// ===================================================================== INSERT
void Operation::planInsertAt(LinkedList& list, int value, int pos,
                             const std::string& title, const std::string& complexity)
{
    const int n = list.count();
    m_title      = title;
    m_complexity = complexity;

    // ---------------------------------------------------------- invalid case
    if (pos < 1 || pos > n + 1)
    {
        m_failed = true;
        m_banner = "INSERTION FAILED";
        m_complexity = "nothing was inserted";

        push(StepKind::Message, 1.3f,
             "Goal: insert " + S(value) + " at position " + S(pos) + ".",
             "This list currently has " + S(n) + " node(s), so the only legal insert "
             "positions are 1 to " + S(n + 1) + ". Position " + S(n + 1) +
             " means 'attach to the end'. Let's see what happens if we try anyway.");

        if (pos < 1)
        {
            Step& f = push(StepKind::Fail, 3.2f,
                "INSERTION FAILED: Position " + S(pos) + " is invalid.",
                "Positions here are 1-based: position 1 is the very first node, right "
                "after HEAD. There is no position " + S(pos) + " in any linked list, so "
                "there is no pointer to redirect. The list was NOT modified.",
                "if (pos < 1) return;   // reject before touching any pointer");
            f.to = Anchor::headMarker();
            return;
        }

        push(StepKind::Message, 1.2f,
             "To insert at position " + S(pos) + " we first need the node at position " + S(pos - 1) + ".",
             "Insertion always works by redirecting the PREVIOUS node's next pointer. "
             "So before we can insert at position " + S(pos) + ", a node must already exist "
             "at position " + S(pos - 1) + ". Let's walk the list and look for it.",
             "Node* current = head;  for (int i = 1; i < pos - 1; i++) current = current->next;");

        if (n == 0)
        {
            Step& e = push(StepKind::Inspect, 1.2f,
                "head is nullptr - the list is empty.",
                "An empty list is drawn as HEAD -> NULL. There is no node 1, "
                "so there is certainly no node " + S(pos - 1) + ".",
                "head == nullptr");
            e.to = Anchor::headMarker();
        }
        else
        {
            walkToNullForFailure(list, "Looking for position " + S(pos - 1) + ".");
        }

        Step& f = push(StepKind::Fail, 3.6f,
            "INSERTION FAILED: Position " + S(pos) + " is invalid for this list.",
            "There are only " + S(n) + " node(s). To insert at position " + S(pos) +
            ", a previous node at position " + S(pos - 1) + " must exist. Traversal reached "
            "NULL before reaching that position, so there is no next pointer to redirect. "
            "The list was NOT modified. Try a position between 1 and " + S(n + 1) + ".",
            "// precondition failed -> no allocation, no pointer change");
        f.to = Anchor::nullMarker();
        return;
    }

    // ------------------------------------------------------------ valid case
    m_banner  = "";
    m_pending = new Node(value);          // owned by us until the Commit step
    m_pendingLinked = false;

    LinkedList* lp = &list;
    Node* newNode  = m_pending;

    push(StepKind::Message, 1.2f,
         "Step 1: allocate the new node.",
         "A node is created on the heap with data = " + S(value) + " and next = nullptr. "
         "At this moment it is floating free: nothing in the list points to it yet.",
         "Node* newNode = new Node(" + S(value) + ");   // newNode->next = nullptr");

    if (pos == 1)
    {
        push(StepKind::Message, 1.2f,
             "Step 2: position 1 means the new node becomes the head.",
             "No traversal is needed at all - we already have the head pointer in our hand. "
             "That is why inserting at the beginning is O(1), no matter how long the list is.",
             "// no walking required");

        Step& sh = push(StepKind::Shoot, 0.85f,
            "Step 3: firing the node into slot 1...",
            "The cannon launches the new node towards the front of the list.",
            "// aiming at position 1");
        sh.slot = 0;

        Step& cm = push(StepKind::Commit, 1.4f,
            "Step 4: newNode->next = head;   head = newNode;",
            "Order matters! First the new node is told to point at the old first node, "
            "THEN head is moved to the new node. Doing it the other way round would lose "
            "the rest of the list forever.",
            "newNode->next = head;\nhead = newNode;");
        cm.to = Anchor::headMarker();
        cm.onBegin = [this, lp, newNode]()
        {
            newNode->vis.pos    = m_projectile;   // drop it where the shell landed
            newNode->vis.placed = true;
            newNode->vis.scale  = 1.5f;
            newNode->vis.glow   = 1.0f;
            lp->insertFront(newNode);             // <-- REAL pointer change
            m_pendingLinked = true;
        };
    }
    else
    {
        push(StepKind::Message, 1.2f,
             "Step 2: walk from HEAD to position " + S(pos - 1) + ".",
             "A singly linked list has no random access, so we must follow next pointers "
             "one at a time until we stand on the node just before the insertion point.",
             "Node* current = head;\nfor (int i = 1; i < " + S(pos) + "; i++) current = current->next;");

        Node* prev = walkTo(list, pos - 1, "Target: the node at position " + S(pos - 1) + ".");

        Step& sh = push(StepKind::Shoot, 0.85f,
            "Step 3: firing the node into slot " + S(pos) + "...",
            "The cannon launches the new node at the gap between position " + S(pos - 1) +
            " and position " + S(pos) + ".",
            "// aiming at position " + S(pos));
        sh.slot = pos - 1;
        sh.from = Anchor::at(prev);

        Step& cm = push(StepKind::Commit, 1.6f,
            "Step 4: newNode->next = current->next;   current->next = newNode;",
            "First the new node adopts whatever current was pointing at (so the tail of the "
            "list is not lost). Only then does current point at the new node. If you swap "
            "these two lines you leak the rest of the list.",
            "newNode->next = current->next;\ncurrent->next = newNode;");
        cm.to = Anchor::at(prev);
        cm.onBegin = [this, lp, prev, newNode]()
        {
            newNode->vis.pos    = m_projectile;
            newNode->vis.placed = true;
            newNode->vis.scale  = 1.5f;
            newNode->vis.glow   = 1.0f;
            if (prev != nullptr) prev->vis.arrow = 1.0f;
            lp->insertAfter(prev, newNode);       // <-- REAL pointer change
            m_pendingLinked = true;
        };
    }

    Step& rl = push(StepKind::Relink, 1.1f,
        "Step 5: the links settle into place.",
        "Notice that nothing else moved in memory - the other nodes never changed address. "
        "Only one or two pointers were rewritten, which is exactly why linked lists are good "
        "at inserting in the middle.",
        "// list is consistent again");
    rl.to = Anchor::at(newNode);

    Step& ok = push(StepKind::Success, 1.8f,
        "NODE INSERTED SUCCESSFULLY!",
        "Value " + S(value) + " now lives at position " + S(pos) + ". Time complexity of this "
        "operation: " + complexity + ".",
        "");
    ok.to = Anchor::at(newNode);
    ok.onBegin = [this]() { m_banner = "NODE INSERTED SUCCESSFULLY!"; };
}

// ===================================================================== DELETE
void Operation::planDelete(LinkedList& list, OpType type, int value, int pos)
{
    const int n = list.count();
    int targetPos = -1;

    switch (type)
    {
        case OpType::DeleteFront:
            m_title = "DELETE FROM BEGINNING"; m_complexity = "O(1)"; targetPos = 1; break;
        case OpType::DeleteBack:
            m_title = "DELETE FROM END";
            m_complexity = "O(n)  (we must find the second-to-last node)"; targetPos = n; break;
        case OpType::DeleteAt:
            m_title = "DELETE AT POSITION " + S(pos);
            m_complexity = (pos == 1) ? "O(1)" : "O(n)"; targetPos = pos; break;
        default:
            m_title = "DELETE BY VALUE " + S(value); m_complexity = "O(n)"; break;
    }

    // ------------------------------------------------- empty list: teach why
    if (n == 0)
    {
        m_failed = true;
        m_banner = "DELETION FAILED";
        m_complexity = "nothing was deleted";

        push(StepKind::Message, 1.3f,
             "Goal: " + m_title + ".",
             "Before deleting anything we always ask the same question: is head nullptr?",
             "if (head == nullptr) { /* nothing to delete */ }");

        Step& e = push(StepKind::Inspect, 1.6f,
            "head is nullptr - the list is empty.",
            "An empty list is drawn as HEAD -> NULL. There is no first node, no last node and "
            "no node at any position.",
            "head == nullptr");
        e.to = Anchor::headMarker();

        Step& f = push(StepKind::Fail, 3.2f,
            "DELETION FAILED: the list is empty.",
            "Deletion needs an existing node whose memory we can free and whose predecessor's "
            "next pointer we can repair. With head == nullptr there is nothing to unlink, so "
            "the operation stops here. Insert something first!",
            "// precondition failed -> no delete, no dangling pointer");
        f.to = Anchor::nullMarker();
        return;
    }

    // --------------------------------------------- delete by value: search it
    Node* prev   = nullptr;
    Node* target = nullptr;

    if (type == OpType::DeleteValue)
    {
        push(StepKind::Message, 1.2f,
             "Goal: find the first node whose data is " + S(value) + ", then unlink it.",
             "We walk the list comparing each node's data, while remembering the previous "
             "node - because to delete a node we must repair the pointer that leads INTO it.",
             "Node* prev = nullptr;\nNode* current = head;");

        Node* p = nullptr;
        int foundPos = -1;
        int i = 1;
        for (Node* cur = list.head(); cur != nullptr; cur = cur->next, i++)
        {
            bool match = (cur->data == value);
            Step& st = push(StepKind::Inspect, 0.7f,
                "node " + S(i) + ": is " + S(cur->data) + " == " + S(value) + " ?   " + (match ? "YES" : "no"),
                match ? "Match found at position " + S(i) + ". Stop walking."
                      : "No match, so remember this node as 'prev' and follow its next pointer.",
                "if (current->data == target) break;\nprev = current;  current = current->next;");
            st.from = (i == 1) ? Anchor::headMarker() : Anchor::at(p);
            st.to   = Anchor::at(cur);
            Node* cn = cur;
            st.onBegin = [cn, match]() { cn->vis.glow = 1.0f; if (match) cn->vis.danger = 1.0f; };

            if (match) { target = cur; prev = p; foundPos = i; break; }
            p = cur;
        }

        if (target == nullptr)
        {
            m_failed = true;
            m_banner = "DELETION FAILED";
            m_complexity = "nothing was deleted";

            Step& w = push(StepKind::Walk, 0.9f,
                "current = NULL   (end of list)",
                "The last node's next pointer is nullptr, so the search is over.",
                "current = current->next;   // nullptr");
            w.from = Anchor::at(list.last());
            w.to   = Anchor::nullMarker();

            Step& f = push(StepKind::Fail, 3.4f,
                "DELETION FAILED: value " + S(value) + " is not in the list.",
                "Every one of the " + S(n) + " nodes was compared and none of them holds " +
                S(value) + ". The traversal reached NULL, which is the only way a singly linked "
                "list can say 'that is everything I have'. Nothing was deleted and no memory was freed.",
                "// value never matched -> list unchanged");
            f.to = Anchor::nullMarker();
            return;
        }
        targetPos = foundPos;
    }
    // -------------------------------------- other deletes: validate the index
    else
    {
        if (targetPos < 1 || targetPos > n)
        {
            m_failed = true;
            m_banner = "DELETION FAILED";
            m_complexity = "nothing was deleted";

            push(StepKind::Message, 1.3f,
                 "Goal: delete the node at position " + S(targetPos) + ".",
                 "This list has " + S(n) + " node(s), so the only valid positions are 1 to " +
                 S(n) + ". Let's walk and see.",
                 "if (pos < 1 || pos > length) { /* invalid */ }");

            if (targetPos >= 1) walkToNullForFailure(list, "Looking for position " + S(targetPos) + ".");

            Step& f = push(StepKind::Fail, 3.4f,
                "DELETION FAILED: position " + S(targetPos) + " does not exist.",
                "There are only " + S(n) + " node(s) in the list, and traversal reached NULL "
                "before arriving at position " + S(targetPos) + ". A node that does not exist "
                "cannot be unlinked or freed, so the list was left exactly as it was.",
                "// precondition failed -> list unchanged");
            f.to = Anchor::nullMarker();
            return;
        }

        if (targetPos == 1)
        {
            push(StepKind::Message, 1.2f,
                 "Deleting the first node: no traversal needed.",
                 "head already points at the victim, so we can unlink it immediately. "
                 "That is why deleting from the beginning is O(1).",
                 "Node* temp = head;");
            target = list.head();
            prev   = nullptr;
        }
        else
        {
            push(StepKind::Message, 1.2f,
                 "Walk to position " + S(targetPos - 1) + " - the node BEFORE the victim.",
                 "To delete a node you must hold its predecessor, because the predecessor's "
                 "next pointer is the one that has to be repaired. A singly linked list cannot "
                 "look backwards.",
                 "Node* prev = head;\nfor (int i = 1; i < " + S(targetPos) + "; i++) prev = prev->next;");
            prev   = walkTo(list, targetPos - 1, "Target: the node before position " + S(targetPos) + ".");
            target = list.nodeAt(targetPos);
        }
    }

    if (target == nullptr) return;   // defensive; should not happen

    const int  targetValue = target->data;
    const bool deletingHead = (prev == nullptr);
    const bool deletingLast = (target->next == nullptr);

    Step& arm = push(StepKind::Arm, 1.2f,
        "Target locked: node " + S(targetPos) + " (value " + S(targetValue) + ").",
        "Before the node disappears we save a pointer to it, because once the links are "
        "rewired we would have no way to find it again - and forgetting it would leak memory.",
        deletingHead ? "Node* temp = head;" : "Node* temp = prev->next;");
    arm.to = Anchor::at(target);
    {
        Node* t = target;
        arm.onBegin = [t]() { t->vis.danger = 1.0f; };
    }

    Step& boom = push(StepKind::Explode, 0.95f,
        "BOOM!",
        "The node is being removed from the list.",
        "");
    boom.to = Anchor::at(target);
    {
        Node* t = target;
        boom.onBegin = [t]() { t->vis.danger = 1.0f; };
    }

    LinkedList* lp = &list;
    Step& cm = push(StepKind::Commit, 1.5f,
        deletingHead ? "head = head->next;   delete temp;"
                     : "prev->next = temp->next;   delete temp;",
        deletingHead
            ? "HEAD now points at what used to be the second node (or nullptr if there was "
              "only one). Only after the list no longer references the node do we call delete, "
              "so no other pointer is ever left dangling."
            : "The previous node now points straight at the victim's successor, jumping over it. "
              "The node is unlinked FIRST and freed SECOND - never the other way round.",
        deletingHead ? "head = head->next;\ndelete temp;"
                     : "prev->next = temp->next;\ndelete temp;");
    cm.to = deletingHead ? Anchor::headMarker() : Anchor::at(prev);
    cm.onBegin = [lp, prev]()
    {
        if (prev == nullptr) lp->removeFront();     // <-- REAL delete + free
        else                 { prev->vis.arrow = 1.0f; lp->removeAfter(prev); }
    };

    Step& rl = push(StepKind::Relink, 1.1f,
        deletingHead ? "The HEAD marker has moved to the next node."
                     : (deletingLast ? "The previous node's next is now nullptr."
                                     : "The gap is closed: prev now points to the successor."),
        deletingLast
            ? "Because the victim was the last node, its predecessor's next pointer is now "
              "nullptr, so the NULL marker slides left."
            : "The surviving nodes never moved in memory; only one pointer changed.",
        "");
    rl.to = deletingHead ? Anchor::headMarker() : Anchor::at(prev);

    Step& ok = push(StepKind::Success, 1.8f,
        "BOOM! NODE DELETED!",
        "Value " + S(targetValue) + " was removed from position " + S(targetPos) +
        " and its memory was freed with delete. Time complexity: " + m_complexity + ".",
        "");
    ok.onBegin = [this]() { m_banner = "BOOM! NODE DELETED!"; };
}

// ===================================================================== SEARCH
void Operation::planSearch(LinkedList& list, int value)
{
    const int n = list.count();
    m_title       = "SEARCH FOR " + S(value);
    m_complexity  = "O(n)";
    m_outputLabel = "COMPARISONS";

    if (n == 0)
    {
        m_failed = true;
        m_banner = "VALUE NOT FOUND";
        Step& e = push(StepKind::Inspect, 1.5f,
            "head is nullptr - the list is empty.",
            "Searching starts at head. With head == nullptr there is not a single node to "
            "compare against, so the search ends before it begins. An empty list is HEAD -> NULL.",
            "Node* current = head;   // nullptr");
        e.to = Anchor::headMarker();

        Step& f = push(StepKind::Fail, 3.0f,
            "SEARCH FAILED: the list is empty.",
            "There are 0 nodes, so " + S(value) + " cannot be here. Insert some values and try again.",
            "while (current != nullptr) { ... }   // loop body never runs");
        f.to = Anchor::nullMarker();
        return;
    }

    push(StepKind::Message, 1.2f,
         "Start at HEAD and compare each node's data with " + S(value) + ".",
         "Linked lists have no index arithmetic, so searching means visiting nodes one by one. "
         "In the worst case we touch every node: O(n).",
         "Node* current = head;\nwhile (current != nullptr) { if (current->data == key) break; current = current->next; }");

    Node* prev = nullptr;
    int i = 1;
    for (Node* cur = list.head(); cur != nullptr; cur = cur->next, i++)
    {
        bool match = (cur->data == value);
        Step& st = push(StepKind::Inspect, 0.75f,
            "node " + S(i) + ":  " + S(cur->data) + " == " + S(value) + " ?   " + (match ? "YES!" : "no"),
            match ? "The values are equal, so the search stops right here - a linear search "
                    "returns the FIRST match."
                  : "Not equal, so follow this node's next pointer and keep looking.",
            match ? "if (current->data == key) return current;"
                  : "current = current->next;");
        st.from = (i == 1) ? Anchor::headMarker() : Anchor::at(prev);
        st.to   = Anchor::at(cur);
        Node* cn = cur;
        st.onBegin = [this, cn, match]()
        {
            m_counter++;
            m_output.push_back(cn->data);
            cn->vis.glow = 1.0f;
            if (match) cn->vis.scale = 1.35f;
        };

        if (match)
        {
            Step& ok = push(StepKind::Success, 2.0f,
                "FOUND! Value " + S(value) + " is at position " + S(i) + ".",
                "It took " + S(i) + " comparison(s). Note that a linked list cannot do binary "
                "search even if the data is sorted, because there is no way to jump to the middle.",
                "");
            ok.to = Anchor::at(cur);
            ok.onBegin = [this, i]() { m_banner = "FOUND AT POSITION " + S(i) + "!"; };
            return;
        }
        prev = cur;
    }

    // not found
    m_failed = true;
    Step& w = push(StepKind::Walk, 0.9f,
        "current = NULL   (end of list)",
        "The last node's next pointer is nullptr. For a singly linked list, reaching NULL is "
        "the proof that there is nothing left to check.",
        "current = current->next;   // nullptr -> loop ends");
    w.from = Anchor::at(list.last());
    w.to   = Anchor::nullMarker();

    Step& f = push(StepKind::Fail, 3.4f,
        "NOT FOUND: " + S(value) + " does not exist in this list.",
        "All " + S(n) + " node(s) were compared and none matched. The search pointer reached "
        "NULL, so the value is definitely absent. Nothing in the list was changed.",
        "return nullptr;");
    f.to = Anchor::nullMarker();
    f.onBegin = [this]() { m_banner = "VALUE NOT FOUND"; };
}

// =================================================================== TRAVERSE
void Operation::planTraverse(LinkedList& list)
{
    const int n = list.count();
    m_title       = "TRAVERSE THE LIST";
    m_complexity  = "O(n)";
    m_outputLabel = "TRAVERSAL OUTPUT";

    if (n == 0)
    {
        m_failed = true;
        m_banner = "NOTHING TO TRAVERSE";
        Step& e = push(StepKind::Inspect, 1.5f,
            "head is nullptr - the list is empty.",
            "Traversal is the loop 'while (current != nullptr)'. When head is already nullptr "
            "the loop body never executes even once, so no values are printed.",
            "Node* current = head;   // nullptr");
        e.to = Anchor::headMarker();

        Step& f = push(StepKind::Fail, 3.0f,
            "TRAVERSAL FAILED: there are no nodes to visit.",
            "An empty list is drawn as HEAD -> NULL. Insert a node and traverse again to see "
            "the lights come on.",
            "while (current != nullptr) { cout << current->data; current = current->next; }");
        f.to = Anchor::nullMarker();
        return;
    }

    push(StepKind::Message, 1.1f,
         "current = head.  Visit every node until current becomes NULL.",
         "Traversal follows the real next pointers. The order on screen is the order in memory, "
         "not left-to-right guessing.",
         "Node* current = head;");

    Node* prev = nullptr;
    int i = 1;
    for (Node* cur = list.head(); cur != nullptr; cur = cur->next, i++)
    {
        Step& st = push(StepKind::Inspect, 0.65f,
            "step " + S(i) + ":  visiting node " + S(i) + ", data = " + S(cur->data),
            "The bulb lights up: this node is the current one. Its value is added to the output.",
            "cout << current->data;");
        st.from = (i == 1) ? Anchor::headMarker() : Anchor::at(prev);
        st.to   = Anchor::at(cur);
        Node* cn = cur;
        st.onBegin = [this, cn]()
        {
            m_output.push_back(cn->data);
            m_counter++;
            cn->vis.glow  = 1.0f;
            cn->vis.scale = 1.18f;
        };

        if (cur->next != nullptr)
        {
            Step& w = push(StepKind::Walk, 0.45f,
                "current = current->next",
                "A spark races along the real next pointer to the following node.",
                "current = current->next;");
            w.from = Anchor::at(cur);
            w.to   = Anchor::at(cur->next);
            Node* c2 = cur;
            w.onBegin = [c2]() { c2->vis.arrow = 1.0f; };
        }
        prev = cur;
    }

    Step& w = push(StepKind::Walk, 0.8f,
        "current = NULL   -> stop",
        "The final node's next pointer is nullptr, which ends the loop. That nullptr is what "
        "the NULL marker on screen represents.",
        "current = current->next;   // nullptr");
    w.from = Anchor::at(list.last());
    w.to   = Anchor::nullMarker();

    Step& ok = push(StepKind::Success, 2.0f,
        "TRAVERSAL COMPLETE!",
        "Visited " + S(n) + " node(s) in order. Time complexity: O(n).",
        "");
    ok.onBegin = [this]() { m_banner = "TRAVERSAL COMPLETE!"; };
}

// ====================================================================== COUNT
void Operation::planCount(LinkedList& list)
{
    const int n = list.count();
    m_title       = "COUNT THE NODES";
    m_complexity  = "O(n)";
    m_outputLabel = "COUNTER";

    push(StepKind::Message, 1.1f,
         "count = 0;  current = head;",
         "This list stores no size field, so the only honest way to know its length is to walk "
         "it and count. That is why counting is O(n), not O(1).",
         "int count = 0;\nNode* current = head;");

    if (n == 0)
    {
        Step& e = push(StepKind::Inspect, 1.4f,
            "head is nullptr, so the loop never runs.",
            "The counter stays at 0. An empty list is HEAD -> NULL - this is a valid answer, "
            "not an error.",
            "while (current != nullptr) { count++; current = current->next; }");
        e.to = Anchor::headMarker();

        Step& ok = push(StepKind::Success, 1.8f,
            "COUNT = 0",
            "The list is empty.",
            "return count;   // 0");
        ok.onBegin = [this]() { m_banner = "COUNT = 0"; };
        return;
    }

    Node* prev = nullptr;
    int i = 1;
    for (Node* cur = list.head(); cur != nullptr; cur = cur->next, i++)
    {
        Step& st = push(StepKind::Inspect, 0.55f,
            "count = " + S(i) + "   (standing on node " + S(i) + ")",
            "Every visited node increments the counter by one.",
            "count++;   current = current->next;");
        st.from = (i == 1) ? Anchor::headMarker() : Anchor::at(prev);
        st.to   = Anchor::at(cur);
        Node* cn = cur;
        st.onBegin = [this, cn]() { m_counter++; cn->vis.glow = 1.0f; };
        prev = cur;
    }

    Step& w = push(StepKind::Walk, 0.7f,
        "current = NULL   -> stop counting",
        "Reaching nullptr ends the loop.",
        "");
    w.from = Anchor::at(list.last());
    w.to   = Anchor::nullMarker();

    Step& ok = push(StepKind::Success, 1.8f,
        "COUNT = " + S(n),
        "The list contains " + S(n) + " node(s). Time complexity: O(n).",
        "return count;");
    ok.onBegin = [this, n]() { m_banner = "COUNT = " + S(n); };
}

// ====================================================================== CLEAR
void Operation::planClear(LinkedList& list)
{
    const int n = list.count();
    m_title      = "CLEAR THE LIST";
    m_complexity = "O(n)";

    if (n == 0)
    {
        m_failed = true;
        m_banner = "ALREADY EMPTY";
        Step& e = push(StepKind::Inspect, 1.4f,
            "head is already nullptr.",
            "There is no memory to free and no pointer to reset. An empty list is HEAD -> NULL.",
            "head == nullptr");
        e.to = Anchor::headMarker();

        Step& f = push(StepKind::Fail, 2.4f,
            "NOTHING TO CLEAR: the list is already empty.",
            "Clearing an empty list is harmless but pointless - the loop body never runs.",
            "while (head != nullptr) { ... }");
        f.to = Anchor::nullMarker();
        return;
    }

    push(StepKind::Message, 1.3f,
         "Delete every node, front to back, then set head = nullptr.",
         "Each iteration saves next BEFORE deleting the current node. If you delete first and "
         "then try to read current->next, you are reading freed memory - a classic bug.",
         "while (head != nullptr) {\n    Node* nxt = head->next;\n    delete head;\n    head = nxt;\n}");

    LinkedList* lp = &list;
    std::vector<Node*> nodes;
    for (Node* cur = list.head(); cur != nullptr; cur = cur->next) nodes.push_back(cur);

    for (size_t k = 0; k < nodes.size(); k++)
    {
        Node* t = nodes[k];
        Step& arm = push(StepKind::Arm, 0.22f,
            "Node " + S((int)k + 1) + " (value " + S(t->data) + ") is next.",
            "Save head->next, then free this node.",
            "Node* nxt = head->next;");
        arm.to = Anchor::at(t);
        arm.onBegin = [t]() { t->vis.danger = 1.0f; };

        Step& boom = push(StepKind::Explode, 0.3f,
            "BOOM!",
            "delete head;  head = nxt;",
            "delete head;\nhead = nxt;");
        boom.to = Anchor::at(t);

        Step& cm = push(StepKind::Commit, 0.12f, "", "", "");
        cm.onBegin = [lp]() { lp->removeFront(); };      // <-- REAL free
    }

    Step& ok = push(StepKind::Success, 2.0f,
        "LIST CLEARED!  head = nullptr",
        "All " + S(n) + " node(s) were freed and the head pointer was reset to nullptr. "
        "No leaks, no dangling pointers. The arena is empty: HEAD -> NULL.",
        "head = nullptr;");
    ok.onBegin = [this]() { m_banner = "LIST CLEARED!"; };
}
