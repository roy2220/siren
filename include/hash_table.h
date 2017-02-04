#pragma once


#include <cstddef>

#include "buffer.h"


namespace siren {

class HashTableNode;


class HashTable final
{
public:
    typedef HashTableNode Node;

    inline bool isEmpty() const noexcept;

    template <class T>
    inline const Node *search(std::size_t, T &&) const;

    template <class T>
    inline Node *search(std::size_t, T &&);

    template <class T>
    inline void traverse(T &&) const;

    template <class T>
    inline void traverse(T &&);

    explicit HashTable();
    HashTable(HashTable &&) noexcept;
    HashTable &operator=(HashTable &&) noexcept;

    void reset() noexcept;
    void insertNode(Node *, std::size_t);
    void removeNode(Node *) noexcept;

private:
    Buffer<Node *> slots_;
    std::size_t nodeCount_;
    std::size_t slotIndexMasks_[2];

    inline Node *const *locateSlot(std::size_t) const noexcept;
    inline Node **locateSlot(std::size_t) noexcept;

    void initialize();
    void move(HashTable *) noexcept;
    std::size_t getNumberOfSlots() const noexcept;
    void setNumberOfSlots(std::size_t);
    void splitSlot(std::size_t, std::size_t, std::size_t) noexcept;
    void mergeSlot(std::size_t, std::size_t) noexcept;
    std::size_t computeSlotIndex(std::size_t) const noexcept;
};


class HashTableNode
{
public:
    inline const HashTableNode *getPrev() const noexcept;
    inline HashTableNode *getPrev() noexcept;

protected:
    inline explicit HashTableNode() noexcept;

    ~HashTableNode() = default;

private:
    HashTableNode *prev_;
    std::size_t digest_;

    HashTableNode(const HashTableNode &) = delete;
    HashTableNode &operator=(const HashTableNode &) = delete;

    friend HashTable;
};

} // namespace siren


/*
 * #include "hash_table-inl.h"
 */


namespace siren {

bool
HashTable::isEmpty() const noexcept
{
    return nodeCount_ == 1;
}


HashTableNode *const *
HashTable::locateSlot(std::size_t slotIndex) const noexcept
{
    return &slots_[slotIndex];
}


HashTableNode **
HashTable::locateSlot(std::size_t slotIndex) noexcept
{
    return &slots_[slotIndex];
}


template <class T>
const HashTableNode *
HashTable::search(std::size_t nodeDigest, T &&nodeMatcher) const
{
    Node *const *slot = locateSlot(computeSlotIndex(nodeDigest));

    for (const Node *node = *slot; node != nullptr; node = node->prev_) {
        if (nodeMatcher(node)) {
            return node;
        }
    }

    return nullptr;
}


template <class T>
HashTableNode *
HashTable::search(std::size_t nodeDigest, T &&nodeMatcher)
{
    Node **slot = locateSlot(computeSlotIndex(nodeDigest));

    for (Node *node = *slot; node != nullptr; node = node->prev_) {
        if (nodeMatcher(node)) {
            return node;
        }
    }

    return nullptr;
}


template <class T>
void
HashTable::traverse(T &&callback) const
{
    for (std::size_t i = 0; i < nodeCount_; ++i) {
        Node *const *slot = locateSlot(i);

        for (const Node *node = *slot; node != nullptr; node = node->prev_) {
            callback(node);
        }
    }
}


template <class T>
void
HashTable::traverse(T &&callback)
{
    for (std::size_t i = 0; i < nodeCount_; ++i) {
        Node **slot = locateSlot(i);

        for (Node *node = *slot; node != nullptr; node = node->prev_) {
            callback(node);
        }
    }
}


HashTableNode::HashTableNode() noexcept
{
}


const HashTableNode *
HashTableNode::getPrev() const noexcept
{
    return prev_;
}


HashTableNode *
HashTableNode::getPrev() noexcept
{
    return prev_;
}

} // namespace siren
