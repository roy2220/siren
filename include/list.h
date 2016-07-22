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
    inline ListNode &operator=(const ListNode &) noexcept;
    inline ListNode &operator=(ListNode &&) noexcept;

    ~ListNode() = default;

private:
    ListNode *prev_;
    ListNode *next_;

    inline void initialize() noexcept;
#ifndef NDEBUG
    inline bool isLinked() const noexcept;
    inline bool isUnlinked() const noexcept;
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
    inline void insertTail(Node *) noexcept;
    inline void insertHead(Node *) noexcept;

private:
    Node nil_;

    inline void finalize() noexcept;
    inline void initialize() noexcept;

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
#ifndef NDEBUG
  : prev_(nullptr),
    next_(nullptr)
#endif
{
}


ListNode::ListNode(const ListNode &other) noexcept
  : ListNode()
{
    static_cast<void>(other);
}


ListNode::ListNode(ListNode &&other) noexcept
  : ListNode()
{
    static_cast<void>(other);
}


ListNode &
ListNode::operator=(const ListNode &other) noexcept
{
    static_cast<void>(other);
    return *this;
}


ListNode &
ListNode::operator=(ListNode &&other) noexcept
{
    static_cast<void>(other);
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
ListNode::isLinked() const noexcept
{
    return prev_ != nullptr && next_ != nullptr;
}


bool
ListNode::isUnlinked() const noexcept
{
    return prev_ == nullptr && next_ == nullptr;
}
#endif


const ListNode *
ListNode::getPrev() const noexcept
{
    assert(isLinked());
    return prev_;
}


ListNode *
ListNode::getPrev() noexcept
{
    assert(isLinked());
    return prev_;
}


const ListNode *
ListNode::getNext() const noexcept
{
    assert(isLinked());
    return next_;
}


ListNode *
ListNode::getNext() noexcept
{
    assert(isLinked());
    return next_;
}


void
ListNode::insertBefore(ListNode *other) noexcept
{
    assert(isUnlinked());
    assert(other != nullptr);
    assert(other->isLinked());
    insert(other->prev_, other);
}


void
ListNode::insertAfter(ListNode *other) noexcept
{
    assert(isUnlinked());
    assert(other != nullptr);
    assert(other->isLinked());
    insert(other, other->next_);
}


void
ListNode::replace(ListNode *other) noexcept
{
    assert(isLinked());
    assert(other != nullptr);
    assert(other->isUnlinked());
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
    assert(isLinked());
    prev_->next_ = next_;
    next_->prev_ = prev_;
    initialize();
}


List::List() noexcept
{
    nil_.prev_ = &nil_;
    nil_.next_ = &nil_;
}


List::List(List &&other) noexcept
{
    if (other.isEmpty()) {
        initialize();
    } else {
        nil_.insert(other.nil_.prev_, other.nil_.next_);
        other.initialize();
    }
}


List::~List()
{
    finalize();
}


List &
List::operator=(List &&other) noexcept
{
    if (&other != this) {
        finalize();

        if (other.isEmpty()) {
            initialize();
        } else {
            nil_.insert(other.nil_.prev_, other.nil_.next_);
            other.initialize();
        }
    }

    return *this;
}


void
List::reset() noexcept
{
    finalize();
    initialize();
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
List::initialize() noexcept
{
    nil_.prev_ = &nil_;
    nil_.next_ = &nil_;
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
List::insertTail(Node *node) noexcept
{
    assert(node != nullptr);
    assert(node->isUnlinked());
    node->insert(getTail(), &nil_);
}


void
List::insertHead(Node *node) noexcept
{
    assert(node != nullptr);
    assert(node->isUnlinked());
    node->insert(&nil_, getHead());
}

}
