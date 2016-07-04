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
    inline const ListNode *getPrev() const;
    inline ListNode *getPrev();
    inline const ListNode *getNext() const;
    inline ListNode *getNext();
    inline void insertBefore(ListNode *);
    inline void insertAfter(ListNode *);
    inline void replace(ListNode *);
    inline void remove();

protected:
    inline explicit ListNode();
    inline ListNode(const ListNode &);
    inline ListNode(ListNode &&);
    inline ListNode &operator=(const ListNode &);
    inline ListNode &operator=(ListNode &&);

    ~ListNode() = default;

private:
    ListNode *prev_;
    ListNode *next_;

#ifndef NDEBUG
    inline bool isLinked() const;
    inline bool isUnlinked() const;
#endif
    inline void insert(ListNode *, ListNode *);

    friend List;
};


class List final
{
public:
    typedef ListNode Node;

    inline explicit List();

    inline bool isEmpty() const;
    inline const Node *getTail() const;
    inline Node *getTail();
    inline const Node *getHead() const;
    inline Node *getHead();
    inline bool isNil(const Node *) const;
    inline void insertTail(Node *);
    inline void insertHead(Node *);

private:
    Node nil_;

    List(const List &) = delete;
    List &operator=(const List &) = delete;
};

}


/*
 * #include "list-inl.h"
 */


#include <cassert>


namespace siren {

ListNode::ListNode()
#ifndef NDEBUG
  : prev_(nullptr),
    next_(nullptr)
#endif
{
}


ListNode::ListNode(const ListNode &other)
  : ListNode()
{
    static_cast<void>(other);
}


ListNode::ListNode(ListNode &&other)
  : ListNode()
{
    static_cast<void>(other);
}


ListNode &
ListNode::operator=(const ListNode &other)
{
    static_cast<void>(other);
    return *this;
}


ListNode &
ListNode::operator=(ListNode &&other)
{
    static_cast<void>(other);
    return *this;
}


#ifndef NDEBUG
bool
ListNode::isLinked() const
{
    return prev_ != nullptr && next_ != nullptr;
}


bool
ListNode::isUnlinked() const
{
    return prev_ == nullptr && next_ == nullptr;
}
#endif


const ListNode *
ListNode::getPrev() const
{
    assert(isLinked());
    return prev_;
}


ListNode *
ListNode::getPrev()
{
    assert(isLinked());
    return prev_;
}


const ListNode *
ListNode::getNext() const
{
    assert(isLinked());
    return next_;
}


ListNode *
ListNode::getNext()
{
    assert(isLinked());
    return next_;
}


void
ListNode::insertBefore(ListNode *other)
{
    assert(isUnlinked());
    assert(other != nullptr);
    assert(other->isLinked());
    insert(other->prev_, other);
}


void
ListNode::insertAfter(ListNode *other)
{
    assert(isUnlinked());
    assert(other != nullptr);
    assert(other->isLinked());
    insert(other, other->next_);
}


void
ListNode::replace(ListNode *other)
{
    assert(isLinked());
    assert(other != nullptr);
    assert(other->isUnlinked());
    other->insert(prev_, next_);
#ifndef NDEBUG
    prev_ = nullptr;
    next_ = nullptr;
#endif
}


void
ListNode::insert(ListNode *prev, ListNode *next)
{
    (prev_ = prev)->next_ = this;
    (next_ = next)->prev_ = this;
}


void
ListNode::remove()
{
    assert(isLinked());
    prev_->next_ = next_;
    next_->prev_ = prev_;
#ifndef NDEBUG
    prev_ = nullptr;
    next_ = nullptr;
#endif
}


List::List()
{
    nil_.prev_ = &nil_;
    nil_.next_ = &nil_;
}


bool
List::isEmpty() const
{
    return nil_.prev_ == &nil_;
}


const List::Node *
List::getTail() const
{
    return nil_.prev_;
}


List::Node *
List::getTail()
{
    return nil_.prev_;
}


const List::Node *
List::getHead() const
{
    return nil_.next_;
}


List::Node *
List::getHead()
{
    return nil_.next_;
}


bool
List::isNil(const Node *node) const
{
    return node == &nil_;
}


void
List::insertTail(Node *node)
{
    assert(node != nullptr);
    assert(node->isUnlinked());
    node->insert(getTail(), &nil_);
}


void
List::insertHead(Node *node)
{
    assert(node != nullptr);
    assert(node->isUnlinked());
    node->insert(&nil_, getHead());
}

}
