#pragma once


#include <cstddef>

#include "buffer.h"


namespace siren {

class HeapNode;


class Heap final
{
public:
    typedef HeapNode Node;

    inline const Node *getTop() const noexcept;
    inline Node *getTop() noexcept;

    explicit Heap(bool (*)(const Node *, const Node *)) noexcept;
    Heap(Heap &&) noexcept;
    Heap &operator=(Heap &&) noexcept;

    void reset() noexcept;
    void insertNode(Node *);
    void removeNode(Node *) noexcept;
    void removeTop() noexcept;

private:
    bool (*const nodeOrderer_)(const Node *, const Node *);
    Buffer<Node *> nodes_;
    std::size_t nodeCount_;

    inline const Node *getNode(std::size_t) const noexcept;
    inline Node *getNode(std::size_t) noexcept;

    void initialize() noexcept;
    void move(Heap *) noexcept;
    std::size_t getMaxNumberOfNodes() const noexcept;
    void setMaxNumberOfNodes(std::size_t);
    void siftUp(Node *, std::size_t) noexcept;
    void siftDown(Node *, std::size_t) noexcept;
    void setNode(std::size_t, Node *) noexcept;
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

} // namespace siren


/*
 * #include "heap-inl.h"
 */


namespace siren {

const Heap::Node *
Heap::getTop() const noexcept
{
    return nodeCount_ == 0 ? nullptr : getNode(0);
}


Heap::Node *
Heap::getTop() noexcept
{
    return nodeCount_ == 0 ? nullptr : getNode(0);
}


const HeapNode *
Heap::getNode(std::size_t nodeIndex) const noexcept
{
    return nodes_[nodeIndex];
}


HeapNode *
Heap::getNode(std::size_t nodeIndex) noexcept
{
    return nodes_[nodeIndex];
}


HeapNode::HeapNode() noexcept
{
}

} // namespace siren
