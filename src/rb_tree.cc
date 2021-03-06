#include "rb_tree.h"

#include <algorithm>
#include <functional>


namespace siren {

namespace detail {

enum class RBTreeNodeColor
{
    Red,
    Black,
};

} // namespace detail


void
RBTreeNode::setLeftChild(RBTreeNode *node) noexcept
{
    (children_[0] = node)->parent_ = this;
}


void
RBTreeNode::setLeftChildToNull(RBTreeNode *null) noexcept
{
    children_[0] = null;
}


void
RBTreeNode::setRightChild(RBTreeNode *node) noexcept
{
    (children_[1] = node)->parent_ = this;
}


void
RBTreeNode::setRightChildToNull(RBTreeNode *null) noexcept
{
    children_[1] = null;
}


void
RBTreeNode::replace(RBTreeNode *other) noexcept
{
    (parent_->children_[this != parent_->children_[0]] = other)->parent_ = parent_;
}


void
RBTreeNode::rotateLeft() noexcept
{
    RBTreeNode *x = getRightChild();
    setRightChild(x->getLeftChild());
    replace(x);
    x->setLeftChild(this);
}


void
RBTreeNode::rotateRight() noexcept
{
    RBTreeNode *x = getLeftChild();
    setLeftChild(x->getRightChild());
    replace(x);
    x->setRightChild(this);
}


RBTree::RBTree(bool (*nodeOrderer)(const Node *, const Node *)) noexcept
  : nodeOrderer_(nodeOrderer)
{
    SIREN_ASSERT(nodeOrderer != nullptr);
    nil_.color_ = NodeColor::Black;
    initialize();
}


RBTree::RBTree(RBTree &&other) noexcept
  : nodeOrderer_(other.nodeOrderer_)
{
    nil_.color_ = NodeColor::Black;
    other.move(this);
}


RBTree &
RBTree::operator=(RBTree &&other) noexcept
{
    if (&other != this) {
        SIREN_ASSERT(nodeOrderer_ == other.nodeOrderer_);
        other.move(this);
    }

    return *this;
}


void
RBTree::initialize() noexcept
{
    setRoot(&nil_);
}


void
RBTree::move(RBTree *other) noexcept
{
    if (isEmpty()) {
        other->initialize();
    } else {
        other->setRoot(getRoot());
        initialize();

        std::function<void (Node *)> patch = [oldNil = &nil_, newNil = &other->nil_
                                              , &patch] (Node *root) -> void {
            if (root->getLeftChild() == oldNil) {
                root->setLeftChildToNull(newNil);
            } else {
                patch(root->getLeftChild());
            }

            if (root->getRightChild() == oldNil) {
                root->setRightChildToNull(newNil);
            } else {
                patch(root->getRightChild());
            }
        };

        patch(other->getRoot());
    }
}


void
RBTree::reset() noexcept
{
    initialize();
}


void
RBTree::setRoot(Node *node) noexcept
{
    nil_.setLeftChild(node);
}


void
RBTree::insertNode(Node *x) noexcept
{
    SIREN_ASSERT(x != nullptr);
    SIREN_ASSERT(x != &nil_);
    Node *y = &nil_;
    Node *z = nil_.getLeftChild();
    void (Node::*setChild)(Node *) = &Node::setLeftChild;

    while (z != &nil_) {
        y = z;

        if (nodeOrderer_(x, z)) {
            z = z->getLeftChild();
            setChild = &Node::setLeftChild;
        } else {
            z = z->getRightChild();
            setChild = &Node::setRightChild;
        }
    }

    x->setLeftChildToNull(&nil_);
    x->setRightChildToNull(&nil_);
    (y->*setChild)(x);
    x->color_ = NodeColor::Red;
    fixForNodeInsertion(x);
}


void
RBTree::removeNode(Node *x) noexcept
{
    SIREN_ASSERT(!isEmpty());
    SIREN_ASSERT(x != nullptr);
    SIREN_ASSERT(x != &nil_);
    Node *y;
    Node *z;

    if (x->getLeftChild() == &nil_) {
        y = x;
        z = x->getRightChild();
    } else if (x->getRightChild() == &nil_) {
        y = x;
        z = x->getLeftChild();
    } else {
        for (Node *w = x->getLeftChild(), *v = x->getRightChild()
             ;; w = w->getRightChild(), v = v->getLeftChild()) {
            if (w->getRightChild() == &nil_) {
                y = w;
                z = w->getLeftChild();
                break;
            } else if (v->getLeftChild() == &nil_) {
                y = v;
                z = v->getRightChild();
                break;
            }
        }
    }

    y->replace(z);
    bool needFixing = y->color_ == NodeColor::Black;

    if (y != x) {
        y->setLeftChild(x->getLeftChild());
        y->setRightChild(x->getRightChild());
        x->replace(y);
        y->color_ = x->color_;
    }

    if (needFixing) {
        fixForNodeRemoval(z);
    }
}


void
RBTree::fixForNodeInsertion(Node *x) noexcept
{
    for (;;) {
        Node *y = x->getParent();

        if (y->color_ == NodeColor::Red) {
            Node *z = y->getParent();

            if (y == z->getLeftChild()) {
                Node *w = z->getRightChild();

                if (w->color_ == NodeColor::Red) {
                    z->color_ = NodeColor::Red;
                    y->color_ = NodeColor::Black;
                    w->color_ = NodeColor::Black;
                    x = z;
                } else {
                    if (x == y->getRightChild()) {
                        y->rotateLeft();
                        std::swap(x, y);
                    }

                    y->color_ = NodeColor::Black;
                    z->color_ = NodeColor::Red;
                    z->rotateRight();
                    break;
                }
            } else {
                Node *w = z->getLeftChild();

                if (w->color_ == NodeColor::Red) {
                    z->color_ = NodeColor::Red;
                    y->color_ = NodeColor::Black;
                    w->color_ = NodeColor::Black;
                    x = z;
                } else {
                    if (x == y->getLeftChild()) {
                        y->rotateRight();
                        std::swap(x, y);
                    }

                    y->color_ = NodeColor::Black;
                    z->color_ = NodeColor::Red;
                    z->rotateLeft();
                    break;
                }
            }
        } else {
            break;
        }
    }

    getRoot()->color_ = NodeColor::Black;
}


void
RBTree::fixForNodeRemoval(Node *x) noexcept
{
    while (x->color_ == NodeColor::Black && x != getRoot()) {
        Node *y = x->getParent();

        if (x == y->getLeftChild()) {
            Node *z = y->getRightChild();

            if (z->color_ == NodeColor::Red) {
                z->color_ = NodeColor::Black;
                y->color_ = NodeColor::Red;
                y->rotateLeft();
                z = y->getRightChild();
            }

            Node *w = z->getRightChild();
            Node *v = z->getLeftChild();

            if (w->color_ == NodeColor::Black && v->color_ == NodeColor::Black) {
                z->color_ = NodeColor::Red;
                x = y;
            } else {
                if (w->color_ == NodeColor::Black) {
                    v->color_ = NodeColor::Black;
                    z->color_ = NodeColor::Red;
                    z->rotateRight();
                    w = z;
                    z = v;
                }

                z->color_ = y->color_;
                y->color_ = NodeColor::Black;
                w->color_ = NodeColor::Black;
                y->rotateLeft();
                x = getRoot();
                break;
            }
        } else {
            Node *z = y->getLeftChild();

            if (z->color_ == NodeColor::Red) {
                z->color_ = NodeColor::Black;
                y->color_ = NodeColor::Red;
                y->rotateRight();
                z = y->getLeftChild();
            }

            Node *w = z->getLeftChild();
            Node *v = z->getRightChild();

            if (w->color_ == NodeColor::Black && v->color_ == NodeColor::Black) {
                z->color_ = NodeColor::Red;
                x = y;
            } else {
                if (w->color_ == NodeColor::Black) {
                    v->color_ = NodeColor::Black;
                    z->color_ = NodeColor::Red;
                    z->rotateLeft();
                    w = z;
                    z = v;
                }

                z->color_ = y->color_;
                y->color_ = NodeColor::Black;
                w->color_ = NodeColor::Black;
                y->rotateRight();
                x = getRoot();
                break;
            }
        }
    }

    x->color_ = NodeColor::Black;
}


const RBTreeNode *
RBTree::findMin() const noexcept
{
    const Node *x = getRoot();

    if (x == &nil_) {
        return &nil_;
    } else {
        while (x->getLeftChild() != &nil_) {
            x = x->getLeftChild();
        }

        return x;
    }
}


RBTreeNode *
RBTree::findMin() noexcept
{
    Node *x = getRoot();

    if (x == &nil_) {
        return &nil_;
    } else {
        while (x->getLeftChild() != &nil_) {
            x = x->getLeftChild();
        }

        return x;
    }
}


const RBTreeNode *
RBTree::findMax() const noexcept
{
    const Node *x = getRoot();

    if (x == &nil_) {
        return &nil_;
    } else {
        while (x->getRightChild() != &nil_) {
            x = x->getRightChild();
        }

        return x;
    }
}


RBTreeNode *
RBTree::findMax() noexcept
{
    Node *x = getRoot();

    if (x == &nil_) {
        return &nil_;
    } else {
        while (x->getRightChild() != &nil_) {
            x = x->getRightChild();
        }

        return x;
    }
}


const RBTreeNode *
RBTree::findNodePrev(const Node *x) const noexcept
{
    SIREN_ASSERT(x != nullptr);
    SIREN_ASSERT(x != &nil_);

    if (x->getLeftChild() == &nil_) {
        for (;;) {
            const Node *y = x;
            x = x->getParent();

            if (x == &nil_) {
                return &nil_;
            } else {
                if (x->getRightChild() == y) {
                    return x;
                }
            }
        }
    } else {
        x = x->getLeftChild();

        while (x->getRightChild() != &nil_) {
            x = x->getRightChild();
        }

        return x;
    }
}


RBTreeNode *
RBTree::findNodePrev(Node *x) noexcept
{
    SIREN_ASSERT(x != nullptr);
    SIREN_ASSERT(x != &nil_);

    if (x->getLeftChild() == &nil_) {
        for (;;) {
            Node *y = x;
            x = x->getParent();

            if (x == &nil_) {
                return &nil_;
            } else {
                if (x->getRightChild() == y) {
                    return x;
                }
            }
        }
    } else {
        x = x->getLeftChild();

        while (x->getRightChild() != &nil_) {
            x = x->getRightChild();
        }

        return x;
    }
}


const RBTreeNode *
RBTree::findNodeNext(const Node *x) const noexcept
{
    SIREN_ASSERT(x != nullptr);
    SIREN_ASSERT(x != &nil_);

    if (x->getRightChild() == &nil_) {
        for (;;) {
            const Node *y = x;
            x = x->getParent();

            if (x == &nil_) {
                return &nil_;
            } else {
                if (x->getLeftChild() == y) {
                    return x;
                }
            }
        }
    } else {
        x = x->getRightChild();

        while (x->getLeftChild() != &nil_) {
            x = x->getLeftChild();
        }

        return x;
    }
}


RBTreeNode *
RBTree::findNodeNext(Node *x) noexcept
{
    SIREN_ASSERT(x != nullptr);
    SIREN_ASSERT(x != &nil_);

    if (x->getRightChild() == &nil_) {
        for (;;) {
            Node *y = x;
            x = x->getParent();

            if (x == &nil_) {
                return &nil_;
            } else {
                if (x->getLeftChild() == y) {
                    return x;
                }
            }
        }
    } else {
        x = x->getRightChild();

        while (x->getLeftChild() != &nil_) {
            x = x->getLeftChild();
        }

        return x;
    }
}

} // namespace siren
