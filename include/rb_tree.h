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

private:
    typedef detail::RBTreeNodeColor Color;

    RBTreeNode *children_[2];
    RBTreeNode *parent_;
    Color color_;

    void setLeftChild(RBTreeNode *) noexcept;
    void setLeftChildToNull(RBTreeNode *) noexcept;
    void setRightChild(RBTreeNode *) noexcept;
    void setRightChildToNull(RBTreeNode *) noexcept;
    void replace(RBTreeNode *) noexcept;
    void rotateLeft() noexcept;
    void rotateRight() noexcept;

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
    inline bool isNil(const Node *) const noexcept;

    template <class T>
    inline const Node *search(T &&) const;

    template <class T>
    inline Node *search(T &&);

    template <class T>
    inline void traverse(T &&) const;

    template <class T>
    inline void traverse(T &&);

    explicit RBTree(bool (*)(const Node *, const Node *)) noexcept;
    RBTree(RBTree &&) noexcept;
    RBTree &operator=(RBTree &&) noexcept;

    void reset() noexcept;
    void insertNode(Node *) noexcept;
    void removeNode(Node *) noexcept;
    const Node *findMin() const noexcept;
    Node *findMin() noexcept;
    const Node *findMax() const noexcept;
    Node *findMax() noexcept;
    const Node *findNodePrev(const Node *) const noexcept;
    Node *findNodePrev(Node *) noexcept;
    const Node *findNodeNext(const Node *) const noexcept;
    Node *findNodeNext(Node *) noexcept;

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


bool
RBTree::isNil(const Node *node) const noexcept
{
    assert(node != nullptr);
    return node == &nil_;
}


template <class T>
const RBTreeNode *
RBTree::search(T &&nodeMatcher) const
{
    const Node *x = getRoot();

    while (x != &nil_) {
        int delta = nodeMatcher(x);

        if (delta == 0) {
            return x;
        } else {
            x = delta < 0 ? x->getLeftChild() : x->getRightChild();
        }
    }

    return &nil_;
}


template <class T>
RBTreeNode *
RBTree::search(T &&nodeMatcher)
{
    Node *x = getRoot();

    while (x != &nil_) {
        int delta = nodeMatcher(x);

        if (delta == 0) {
            return x;
        } else {
            x = delta < 0 ? x->getLeftChild() : x->getRightChild();
        }
    }

    return &nil_;
}


template <class T>
void
RBTree::traverse(T &&callback) const
{
    const Node *s[64];
    int i = -1;
    const Node *x = getRoot();

    for (;;) {
        if (x == &nil_) {
            if (i < 0) {
                return;
            } else {
                x = s[i--];
                callback(x);
                x = x->getRightChild();
            }
        } else {
            s[++i] = x;
            x = x->getLeftChild();
        }
    }
}


template <class T>
void
RBTree::traverse(T &&callback)
{
    Node *s[64];
    int i = -1;
    Node *x = getRoot();

    for (;;) {
        if (x == &nil_) {
            if (i < 0) {
                return;
            } else {
                x = s[i--];
                Node *y = x;
                x = x->getRightChild();
                callback(y);
            }
        } else {
            s[++i] = x;
            x = x->getLeftChild();
        }
    }
}

} // namespace siren
