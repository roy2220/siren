#include "heap.h"

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
    slots_(std::move(other.slots_))
{
    other.move(this);
}


Heap &
Heap::operator=(Heap &&other) noexcept
{
    if (&other != this) {
        assert(nodeOrderer_ == other.nodeOrderer_);
        slots_ = std::move(other.slots_);
        other.move(this);
    }

    return *this;
}


void
Heap::initialize() noexcept
{
    nodeCount_ = 0;
}


void
Heap::move(Heap *other) noexcept
{
    other->nodeCount_ = nodeCount_;
    initialize();
}


void
Heap::reset() noexcept
{
    slots_.reset();
    initialize();
}


void
Heap::insertNode(Node *node)
{
    assert(node != nullptr);

    if (nodeCount_ == getNumberOfSlots()) {
        setNumberOfSlots(nodeCount_ + 1);
    }

    std::size_t slotIndex = nodeCount_++;
    siftUp(node, slotIndex);
}


void
Heap::removeNode(Node *node1) noexcept
{
    assert(!isEmpty());
    assert(node1 != nullptr);
    std::size_t slot1Index = node1->getSlotIndex();
    std::size_t slot2Index = --nodeCount_;

    if (slot2Index > slot1Index) {
        Node *node2 = getSlot(slot2Index);

        if (nodeOrderer_(node2, node1)) {
            siftUp(node2, slot1Index);
        } else {
            siftDown(node2, slot1Index);
        }
    }
}


void
Heap::removeTop() noexcept
{
    assert(!isEmpty());
    std::size_t slotIndex = --nodeCount_;

    if (slotIndex > 0) {
        Node *node = getSlot(slotIndex);
        siftDown(node, 0);
    }
}


std::size_t
Heap::getNumberOfSlots() const noexcept
{
    return slots_.getLength();
}


void
Heap::setNumberOfSlots(std::size_t numberOfSlots)
{
    slots_.setLength(numberOfSlots);
}


void
Heap::siftUp(Node *x, std::size_t i) noexcept
{
    while (i >= 1) {
        std::size_t j = (i - 1) / 2;
        Node *y = getSlot(j);

        if (nodeOrderer_(y, x)) {
            break;
        } else {
            setSlot(i, y);
            i = j;
        }
    }

    setSlot(i, x);
}


void
Heap::siftDown(Node *x, std::size_t i) noexcept
{
    for (;;) {
        std::size_t j;

        {
            std::size_t j1 = (2 * i) + 1;

            if (j1 < nodeCount_) {
                std::size_t j2 = j1 + 1;

                if (j2 < nodeCount_) {
                    j = nodeOrderer_(getSlot(j1), getSlot(j2)) ? j1 : j2;
                } else {
                    j = j1;
                }
            } else {
                break;
            }
        }

        Node *y = getSlot(j);

        if (nodeOrderer_(x, y)) {
            break;
        } else {
            setSlot(i, y);
            i = j;
        }
    }

    setSlot(i, x);
}


void
Heap::setSlot(std::size_t slotIndex, Node *node) noexcept
{
    (slots_[slotIndex] = node)->slotIndex_ = slotIndex;
}


std::size_t
HeapNode::getSlotIndex() const noexcept
{
    return slotIndex_;
}

} // namespace siren
