

#ifndef CMSSSHH_H
#define CMSSSHH_H

#include <vector>
#include <utility>
#include "../sketch/CMS.h"
#include "sketchHH.h"
#include "../heap/IndexMinHeap.h"
#include "../sketch/CMSSO.h"
using namespace std;

class CMSSOHH : public SketchHH {
private:

    size_t k_{0};
    size_t tilde_k_{0};
    size_t n_{0};
    int depth_{0};
    double eps_{1.0};
    double delta_{1e-6};
    CMSSO* sketch;
    IndexMinHeap<int,double> heap;

public:
    CMSSOHH(int depth, double epsilon, double delta, int k, int tilde_k, uint32_t seed, size_t T)
        : k_(k), tilde_k_(tilde_k),  eps_(epsilon), delta_(delta), heap(tilde_k) {
        int min_d;
        min_d = static_cast<int>(log(4.0*(static_cast<double>(T)+tilde_k)/(delta_))) +1;

        if(depth < min_d) {
            depth_ = min_d;
        } else {
            depth_ = depth;
        }
        sketch = new CMSSO(2*tilde_k, depth_, epsilon, seed);
    }

    ~CMSSOHH() override {
        delete sketch;
    }

    void update(int item) override {
        ++n_;

        double est = sketch->update_estimate(item, 1);

        if (heap.contains(item)) {
            heap.update(item, est);
        } else if (!heap.full()) {
            heap.insert(item, est);
        } else if (est > heap.min_value()) {
            heap.replace_top(item, est);
        }
    }

    vector<pair<int, double>> query() const override {
        vector<pair<int,double>> out;

        double noise;
        noise = (2.0 * depth_ / eps_) * log(4.0*depth_*static_cast<double>(tilde_k_) / delta_);

        const auto n_double = static_cast<double>(n_);
        const double tau_1 = n_double / static_cast<double>(k_);
        const double tau_2 = 3*n_double / static_cast<double>(tilde_k_) + 1.0 + 3*noise;
        const double tau = max(tau_1, tau_2);

        // printf("noise: %.2f, tau_1: %.2f, tau_2 %.2f\n", noise, tau_1, tau_2);

        for (auto &p : heap.items()) {
            double est = sketch->query(p.first);
            if (p.second >= tau && est >= tau) {
                out.push_back(p);
            }
        }
        return out;
    }
};

#endif //CMSSSHH_H
