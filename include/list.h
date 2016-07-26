#pragma once


#define SIREN_LIST_FOR_EACH_NODE_REVERSE(LIST_NODE, LIST)              \
    for (auto LIST_NODE = (LIST).getTail(); !(LIST).isNil((LIST_NODE)) \
         ; (LIST_NODE) = (LIST_NODE)->getPrev())

#define SIREN_LIST_FOR_EACH_NODE(LIST_NODE, LIST)                      \
    for (auto LIST_NODE = (LIST).getHead(); !(LIST).isNil((LIST_NODE)) \
         ; (LIST_NODE) = (LIST_NODE)->getNext())

#define SIREN_LIST_FOR_EACH_NODE_SAFE_REVERSE(LIST_NODE, LIST)                 \
    for (auto LIST_NODE = (LIST).getTail(), sirenTemp = (LIST_NODE)->getPrev() \
         ; !(LIST).isNil((LIST_NODE)); (LIST_NODE) = sirenTemp, sirenTemp = (LIST_NODE)->getPrev())

#define SIREN_LIST_FOR_EACH_NODE_SAFE(LIST_NODE, LIST)                         \
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
    inline void replace(ListNode *) noexcept;
    inline void remove() noexcept;

protected:
    inline explicit ListNode() noexcept;
    inline ListNode(const ListNode &) noexcept;
    inline ListNode(ListNode &&) noexcept;
    inline ~ListNode();
    inline ListNode &operator=(const ListNode &) noexcept;
    inline ListNode &operator=(ListNode &&) noexcept;

private:
    ListNode *prev_;
    ListNode *next_;

    inline void initialize() noexcept;
#ifndef NDEBUG
    inline bool isUsed() const noexcept;
#endif
    inline void insert(ListNode *, ListNode *) noexcept;

    friend List;
};


class List final
{
public:
    typedef ListNode Node;

    inline explicit List() noexcept;
    inline List(List &&) noexcept;
    inline ~List();
    inline List &operator=(List &&) noexcept;

    inline void reset() noexcept;
    inline bool isEmpty() const noexcept;
    inline const Node *getTail() const noexcept;
    inline Node *getTail() noexcept;
    inline const Node *getHead() const noexcept;
    inline Node *getHead() noexcept;
    inline bool isNil(const Node *) const noexcept;
    inline void addTail(Node *) noexcept;
    inline void addHead(Node *) noexcept;

private:
    Node nil_;

    inline void initialize() noexcept;
    inline void finalize() noexcept;
    inline void move(List *) noexcept;

    List(const List &) = delete;
    List &operator=(const List &) = delete;
};

}


/*
 * #include "list-inl.h"
 */


#include <cassert>


namespace siren {

ListNode::ListNode() noexcept
{
    initialize();
}


ListNode::ListNode(const ListNode &dummy) noexcept
  : ListNode()
{
    static_cast<void>(dummy);
}


ListNode::ListNode(ListNode &&dummy) noexcept
  : ListNode()
{
    static_cast<void>(dummy);
}

ListNode::~ListNode()
{
    assert(!isUsed());
}


ListNode &
ListNode::operator=(const ListNode &dummy) noexcept
{
    static_cast<void>(dummy);
    return *this;
}


ListNode &
ListNode::operator=(ListNode &&dummy) noexcept
{
    static_cast<void>(dummy);
    return *this;
}


void
ListNode::initialize() noexcept
{
#ifndef NDEBUG
    prev_ = nullptr;
    next_ = nullptr;
#endif
}


#ifndef NDEBUG
bool
ListNode::isUsed() const noexcept
{
    return prev_ != nullptr;
}
#endif


bool
ListNode::isOnly() const noexcept
{
    assert(isUsed());
    return prev_ == next_;
}


const ListNode *
ListNode::getPrev() const noexcept
{
    assert(isUsed());
    return prev_;
}


ListNode *
ListNode::getPrev() noexcept
{
    assert(isUsed());
    return prev_;
}


const ListNode *
ListNode::getNext() const noexcept
{
    assert(isUsed());
    return next_;
}


ListNode *
ListNode::getNext() noexcept
{
    assert(isUsed());
    return next_;
}


void
ListNode::insertBefore(ListNode *other) noexcept
{
    assert(!isUsed());
    assert(other != nullptr);
    assert(other->isUsed());
    insert(other->prev_, other);
}


void
ListNode::insertAfter(ListNode *other) noexcept
{
    assert(!isUsed());
    assert(other != nullptr);
    assert(other->isUsed());
    insert(other, other->next_);
}


void
ListNode::replace(ListNode *other) noexcept
{
    assert(isUsed());
    assert(other != nullptr);
    assert(!other->isUsed());
    other->insert(prev_, next_);
    initialize();
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
    assert(isUsed());
    prev_->next_ = next_;
    next_->prev_ = prev_;
    initialize();
}


List::List() noexcept
{
    initialize();
}


List::List(List &&other) noexcept
{
    other.move(this);
}


List::~List()
{
    nil_.initialize();
}


List &
List::operator=(List &&other) noexcept
{
    if (&other != this) {
        finalize();
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
List::finalize() noexcept
{
#ifndef NDEBUG
    SIREN_LIST_FOR_EACH_NODE_SAFE_REVERSE(node, *this) {
        node->initialize();
    }
#endif
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
    finalize();
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
    return node == &nil_;
}


void
List::addTail(Node *node) noexcept
{
    assert(node != nullptr);
    assert(!node->isUsed());
    node->insert(getTail(), &nil_);
}


void
List::addHead(Node *node) noexcept
{
    assert(node != nullptr);
    assert(!node->isUsed());
    node->insert(&nil_, getHead());
}

}
