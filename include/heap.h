#pragma once


#include <cstddef>

#include "buffer.h"


namespace siren {

class HeapNode;


class Heap final
{
public:
    typedef HeapNode Node;

    inline explicit Heap(bool (*)(const Node &, const Node &));
    inline Heap(Heap &&);
    inline Heap &operator=(Heap &&);

    inline const Node *getTop() const;
    inline Node *getTop();

    void insertNode(Node *);
    void removeNode(Node *);

private:
    bool (*const nodeOrderer_)(const Node &, const Node &);
    Buffer<Node *> nodes_;
    std::size_t numberOfNodes_;

    inline void finalize();
    inline void initialize();

    void siftUp(Node *, std::size_t);
    void siftDown(Node *, std::size_t);

    Heap(const Heap &) = delete;
    Heap &operator=(const Heap &) = delete;
};


class HeapNode
{
protected:
    inline explicit HeapNode();
    inline HeapNode(const HeapNode &);
    inline HeapNode(HeapNode &&);
    inline HeapNode &operator=(const HeapNode &);
    inline HeapNode &operator=(HeapNode &&);

    ~HeapNode() = default;

private:
    std::size_t index_;

#ifndef NDEBUG
    inline void initialize();
    inline bool isLinked() const;
    inline bool isUnlinked() const;
#endif

    friend Heap;
};

}


/*
 * #include "heap-inl.h"
 */


#include <cassert>
#include <utility>

#include "unsigned_to_signed.h"


namespace siren {

Heap::Heap(bool (*nodeOrderer)(const Node &, const Node &))
  : nodeOrderer_(nodeOrderer),
    numberOfNodes_(0)
{
    assert(nodeOrderer_ != nullptr);
}


Heap::Heap(Heap &&other)
  : nodeOrderer_(other.nodeOrderer_),
    nodes_(std::move(other.nodes_)),
    numberOfNodes_(other.numberOfNodes_)
{
    other.initialize();
}


Heap &
Heap::operator=(Heap &&other)
{
    if (&other != this) {
        finalize();
        nodes_ = std::move(other.nodes_);
        numberOfNodes_ = other.numberOfNodes_;
        other.initialize();
    }

    return *this;
}


void
Heap::finalize()
{
#ifndef NDEBUG
    for (std::size_t i = 0; i < numberOfNodes_; ++i) {
        nodes_[i]->initialize();
    }
#endif
}


void
Heap::initialize()
{
    numberOfNodes_ = 0;
}


const Heap::Node *
Heap::getTop() const
{
    return numberOfNodes_ == 0 ? nullptr : nodes_[0];
}


Heap::Node *
Heap::getTop()
{
    return numberOfNodes_ == 0 ? nullptr : nodes_[0];
}


HeapNode::HeapNode()
#ifndef NDEBUG
  : index_(-1)
#endif
{
}


HeapNode::HeapNode(const HeapNode &other)
  : HeapNode()
{
    static_cast<void>(other);
}


HeapNode::HeapNode(HeapNode &&other)
  : HeapNode()
{
    static_cast<void>(other);
}


HeapNode &
HeapNode::operator=(const HeapNode &other)
{
    static_cast<void>(other);
    return *this;
}


HeapNode &
HeapNode::operator=(HeapNode &&other)
{
    static_cast<void>(other);
    return *this;
}


#ifndef NDEBUG
void
HeapNode::initialize()
{
    index_ = -1;
}


bool
HeapNode::isLinked() const
{
    return UnsignedToSigned(index_) >= 0;
}


bool
HeapNode::isUnlinked() const
{
    return UnsignedToSigned(index_) < 0;
}
#endif

}
