#pragma once


#define SIREN_LIST_FOREACH_REVERSE(LIST_NODE, LIST)                    \
    for (auto LIST_NODE = (LIST).getTail(); !(LIST).isNil((LIST_NODE)) \
         ; (LIST_NODE) = (LIST_NODE)->getPrev())

#define SIREN_LIST_FOREACH(LIST_NODE, LIST)                            \
    for (auto LIST_NODE = (LIST).getHead(); !(LIST).isNil((LIST_NODE)) \
         ; (LIST_NODE) = (LIST_NODE)->getNext())

#define SIREN_LIST_FOREACH_SAFE_REVERSE(LIST_NODE, LIST)                       \
    for (auto LIST_NODE = (LIST).getTail(), sirenTemp = (LIST_NODE)->getPrev() \
         ; !(LIST).isNil((LIST_NODE)); (LIST_NODE) = sirenTemp, sirenTemp = (LIST_NODE)->getPrev())

#define SIREN_LIST_FOREACH_SAFE(LIST_NODE, LIST)                               \
    for (auto LIST_NODE = (LIST).getHead(), sirenTemp = (LIST_NODE)->getNext() \
         ; !(LIST).isNil((LIST_NODE)); (LIST_NODE) = sirenTemp, sirenTemp = (LIST_NODE)->getNext())


namespace siren {

class List;


class ListNode
{
public:
    inline bool isOnly() const noexcept;
    inline const ListNode *getPrev() const noexcept;
    inline ListNode *getPrev() noexcept;
    inline const ListNode *getNext() const noexcept;
    inline ListNode *getNext() noexcept;
    inline void insertBefore(ListNode *) noexcept;
    inline void insertAfter(ListNode *) noexcept;
    inline void remove() noexcept;

protected:
    inline explicit ListNode() noexcept;

    ~ListNode() = default;

private:
    ListNode *prev_;
    ListNode *next_;

    inline void insert(ListNode *, ListNode *) noexcept;

    ListNode(const ListNode &) = delete;
    ListNode &operator=(const ListNode &) = delete;

    friend List;
};


class List final
{
public:
    typedef ListNode Node;

    inline static void InsertNodesBefore(Node *, Node *, Node *) noexcept;
    inline static void InsertNodesAfter(Node *, Node *, Node *) noexcept;
    inline static void RemoveNodes(Node *, Node *) noexcept;

    inline explicit List() noexcept;
    inline List(List &&) noexcept;
    inline List &operator=(List &&) noexcept;

    inline void reset() noexcept;
    inline bool isEmpty() const noexcept;
    inline const Node *getTail() const noexcept;
    inline Node *getTail() noexcept;
    inline const Node *getHead() const noexcept;
    inline Node *getHead() noexcept;
    inline bool isNil(const Node *) const noexcept;
    inline void appendNode(Node *) noexcept;
    inline void prependNode(Node *) noexcept;
    inline void appendNodes(Node *, Node *) noexcept;
    inline void prependNodes(Node *, Node *) noexcept;
    inline void append(List *) noexcept;
    inline void prepend(List *) noexcept;

    void sort(bool (*)(const Node *, const Node *)) noexcept;

private:
    Node nil_;

    inline static void InsertNodes(Node *, Node *, Node *, Node *) noexcept;

    inline void initialize() noexcept;
    inline void move(List *) noexcept;

    void SortNodes(Node **, Node **, bool (*)(const Node *, const Node *)) noexcept;
};


constexpr auto InsertListNodesBefore = List::InsertNodesBefore;
constexpr auto InsertListNodesAfter = List::InsertNodesAfter;
constexpr auto RemoveListNodes = List::RemoveNodes;

}


/*
 * #include "list-inl.h"
 */


#include <cassert>


