
#ifndef CSSOHH_H
#define CSSOHH_H

#include <vector>
#include <utility>
#include "sketchHH.h"
#include "../heap/IndexMinHeap.h"
#include "../sketch/CSSO.h"

using namespace std;

class CSSOHH : public SketchHH {
private:

    size_t k_{0};
    size_t tilde_k_{0};
    size_t n_{0};
    int depth_{0};
    double eps_{1.0};
    double delta_{1e-6};
    CSSO* sketch;
    IndexMinHeap<int,double> heap;

public:
    CSSOHH(int depth, double epsilon, double delta, int k, int tilde_k, uint32_t seed_index, uint32_t seed_sign, size_t T)
        : k_(k), tilde_k_(tilde_k), eps_(epsilon), delta_(delta), heap(tilde_k_) {
        int min_d;
        min_d = static_cast<int>(log(6.0*static_cast<double>(T)/(delta_))) +1;

        if(depth < min_d) {
            depth_ = min_d;
        } else {
            depth_ = depth;
        }
        sketch = new CSSO(2*tilde_k, depth_, epsilon, seed_index, seed_sign);
    }

    ~CSSOHH() override {
        delete sketch;
    }

    void update(int item) override {
        ++n_;
        // sketch->update(item, 1);
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
        vector<pair<int,double>> filter;

        double noise;
        noise = (2.0 * depth_ / eps_) * log(6.0 * depth_ * static_cast<double>(tilde_k_) / delta_);

        const double eta = sqrt(3.0 / static_cast<double>(tilde_k_));
        double F2_est = sketch->queryF2();
        double F2_upper = (1+eta) * F2_est;

        double freq_error = eta * sqrt(F2_upper / static_cast<double>(tilde_k_));

        double additive_error = 2 * (noise + freq_error);

        const auto n_double = static_cast<double>(n_);
        const double tau_1 = n_double / static_cast<double>(k_);
        const double tau_2 = n_double / static_cast<double>(tilde_k_) + 1.0 + additive_error;
        const double tau = max(tau_1, tau_2);

        for (auto &p : heap.items()) {
            double est = sketch->query(p.first);
            if (p.second >= tau && est >= tau) {
                filter.push_back(p);
            }
        }
        return filter;
    }
};


#endif //CSSOHH_H
