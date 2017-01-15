#include "list.h"


namespace siren {

void
List::sort(bool (*nodeOrderer)(const Node *, const Node *)) noexcept
{
    SortNodes(&nil_.next_, &nil_.prev_, nodeOrderer);
}


void
List::SortNodes(Node **firstNode, Node **lastNode, bool (*nodeOrderer)(const Node *, const Node *))
    noexcept
{
    Node *x = *firstNode;
    Node *y = *lastNode;

    if (x != y) {
        do {
            Node *z = y;
            y = y->prev_;

            if (!nodeOrderer(x, z)) {
                z->remove();
                z->insertBefore(x);
            }
        } while (y != x);

        if (*firstNode != x) {
            SortNodes(firstNode, &x->prev_, nodeOrderer);
        }

        if (*lastNode != x) {
            SortNodes(&x->next_, lastNode, nodeOrderer);
        }
    }
}

}
