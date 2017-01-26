#include "heap.h"

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
    nodes_.reset();
    initialize();
}


void
Heap::addNode(Node *node)
{
    assert(node != nullptr);

    if (nodeCount_ == getMaxNumberOfNodes()) {
        setMaxNumberOfNodes(nodeCount_ + 1);
    }

    std::size_t nodeIndex = nodeCount_++;
    siftUp(node, nodeIndex);
}


void
Heap::removeNode(Node *node) noexcept
{
    assert(node != nullptr);
    assert(node->index_ < nodeCount_ && node == getNode(node->index_));
    std::size_t lastNodeIndex = --nodeCount_;

    if (node->index_ < lastNodeIndex) {
        Node *lastNode = getNode(lastNodeIndex);

        if (nodeOrderer_(lastNode, node)) {
            siftUp(lastNode, node->index_);
        } else {
            siftDown(lastNode, node->index_);
        }
    }
}


void
Heap::removeTop() noexcept
{
    assert(nodeCount_ >= 1);
    std::size_t lastNodeIndex = --nodeCount_;

    if (lastNodeIndex >= 1) {
        Node *lastNode = getNode(lastNodeIndex);
        siftDown(lastNode, 0);
    }
}


std::size_t
Heap::getMaxNumberOfNodes() const noexcept
{
    return nodes_.getLength();
}


void
Heap::setMaxNumberOfNodes(std::size_t maxNumberOfNodes)
{
    nodes_.setLength(maxNumberOfNodes);
}


void
Heap::siftUp(Node *x, std::size_t i) noexcept
{
    while (i >= 1) {
        std::size_t j = (i - 1) / 2;
        Node *y = getNode(j);

        if (nodeOrderer_(y, x)) {
            break;
        } else {
            setNode(i, y);
            i = j;
        }
    }

    setNode(i, x);
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
                    j = nodeOrderer_(getNode(j1), getNode(j2)) ? j1 : j2;
                } else {
                    j = j1;
                }
            } else {
                break;
            }
        }

        Node *y = getNode(j);

        if (nodeOrderer_(x, y)) {
            break;
        } else {
            setNode(i, y);
            i = j;
        }
    }

    setNode(i, x);
}


void
Heap::setNode(std::size_t nodeIndex, Node *node) noexcept
{
    (nodes_[nodeIndex] = node)->index_ = nodeIndex;
}

} // namespace siren
