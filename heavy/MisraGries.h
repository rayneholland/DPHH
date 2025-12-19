#ifndef MISRAGRIES_H
#define MISRAGRIES_H

#include "sketchHH.h"
#include "../help/nodes.h"
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <cassert>
#include <utility>

class MisraGries : public SketchHH {
public:
    explicit MisraGries(size_t num_counters)
    : smallest_(new Parent()), largest_(smallest_) {
        index_.reserve(num_counters * 2);
        for (size_t i = 0; i < num_counters; ++i) {
            smallest_->Add(new Child());
        }
    }

    ~MisraGries() override {
        Parent* p = smallest_;
        while (p != nullptr) {
            if (p->child_) {
                Child* start = p->child_;
                Child* it    = start;
                do {
                    Child* nxt = it->next_;
                    delete it;
                    it = (nxt == start ? nullptr : nxt);
                } while (it != nullptr);
            }
            Parent* nxtp = p->right_;
            delete p;
            p = nxtp;
        }
        index_.clear();
    }

    void update(int item) override {
        Process(item);
    }

    [[nodiscard]] std::vector<std::pair<int, double>> query() const override {
        std::vector<std::pair<int, double>> result;
        Parent* p = largest_;
        while (p != nullptr) {
            Child* c = p->child_;
            if (c != nullptr) {
                Child* start = c;
                do {
                    if (c->in_use_) {
                        size_t abs_val = AbsoluteFor(p);
                        result.emplace_back(c->element_, static_cast<double>(abs_val));
                    }
                    c = c->next_;
                } while (c != start);
            }
            p = p->left_;
        }
        return result;
    }


void print() const {
        using std::cout;

        cout << "=== MisraGries Debug ===\n";

        // Base (absolute value stored at the smallest group)
        cout << "Base (smallest_->value_): ";
        if (smallest_) cout << smallest_->value_ << "\n";
        else           cout << "(null)\n";

        // Walk groups from smallest -> largest
        const Parent* grp = smallest_;
        size_t group_idx = 0;

        while (grp != nullptr) {
            cout << "Group[" << group_idx << "] "
                 << "(offset=" << grp->value_ << "): ";

            size_t abs_val = AbsoluteFor(grp);

            const Child* c = grp->child_;
            if (c == nullptr) {
                cout << "[empty]\n";
            } else {
                const Child* start = c;
                cout << "\n";
                size_t ring_pos = 0;
                do {
                    cout << "  - child#" << ring_pos
                         << " elem=" << c->element_
                         << " in_use=" << (c->in_use_ ? "true" : "false");

                    if (c->in_use_) {
                        cout << " est=" << abs_val;
                    }
                    cout << "\n";
                    c = c->next_;
                    ++ring_pos;
                } while (c != start);
            }

            grp = grp->right_;
            ++group_idx;
        }
        cout << "=== End ===\n";
    }

private:
    std::unordered_map<int, Child*> index_;
    Parent* smallest_{nullptr};
    Parent* largest_{nullptr};


    [[nodiscard]] size_t AbsoluteFor(const Parent* p) const {
        if (!smallest_) return 0;
        if (!p) return 0;
        size_t abs_val = smallest_->value_;
        const Parent* walk = p;
        while (walk && walk != smallest_) {
            abs_val += walk->value_;
            walk = walk->left_;
        }
        return abs_val;
    }

    void Process(int element) {
        auto it = index_.find(element);
        if (it != index_.end()) {
            Increment(it->second);
            return;
        }
        if (smallest_->value_ > 0) {
            smallest_->value_ -= 1;
        } else {
            Child* bucket = smallest_->child_;
            assert(bucket);
            if (bucket->in_use_) {
                index_.erase(bucket->element_);
            }
            bucket->element_ = element;
            bucket->in_use_  = true;
            index_[element]  = bucket;

            Parent* next_grp = smallest_->right_;
            Child* moving = bucket->Detach(&smallest_, index_);
            if (next_grp != nullptr && next_grp->value_ == 1) {
                next_grp->Add(moving);
            } else {
                auto* p = new Parent();
                if(smallest_ == next_grp) {
                    smallest_ = p;
                    p->left_ = nullptr;
                } else {
                    p->left_ = smallest_;
                    smallest_->right_ = p;
                }
                p->value_ = 1;
                p->right_ = next_grp;

                if (next_grp) {
                    next_grp->left_ = p;
                    if (next_grp->value_ > 1) {
                        next_grp->value_ -= 1;
                    }
                } else {
                    largest_ = p;
                }
                p->Add(moving);
            }
        }
    }

    void Increment(Child* bucket) {
        assert(bucket && bucket->parent_);
        Parent* par = bucket->parent_;
        Parent* next_grp = par->right_;

        if(bucket->next_ == bucket) {
            size_t curr_offset = par->value_;

            if (next_grp != nullptr && next_grp->value_ == 1) {
                Child* moving = bucket->Detach(&smallest_, index_);
                next_grp->value_ += curr_offset;
                next_grp->Add(moving);
            } else {
                bucket->parent_->value_ += 1;
                if(next_grp!= nullptr) {
                    next_grp->value_ --;
                }
            }
        } else {
            if (next_grp != nullptr && next_grp->value_ == 1) {
                Child* moving = bucket->Detach(&smallest_, index_);
                next_grp->Add(moving);
            } else {
                Child* moving = bucket->Detach(&smallest_, index_);
                auto* p = new Parent();
                p->left_ = par;
                p->value_ = 1;
                p->right_ = next_grp;
                par->right_ = p;
                if (next_grp) {
                    next_grp->left_ = p;
                    if (next_grp->value_ > 1) {
                        next_grp->value_ -= 1;
                    }
                } else {
                    largest_ = p;
                }
                p->Add(moving);
            }
        }
    }
};

#endif // MISRAGRIES_H
