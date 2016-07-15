#pragma once


#define SIREN_LIST_FOR_EACH_ITEM_REVERSE(LIST_ITEM, LIST)                              \
    for (::siren::List::Item *LIST_ITEM = (LIST).getTail(); !(LIST).isNil((LIST_ITEM)) \
         ; (LIST_ITEM) = (LIST_ITEM)->getPrev())

#define SIREN_LIST_FOR_EACH_ITEM(LIST_ITEM, LIST)                                      \
    for (::siren::List::Item *LIST_ITEM = (LIST).getHead(); !(LIST).isNil((LIST_ITEM)) \
         ; (LIST_ITEM) = (LIST_ITEM)->getNext())

#define SIREN_LIST_FOR_EACH_ITEM_SAFE_REVERSE(LIST_ITEM, LIST)                              \
    for (::siren::List::Item *LIST_ITEM = (LIST).getTail(), *temp_ = (LIST_ITEM)->getPrev() \
         ; !(LIST).isNil((LIST_ITEM)); (LIST_ITEM) = temp_, temp_ = (LIST_ITEM)->getPrev())

#define SIREN_LIST_FOR_EACH_ITEM_SAFE(LIST_ITEM, LIST)                                      \
    for (::siren::List::Item *LIST_ITEM = (LIST).getHead(), *temp_ = (LIST_ITEM)->getNext() \
         ; !(LIST).isNil((LIST_ITEM)); (LIST_ITEM) = temp_, temp_ = (LIST_ITEM)->getNext())


namespace siren {

class List;


class ListItem
{
public:
    inline ListItem *getPrev() const;
    inline ListItem *getNext() const;
    inline void insertBefore(ListItem *);
    inline void insertAfter(ListItem *);
    inline void remove();

protected:
    inline explicit ListItem();
    inline ListItem(const ListItem &);
    inline ListItem(ListItem &&);
    inline ListItem &operator=(const ListItem &);
    inline ListItem &operator=(ListItem &&);

    ~ListItem() = default;
    
private:
    ListItem *prev_;
    ListItem *next_;

#ifndef NDEBUG
    inline bool isLinked() const;
    inline bool isUnlinked() const;
#endif
    inline void insert(ListItem *, ListItem *);

    friend List;
};


class List final
{
public:
    typedef ListItem Item;

    inline explicit List();

    inline bool isEmpty() const;
    inline Item *getTail() const;
    inline Item *getHead() const;
    inline bool isNil(const Item *) const;
    inline void insertTail(Item *);
    inline void insertHead(Item *);

private:
    Item nil_;

    List(const List &) = delete;
    List &operator=(const List &) = delete;
};

}


/*
 * #include "list-inl.h"
 */


#include <cassert>


namespace siren {

ListItem::ListItem()
#ifndef NDEBUG
  : prev_(nullptr),
    next_(nullptr)
#endif
{
}


ListItem::ListItem(const ListItem &other)
  : ListItem()
{
    static_cast<void>(other);
}


ListItem::ListItem(ListItem &&other)
  : ListItem()
{
    static_cast<void>(other);
}


ListItem &
ListItem::operator=(const ListItem &other)
{
    static_cast<void>(other);
    return *this;
}


ListItem &
ListItem::operator=(ListItem &&other)
{
    static_cast<void>(other);
    return *this;
}


#ifndef NDEBUG
bool
ListItem::isLinked() const
{
    return prev_ != nullptr && next_ != nullptr;
}


bool
ListItem::isUnlinked() const
{
    return prev_ == nullptr && next_ == nullptr;
}
#endif


ListItem *
ListItem::getPrev() const
{
    assert(isLinked());
    return prev_;
}


ListItem *
ListItem::getNext() const
{
    assert(isLinked());
    return next_;
}


void
ListItem::insertBefore(ListItem *other)
{
    assert(isUnlinked());
    assert(other != nullptr);
    assert(other->isLinked());
    insert(other->prev_, other);
}


void
ListItem::insertAfter(ListItem *other)
{
    assert(isUnlinked());
    assert(other != nullptr);
    assert(other->isLinked());
    insert(other, other->next_);
}


void
ListItem::insert(ListItem *prev, ListItem *next)
{
    (prev_ = prev)->next_ = this;
    (next_ = next)->prev_ = this;
}


void
ListItem::remove()
{
    assert(isLinked());
    prev_->next_ = next_;
    next_->prev_ = prev_;
#ifndef NDEBUG
    prev_ = nullptr;
    next_ = nullptr;
#endif
}


List::List()
{
    nil_.prev_ = &nil_;
    nil_.next_ = &nil_;
}


bool
List::isEmpty() const
{
    return nil_.prev_ == &nil_;
}


List::Item *
List::getTail() const
{
    return nil_.prev_;
}


List::Item *
List::getHead() const
{
    return nil_.next_;
}


bool
List::isNil(const Item *item) const
{
    return item == &nil_;
}


void
List::insertTail(Item *item)
{
    assert(item != nullptr);
    assert(item->isUnlinked());
    item->insert(getTail(), &nil_);
}


void
List::insertHead(Item *item)
{
    assert(item != nullptr);
    assert(item->isUnlinked());
    item->insert(&nil_, getHead());
}

}