namespace siren {

ListNode::ListNode() noexcept
{
}


bool
ListNode::isOnly() const noexcept
{
    return prev_ == next_;
}


const ListNode *
ListNode::getPrev() const noexcept
{
    return prev_;
}


ListNode *
ListNode::getPrev() noexcept
{
    return prev_;
}


const ListNode *
ListNode::getNext() const noexcept
{
    return next_;
}


ListNode *
ListNode::getNext() noexcept
{
    return next_;
}


void
ListNode::insertBefore(ListNode *other) noexcept
{
    assert(other != nullptr);
    insert(other->prev_, other);
}


void
ListNode::insertAfter(ListNode *other) noexcept
{
    assert(other != nullptr);
    insert(other, other->next_);
}


void
ListNode::insert(ListNode *prev, ListNode *next) noexcept
{
    (prev_ = prev)->next_ = this;
    (next_ = next)->prev_ = this;
}


void
ListNode::remove() noexcept
{
    prev_->next_ = next_;
    next_->prev_ = prev_;
}


void
List::InsertNodesBefore(Node *firstNode, Node *lastNode, Node *node) noexcept
{
    assert(firstNode != nullptr);
    assert(lastNode != nullptr);
    assert(node != nullptr);
    InsertNodes(firstNode, lastNode, node->prev_, node);
}


void
List::InsertNodesAfter(Node *firstNode, Node *lastNode, Node *node) noexcept
{
    assert(firstNode != nullptr);
    assert(lastNode != nullptr);
    assert(node != nullptr);
    InsertNodes(firstNode, lastNode, node, node->next_);
}


void
List::InsertNodes(Node *firstNode, Node *lastNode, Node *firstNodePrev, Node *lastNodeNext) noexcept
{
    (firstNode->prev_ = firstNodePrev)->next_ = firstNode;
    (lastNode->next_ = lastNodeNext)->prev_ = lastNode;
}


void
List::RemoveNodes(Node *firstNode, Node *lastNode) noexcept
{
    assert(firstNode != nullptr);
    assert(lastNode != nullptr);
    Node *firstNodePrev = firstNode->prev_;
    Node *lastNodeNext = lastNode->next_;
    firstNodePrev->next_ = lastNodeNext;
    lastNodeNext->prev_ = firstNodePrev;
}


List::List() noexcept
{
    initialize();
}


List::List(List &&other) noexcept
{
    other.move(this);
}


List &
List::operator=(List &&other) noexcept
{
    if (&other != this) {
        other.move(this);
    }

    return *this;
}


void
List::initialize() noexcept
{
    nil_.prev_ = &nil_;
    nil_.next_ = &nil_;
}


void
List::move(List *other) noexcept
{
    if (isEmpty()) {
        other->initialize();
    } else {
        other->nil_.insert(nil_.prev_, nil_.next_);
        initialize();
    }
}


void
List::reset() noexcept
{
    initialize();
}


bool
List::isEmpty() const noexcept
{
    return nil_.prev_ == &nil_;
}


const List::Node *
List::getTail() const noexcept
{
    return nil_.prev_;
}


List::Node *
List::getTail() noexcept
{
    return nil_.prev_;
}


const List::Node *
List::getHead() const noexcept
{
    return nil_.next_;
}


List::Node *
List::getHead() noexcept
{
    return nil_.next_;
}


bool
List::isNil(const Node *node) const noexcept
{
    assert(node != nullptr);
    return node == &nil_;
}


void
List::appendNode(Node *node) noexcept
{
    assert(node != nullptr);
    node->insert(getTail(), &nil_);
}


void
List::prependNode(Node *node) noexcept
{
    assert(node != nullptr);
    node->insert(&nil_, getHead());
}


void
List::appendNodes(Node *firstNode, Node *lastNode) noexcept
{
    assert(firstNode != nullptr);
    assert(lastNode != nullptr);
    InsertNodes(firstNode, lastNode, getTail(), &nil_);
}


void
List::prependNodes(Node *firstNode, Node *lastNode) noexcept
{
    assert(firstNode != nullptr);
    assert(lastNode != nullptr);
    InsertNodes(firstNode, lastNode, &nil_, getHead());
}


void
List::append(List *other) noexcept
{
    assert(other != nullptr);
    assert(other != this);

    if (!isEmpty()) {
        InsertNodes(getHead(), getTail(), other->getTail(), &other->nil_);
        initialize();
    }
}


void
List::prepend(List *other) noexcept
{
    assert(other != nullptr);
    assert(other != this);

    if (!isEmpty()) {
        InsertNodes(getHead(), getTail(), &other->nil_, other->getHead());
        initialize();
    }
}

}
