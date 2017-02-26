#pragma once


#include <cstddef>

#include "buffer.h"


namespace siren {

class HeapNode;


class Heap final
{
public:
    typedef HeapNode Node;

    inline bool isEmpty() const noexcept;
    inline const Node *getTop() const noexcept;
    inline Node *getTop() noexcept;

    template <class T>
    inline void traverse(T &&) const;

    template <class T>
    inline void traverse(T &&);

    explicit Heap(bool (*)(const Node *, const Node *)) noexcept;
    Heap(Heap &&) noexcept;
    Heap &operator=(Heap &&) noexcept;

    void reset() noexcept;
    void insertNode(Node *);
    void removeNode(Node *) noexcept;
    void removeTop() noexcept;

private:
    bool (*const nodeOrderer_)(const Node *, const Node *);
    Buffer<Node *> slots_;
    std::size_t nodeCount_;

    inline const Node *getSlot(std::size_t) const noexcept;
    inline Node *getSlot(std::size_t) noexcept;

    void initialize() noexcept;
    void move(Heap *) noexcept;
    std::size_t getNumberOfSlots() const noexcept;
    void setNumberOfSlots(std::size_t);
    void siftUp(Node *, std::size_t) noexcept;
    void siftDown(Node *, std::size_t) noexcept;
    void setSlot(std::size_t, Node *) noexcept;
};


class HeapNode
{
protected:
    inline explicit HeapNode() noexcept;

    ~HeapNode() = default;

private:
    std::size_t slotIndex_;

    std::size_t getSlotIndex() const noexcept;

    HeapNode(const HeapNode &) = delete;
    HeapNode &operator=(const HeapNode &) = delete;

    friend Heap;
};

} // namespace siren


/*
 * #include "heap-inl.h"
 */


#include "assert.h"


namespace siren {

bool
Heap::isEmpty() const noexcept
{
    return nodeCount_ == 0;
}


const Heap::Node *
Heap::getTop() const noexcept
{
    SIREN_ASSERT(!isEmpty());
    return getSlot(0);
}


Heap::Node *
Heap::getTop() noexcept
{
    SIREN_ASSERT(!isEmpty());
    return getSlot(0);
}


const HeapNode *
Heap::getSlot(std::size_t slotIndex) const noexcept
{
    return slots_[slotIndex];
}


HeapNode *
Heap::getSlot(std::size_t slotIndex) noexcept
{
    return slots_[slotIndex];
}


template <class T>
void
Heap::traverse(T &&callback) const
{
    for (std::size_t i = 0; i < nodeCount_; ++i) {
        const Node *x = getSlot(i);
        callback(x);
    }
}


template <class T>
void
Heap::traverse(T &&callback)
{
    for (std::size_t i = 0; i < nodeCount_; ++i) {
        Node *x = getSlot(i);
        callback(x);
    }
}


HeapNode::HeapNode() noexcept
{
}

} // namespace siren
