

#ifndef MGSO_H
#define MGSO_H
#include <cmath>

#include "MisraGries.h"

class MGSO : public SketchHH {

private:

    size_t k_{0};
    double eps_{1.0};
    double delta_{1e-6};
    size_t n_{0};

    MisraGries* mg_{nullptr};

public:

    MGSO(size_t k, size_t tilde_k, double eps, double delta)
            : k_(k), eps_(eps), delta_(delta), n_(0), mg_(new MisraGries(tilde_k)) {}


    ~MGSO() override {
        delete mg_;
        mg_ = nullptr;
    }

    void update(int item) override {
        mg_->update(item);
        ++n_;
    }

    [[nodiscard]] vector<pair<int, double>> query() const override {
        vector<pair<int, double>> out;

        if (k_ == 0 || eps_ <= 0.0 || delta_ <= 0.0) {
            return out;
        }
        const double tau =  (1.0 + (2.0 * log(3.0 / delta_)) / eps_);
        const double eta = laplaceNoise(eps_, /*sensitivity=*/1.0);
        const auto summary = mg_->query();

        const double hh_tau = static_cast<double>(n_)/static_cast<double>(k_);

        for (const auto& kv : summary) {
            const int item = kv.first;
            const double count_est = kv.second;
            const double noisy = count_est
                               + eta
                               + laplaceNoise(eps_, /*sensitivity=*/1.0);
            if (noisy >= tau && noisy >= hh_tau) {
                out.emplace_back(item, noisy);
            }
        }
        return out; 
    }
};


#endif //MGSO_H
