#pragma once


namespace siren {

class RBTree;
namespace detail { enum class RBTreeNodeColor; }


class RBTreeNode
{
public:
    inline const RBTreeNode *getLeftChild() const noexcept;
    inline RBTreeNode *getLeftChild() noexcept;
    inline const RBTreeNode *getRightChild() const noexcept;
    inline RBTreeNode *getRightChild() noexcept;
    inline const RBTreeNode *getParent() const noexcept;
    inline RBTreeNode *getParent() noexcept;

protected:
    inline explicit RBTreeNode() noexcept;

    ~RBTreeNode() = default;

    void setLeftChild(RBTreeNode *) noexcept;
    void setLeftChildToNil(RBTreeNode *) noexcept;
    void setRightChild(RBTreeNode *) noexcept;
    void setRightChildToNil(RBTreeNode *) noexcept;
    void replace(RBTreeNode *) noexcept;
    void rotateLeft() noexcept;
    void rotateRight() noexcept;

private:
    typedef detail::RBTreeNodeColor Color;

    RBTreeNode *children_[2];
    RBTreeNode *parent_;
    Color color_;

    RBTreeNode(const RBTreeNode &) = delete;
    RBTreeNode &operator=(const RBTreeNode &) = delete;

    friend RBTree;
};


class RBTree final
{
public:
    typedef RBTreeNode Node;

    inline bool isEmpty() const noexcept;
    inline const Node *getRoot() const noexcept;
    inline Node *getRoot() noexcept;
    inline const RBTreeNode *getMin() const noexcept;
    inline RBTreeNode *getMin() noexcept;
    inline const RBTreeNode *getMax() const noexcept;
    inline RBTreeNode *getMax() noexcept;
    inline bool isNil(const Node *) const noexcept;

    explicit RBTree(bool (*)(const Node *, const Node *)) noexcept;
    RBTree(RBTree &&) noexcept;
    RBTree &operator=(RBTree &&) noexcept;

    void reset() noexcept;
    void insertNode(Node *) noexcept;
    void removeNode(Node *) noexcept;

private:
    typedef detail::RBTreeNodeColor NodeColor;

    bool (*const nodeOrderer_)(const Node *, const Node *);
    Node nil_;

    void initialize() noexcept;
    void move(RBTree *) noexcept;
    void setRoot(Node *) noexcept;
    void fixForNodeInsertion(Node *) noexcept;
    void fixForNodeRemoval(Node *) noexcept;
};

} // namespace siren


/*
 * #include "rb_tree-inl.h"
 */


#include <cassert>


namespace siren {

RBTreeNode::RBTreeNode() noexcept
{
}


const RBTreeNode *
RBTreeNode::getLeftChild() const noexcept
{
    return children_[0];
}


RBTreeNode *
RBTreeNode::getLeftChild() noexcept
{
    return children_[0];
}


const RBTreeNode *
RBTreeNode::getRightChild() const noexcept
{
    return children_[1];
}


RBTreeNode *
RBTreeNode::getRightChild() noexcept
{
    return children_[1];
}


const RBTreeNode *
RBTreeNode::getParent() const noexcept
{
    return parent_;
}


RBTreeNode *
RBTreeNode::getParent() noexcept
{
    return parent_;
}


bool
RBTree::isEmpty() const noexcept
{
    return getRoot() == &nil_;
}


const RBTreeNode *
RBTree::getRoot() const noexcept
{
    return nil_.getLeftChild();
}


RBTreeNode *
RBTree::getRoot() noexcept
{
    return nil_.getLeftChild();
}


const RBTreeNode *
RBTree::getMin() const noexcept
{
    const Node *x;
    for (x = getRoot(); x->getLeftChild() != &nil_; x = x->getLeftChild());
    return x;
}


RBTreeNode *
RBTree::getMin() noexcept
{
    Node *x;
    for (x = getRoot(); x->getLeftChild() != &nil_; x = x->getLeftChild());
    return x;
}


const RBTreeNode *
RBTree::getMax() const noexcept
{
    const Node *x;
    for (x = getRoot(); x->getRightChild() != &nil_; x = x->getRightChild());
    return x;
}


RBTreeNode *
RBTree::getMax() noexcept
{
    Node *x;
    for (x = getRoot(); x->getRightChild() != &nil_; x = x->getRightChild());
    return x;
}


bool
RBTree::isNil(const Node *node) const noexcept
{
    assert(node != nullptr);
    return node == &nil_;
}

} // namespace siren
