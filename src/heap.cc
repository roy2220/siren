#include "heap.h"


namespace siren {

void
Heap::insertNode(Node *node)
{
    assert(node != nullptr);
    assert(node->isUnlinked());

    if (numberOfNodes_ == nodes_.getLength()) {
        nodes_.setLength(numberOfNodes_ + 1);
    }

    std::size_t i = numberOfNodes_++;
    siftUp(node, i);
}


void
Heap::removeNode(Node *node)
{
    assert(node != nullptr);
    assert(node->isLinked());
    assert(node->index_ < numberOfNodes_ && node == nodes_[node->index_]);
    std::size_t i = node->index_;
    node->initialize();

    if (i < --numberOfNodes_) {
        node = nodes_[numberOfNodes_];

        if (nodeOrderer_(*node, *nodes_[i])) {
            siftUp(node, i);
        } else {
            siftDown(node, i);
        }
    }
}


void
Heap::removeTop()
{
    assert(numberOfNodes_ >= 1);
    Node *top = nodes_[0];
    top->initialize();

    if (0 < --numberOfNodes_) {
        top = nodes_[numberOfNodes_];
        siftDown(top, 0);
    }
}


void
Heap::siftUp(Node *node, std::size_t i)
{
    while (i >= 1) {
        std::size_t j = (i - 1) / 2;

        if (nodeOrderer_(*nodes_[j], *node)) {
            break;
        }

        (nodes_[i] = nodes_[j])->index_ = i;
        i = j;
    }

    (nodes_[i] = node)->index_ = i;
}


void
Heap::siftDown(Node *node, std::size_t i)
{
    for (;;) {
        std::size_t j;

        {
            std::size_t j1 = (2 * i) + 1;
            std::size_t j2 = j1 + 1;

            if (j2 >= numberOfNodes_) {
                if (j1 >= numberOfNodes_) {
                    break;
                }

                j = j1;
            } else {
                j = nodeOrderer_(*nodes_[j1], *nodes_[j2]) ? j1 : j2;
            }
        }

        if (nodeOrderer_(*node, *nodes_[j])) {
            break;
        }

        (nodes_[i] = nodes_[j])->index_ = i;
        i = j;
    }

    (nodes_[i] = node)->index_ = i;
}

}
