

#ifndef NODES_H
#define NODES_H

#include <unordered_map>
#include <cstddef>
#include <cassert>


class Parent;
class Child;


class Child {
public:
    Child() noexcept
        : parent_(nullptr), next_(nullptr), element_(0), in_use_(false) {}

    Child* Detach(Parent** smallest, std::unordered_map<int, Child*>& index);

    Parent* parent_;
    Child*  next_;
    int     element_;
    bool    in_use_;
};

class Parent {
public:
    Parent() noexcept
        : left_(nullptr), right_(nullptr), child_(nullptr), value_(0) {}

    void Add(Child* c) noexcept;

    Parent* left_;
    Parent* right_;
    Child*  child_;
    std::size_t value_; // absolute group count
};



inline void Parent::Add(Child* c) noexcept {
    assert(c);
    c->parent_ = this;
    if (child_ == nullptr) {
        child_ = c;
        c->next_ = c;
        return;
    }
    c->next_ = child_->next_;
    child_->next_ = c;
    child_ = c;
}

inline Child* Child::Detach(Parent** smallest, std::unordered_map<int, Child*>& index) {
    assert(parent_ && smallest);

    if (next_ == this) {
        Parent* grp = parent_;
        if (grp == *smallest) {
            *smallest = grp->right_;
            if (grp->right_) grp->right_->left_ = nullptr;
        } else {
            if (grp->right_) grp->right_->left_ = grp->left_;
            if (grp->left_)  grp->left_->right_ = grp->right_;
        }
        parent_ = nullptr;
        next_   = nullptr;
        delete grp;
        return this;
    }
    if (parent_->child_ == next_) {
        parent_->child_ = this;
    }
    const int  tmp_el = element_;
    const bool tmp_in = in_use_;
    element_  = next_->element_;
    in_use_   = next_->in_use_;
    next_->element_ = tmp_el;
    next_->in_use_  = tmp_in;

    if (in_use_)            index[element_] = this;
    if (next_->in_use_)     index[next_->element_] = next_;

    Child* removed = next_;
    next_ = next_->next_;
    removed->parent_ = nullptr;
    removed->next_   = nullptr;
    return removed;
}

#endif // NODES_H