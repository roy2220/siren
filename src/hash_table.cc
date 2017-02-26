#include "hash_table.h"

#include <utility>

#include "assert.h"


namespace siren {

HashTable::HashTable()
{
    initialize();
}


HashTable::HashTable(HashTable &&other) noexcept
  : slots_(std::move(other.slots_))
{
    other.move(this);
}


HashTable &
HashTable::operator=(HashTable &&other) noexcept
{
    if (&other != this) {
        slots_ = std::move(other.slots_);
        other.move(this);
    }

    return *this;
}


void
HashTable::initialize()
{
    setNumberOfSlots(1);
    *locateSlot(0) = nullptr;
    nodeCount_ = 1;
    slotIndexMasks_[0] = 0;
}


void
HashTable::move(HashTable *other) noexcept
{
    other->nodeCount_ = nodeCount_;
    other->slotIndexMasks_[0] = slotIndexMasks_[0];

    if (slotIndexMasks_[0] != 0) {
        other->slotIndexMasks_[1] = slotIndexMasks_[1];
    }

    initialize();
}


void
HashTable::reset() noexcept
{
    initialize();
}


void
HashTable::insertNode(Node *node, std::size_t nodeDigest)
{
    SIREN_ASSERT(node != nullptr);

    if (nodeCount_ == getNumberOfSlots()) {
        setNumberOfSlots(nodeCount_ + 1);
    }

    if (nodeCount_ - 1 == slotIndexMasks_[0]) {
        slotIndexMasks_[1] = slotIndexMasks_[0];
        slotIndexMasks_[0] = (slotIndexMasks_[0] << 1) | 1;
    }

    {
        std::size_t targetSlotIndex = nodeCount_++;
        std::size_t sourceSlotIndex = targetSlotIndex & slotIndexMasks_[1];
        splitSlot(sourceSlotIndex, targetSlotIndex, slotIndexMasks_[0]);
    }

    {
        node->digest_ = nodeDigest;
        Node **slot = locateSlot(computeSlotIndex(nodeDigest));
        node->prev_ = *slot;
        *slot = node;
    }
}


void
HashTable::removeNode(Node *node) noexcept
{
    SIREN_ASSERT(!isEmpty());
    SIREN_ASSERT(node != nullptr);

    {
        Node **slot = locateSlot(computeSlotIndex(node->digest_));

        while (*slot != node) {
            slot = &(*slot)->prev_;
        }

        *slot = node->prev_;
    }

    {
        std::size_t sourceSlotIndex = --nodeCount_;
        std::size_t targetSlotIndex = sourceSlotIndex & slotIndexMasks_[1];
        mergeSlot(sourceSlotIndex, targetSlotIndex);
    }

    if (nodeCount_ - 1 == slotIndexMasks_[1]) {
        slotIndexMasks_[0] = slotIndexMasks_[1];
        slotIndexMasks_[1] >>= 1;
    }
}


std::size_t
HashTable::getNumberOfSlots() const noexcept
{
    return slots_.getLength();
}


void
HashTable::setNumberOfSlots(std::size_t numberOfSlots)
{
    slots_.setLength(numberOfSlots);
}


std::size_t
HashTable::computeSlotIndex(std::size_t nodeDigest) const noexcept
{
    std::size_t slotIndex = nodeDigest & slotIndexMasks_[0];

    if (slotIndex >= nodeCount_) {
        slotIndex &= slotIndexMasks_[1];
    }

    return slotIndex;
}


void
HashTable::splitSlot(std::size_t sourceSlotIndex, std::size_t targetSlotIndex
                     , std::size_t slotIndexMask) noexcept
{
    Node **sourceSlot = locateSlot(sourceSlotIndex);
    Node **targetSlot = locateSlot(targetSlotIndex);

    for (;;) {
        Node *node = *sourceSlot;

        if (node == nullptr) {
            break;
        } else {
            if ((node->digest_ & slotIndexMask) == sourceSlotIndex) {
                sourceSlot = &node->prev_;
            } else {
                *sourceSlot = node->prev_;
                *targetSlot = node;
                targetSlot = &node->prev_;
            }
        }
    }

    *targetSlot = nullptr;
}


void
HashTable::mergeSlot(std::size_t sourceSlotIndex, std::size_t targetSlotIndex) noexcept
{
    Node **sourceSlot = locateSlot(sourceSlotIndex);
    Node **targetSlot = locateSlot(targetSlotIndex);

    while (*targetSlot != nullptr) {
        targetSlot = &(*targetSlot)->prev_;
    }

    *targetSlot = *sourceSlot;
}

} // namespace siren
