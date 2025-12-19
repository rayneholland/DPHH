
#ifndef SSSO_H
#define SSSO_H

#include <cmath>

#include "SketchHH.h"
#include "SpaceSaving.h"

class SSSO : public SketchHH {

private:
    size_t k_{0};
    size_t tilde_k_{0};
    size_t n_{0};
    double eps_{1.0};
    double delta_{1e-6};

    SpaceSaving* ss_{nullptr};

public:

    SSSO(size_t k, size_t tilde_k, double eps, double delta)
           : k_(k),
             tilde_k_(tilde_k),
             eps_(eps),
             delta_(delta),
             n_(0),
             ss_(new SpaceSaving(tilde_k))
    {}

    ~SSSO() override {
        delete ss_;
        ss_ = nullptr;
    }

    void update(int item) override {
        ss_->update(item);
        ++n_;
    }

    [[nodiscard]] vector<pair<int, double>> query() const override {
        vector<pair<int, double>> out;

        if (k_ < 0 || eps_ < 0.0 || delta_ < 0.0) {
            return out;
        }

        double gamma;
        gamma = (1.0 / eps_) * log(2.0 / delta_);
        const vector<pair<int, double>> ss_summary = ss_->query();

        const auto n_double = static_cast<double>(n_);
        const double tau = max(n_double / static_cast<double>(k_),
                                    n_double / static_cast<double>(tilde_k_) + 1.0 + gamma);

        for (const auto& kv : ss_summary) {
            const int item = kv.first;
            const double count_est = kv.second;
            const double noise = laplaceNoise(eps_, /*sensitivity=*/1.0);
            const double noisy = count_est + noise;

            if (noisy > tau) {
                out.emplace_back(item, noisy);
            }
        }
        return out;
    }


};

#endif //SSSO_H
