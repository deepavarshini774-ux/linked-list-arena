// ============================================================================
//  LinkedList.h  -  A REAL singly linked list (nodes + pointers on the heap).
//
//  Nothing in this file knows anything about drawing. The only concession to
//  the visualizer is the small NodeVisual struct carried inside each node,
//  which stores *where the box currently is on screen*. The list operations
//  below never read or write those fields - only Visualizer.cpp does.
// ============================================================================
#ifndef LINKEDLIST_H
#define LINKEDLIST_H

#include "raylib.h"

// Purely cosmetic per-node state (position on screen, glow amount, ...).
struct NodeVisual
{
    Vector2 pos    = { 0.0f, 0.0f };   // current drawn position (centre of box)
    Vector2 target = { 0.0f, 0.0f };   // where the layout wants it to be
    bool    placed = false;            // false => snap to target on first frame
    float   scale  = 1.0f;             // 1.0 = normal size (pop effect)
    float   glow   = 0.0f;             // 0..1 cyan/green highlight
    float   danger = 0.0f;             // 0..1 red "about to be deleted" pulse
    float   arrow  = 0.0f;             // 0..1 highlight of this node's next-arrow
};

// The node itself: data + next pointer, exactly like a textbook linked list.
struct Node
{
    int        data;
    Node*      next;
    NodeVisual vis;

    explicit Node(int value) : data(value), next(nullptr) {}
};

// ----------------------------------------------------------------------------
// The list. Every method here manipulates genuine pointers.
// Positions in the public API are 1-based, matching what the UI shows.
// ----------------------------------------------------------------------------
class LinkedList
{
public:
    LinkedList() = default;
    ~LinkedList() { clear(); }                       // never leak on shutdown

    LinkedList(const LinkedList&)            = delete;   // no accidental copies
    LinkedList& operator=(const LinkedList&) = delete;

    // ---- queries -----------------------------------------------------------
    Node* head()  const { return m_head; }
    bool  empty() const { return m_head == nullptr; }

    int   count() const;                       // walks the list, O(n)
    Node* nodeAt(int position) const;          // 1-based, nullptr if out of range
    Node* last() const;                        // nullptr when empty
    Node* find(int value, int* outPosition) const;  // first match, O(n)

    // ---- modifiers ---------------------------------------------------------
    // These assume the caller already validated the operation (Operation.cpp
    // does that and explains failures to the user instead of calling these).
    void insertFront(Node* newNode);
    void insertAfter(Node* prev, Node* newNode);
    void insertBack(Node* newNode);

    void removeFront();                // frees the node
    void removeAfter(Node* prev);      // frees prev->next
    void clear();                      // frees everything, head = nullptr

private:
    Node* m_head = nullptr;            // the one and only genuine head pointer
};

#endif // LINKEDLIST_H
