#pragma once


#include <cstddef>

#include "buffer.h"


namespace siren {

class HeapNode;


class Heap final
{
public:
    typedef HeapNode Node;

    inline explicit Heap(bool (*)(const Node *, const Node *)) noexcept;
    inline Heap(Heap &&) noexcept;
    inline Heap &operator=(Heap &&) noexcept;

    inline void reset() noexcept;
    inline const Node *getTop() const noexcept;
    inline Node *getTop() noexcept;
    inline void addNode(Node *);
    inline void removeTop() noexcept;

    void removeNode(Node *) noexcept;

private:
    bool (*const nodeOrderer_)(const Node *, const Node *);
    Buffer<Node *> nodes_;
    std::size_t numberOfNodes_;

    inline void initialize() noexcept;
    inline void move(Heap *) noexcept;

    void siftUp(Node *, std::size_t) noexcept;
    void siftDown(Node *, std::size_t) noexcept;
};


class HeapNode
{
protected:
    inline explicit HeapNode() noexcept;

    ~HeapNode() = default;

private:
    std::size_t index_;

    HeapNode(const HeapNode &) = delete;
    HeapNode &operator=(const HeapNode &) = delete;

    friend Heap;
};

}


/*
 * #include "heap-inl.h"
 */


#include <cassert>
#include <utility>


namespace siren {

Heap::Heap(bool (*nodeOrderer)(const Node *, const Node *)) noexcept
  : nodeOrderer_(nodeOrderer)
{
    assert(nodeOrderer != nullptr);
    initialize();
}


Heap::Heap(Heap &&other) noexcept
  : nodeOrderer_(other.nodeOrderer_),
    nodes_(std::move(other.nodes_))
{
    other.move(this);
}


Heap &
Heap::operator=(Heap &&other) noexcept
{
    if (&other != this) {
        assert(nodeOrderer_ == other.nodeOrderer_);
        nodes_ = std::move(other.nodes_);
        other.move(this);
    }

    return *this;
}


void
Heap::initialize() noexcept
{
    numberOfNodes_ = 0;
}


void
Heap::move(Heap *other) noexcept
{
    other->numberOfNodes_ = numberOfNodes_;
    initialize();
}


void
Heap::reset() noexcept
{
    nodes_.reset();
    initialize();
}


const Heap::Node *
Heap::getTop() const noexcept
{
    return numberOfNodes_ == 0 ? nullptr : nodes_[0];
}


Heap::Node *
Heap::getTop() noexcept
{
    return numberOfNodes_ == 0 ? nullptr : nodes_[0];
}


void
Heap::addNode(Node *node)
{
    assert(node != nullptr);

    if (numberOfNodes_ == nodes_.getLength()) {
        nodes_.setLength(numberOfNodes_ + 1);
    }

    std::size_t i = numberOfNodes_++;
    siftUp(node, i);
}


void
Heap::removeTop() noexcept
{
    assert(numberOfNodes_ >= 1);

    if (--numberOfNodes_ >= 1) {
        Node *top = nodes_[numberOfNodes_];
        siftDown(top, 0);
    }
}


HeapNode::HeapNode() noexcept
{
}

}
