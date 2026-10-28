// ============================================================================
//  LinkedList.cpp  -  the real pointer surgery. No drawing code here.
// ============================================================================
#include "LinkedList.h"

int LinkedList::count() const
{
    int n = 0;
    for (Node* cur = m_head; cur != nullptr; cur = cur->next) n++;
    return n;
}

Node* LinkedList::nodeAt(int position) const
{
    if (position < 1) return nullptr;
    Node* cur = m_head;
    int i = 1;
    while (cur != nullptr && i < position) { cur = cur->next; i++; }
    return cur;                       // nullptr if we ran off the end
}

Node* LinkedList::last() const
{
    if (m_head == nullptr) return nullptr;
    Node* cur = m_head;
    while (cur->next != nullptr) cur = cur->next;
    return cur;
}

Node* LinkedList::find(int value, int* outPosition) const
{
    int pos = 1;
    for (Node* cur = m_head; cur != nullptr; cur = cur->next, pos++)
    {
        if (cur->data == value)
        {
            if (outPosition) *outPosition = pos;
            return cur;
        }
    }
    if (outPosition) *outPosition = -1;
    return nullptr;
}

// ---------------------------------------------------------------- insertion
void LinkedList::insertFront(Node* newNode)
{
    if (newNode == nullptr) return;
    newNode->next = m_head;     // 1. new node points at the old first node
    m_head        = newNode;    // 2. head now points at the new node
}

void LinkedList::insertAfter(Node* prev, Node* newNode)
{
    if (newNode == nullptr) return;
    if (prev == nullptr) { insertFront(newNode); return; }
    newNode->next = prev->next; // 1. new node takes over prev's successor
    prev->next    = newNode;    // 2. prev now points at the new node
}

void LinkedList::insertBack(Node* newNode)
{
    if (newNode == nullptr) return;
    newNode->next = nullptr;
    if (m_head == nullptr) { m_head = newNode; return; }
    Node* tail = last();        // O(n): no tail pointer in this list
    tail->next = newNode;
}

// ----------------------------------------------------------------- deletion
void LinkedList::removeFront()
{
    if (m_head == nullptr) return;          // nothing to delete
    Node* doomed = m_head;                  // keep a handle on it
    m_head       = m_head->next;            // unlink first
    delete doomed;                          // then free the memory
}

void LinkedList::removeAfter(Node* prev)
{
    if (prev == nullptr) { removeFront(); return; }
    Node* doomed = prev->next;
    if (doomed == nullptr) return;          // nothing after prev
    prev->next = doomed->next;              // bypass the doomed node
    delete doomed;                          // no dangling pointer remains
}

void LinkedList::clear()
{
    Node* cur = m_head;
    while (cur != nullptr)
    {
        Node* nxt = cur->next;              // save next BEFORE deleting
        delete cur;
        cur = nxt;
    }
    m_head = nullptr;
}
