#ifndef SPACESAVING_H
#define SPACESAVING_H

#include "sketchHH.h"
#include "../help/nodes.h"
#include <unordered_map>
#include <vector>
#include <cstddef>
#include <cassert>
#include <utility>

class SpaceSaving : public SketchHH {

public:
  explicit SpaceSaving(size_t num_counters)
      : smallest_(new Parent()),
        largest_(smallest_) {
    index_.reserve(num_counters * 2);
    for (size_t i = 0; i < num_counters; ++i) {
      smallest_->Add(new Child());
    }
  }

  ~SpaceSaving() override {
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

  [[nodiscard]] vector<pair<int, double>> query() const override {
    vector<pair<int, double>> result;
    Parent* p = largest_;
    while (p != nullptr) {
      Child* c = p->child_;
      if (c != nullptr) {
        Child* start = c;
        do {
          if (c->in_use_) {
            result.emplace_back(c->element_, static_cast<double>(p->value_));
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

    cout << "=== SpaceSaving Debug ===\n";

    cout << "Base (smallest_->value_): ";
    if (smallest_) cout << smallest_->value_ << "\n";
    else           cout << "(null)\n";

    const Parent* grp = smallest_;
    size_t group_idx = 0;

    while (grp != nullptr) {
      cout << "Group[" << group_idx << "] "
           << "(value=" << grp->value_ << "): ";

      const Child* c = grp->child_;
      if (c == nullptr) {
        cout << "[empty]\n";
      } else {
        const Child* start = c;
        cout << "\n";
        size_t ring_pos = 0;
        do {
          cout << "  - child#" << ring_pos
               << " elem=" << c->element_;
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

  unordered_map<int, Child*> index_;
  Parent* smallest_{nullptr};
  Parent* largest_{nullptr};

  void Process(int element) {
    auto it = index_.find(element);
    if (it == index_.end()) {
      Child* bucket = smallest_->child_;
      assert(bucket);
      if (bucket->in_use_) {
        index_.erase(bucket->element_);
      }
      bucket->element_ = element;
      bucket->in_use_  = true;
      index_[element]  = bucket;
      Increment(bucket);
    } else {
      Increment(it->second);
    }
  }

  void Increment(Child* bucket) {
    assert(bucket && bucket->parent_);
    Parent* g        = bucket->parent_;
    Parent* next_grp = g->right_;
    const size_t next_count = g->value_ + 1;

    if (next_grp != nullptr && next_grp->value_ == next_count) {
      Child* moving = bucket->Detach(&smallest_, index_);
      next_grp->Add(moving);
    } else if (bucket->next_ == bucket) {
      g->value_ = next_count;
      if (next_grp == nullptr) {
        largest_ = g;
      }
    } else {
      Child* moving = bucket->Detach(&smallest_, index_);
      auto* p = new Parent();
      p->left_  = g;
      p->value_ = next_count;
      p->right_ = next_grp;
      g->right_ = p;
      if (next_grp != nullptr) {
        next_grp->left_ = p;
      } else {
        largest_ = p;
      }
      p->Add(moving);
    }
  }
};

#endif // SPACESAVING_H
