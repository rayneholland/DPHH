
#ifndef SKETCHHH_H
#define SKETCHHH_H

#include <random>

using namespace std;

class SketchHH {

public:

    virtual ~SketchHH() = default;

    virtual void update(int item) = 0;
    virtual vector<std::pair<int, double>> query() const = 0;

    static double laplaceNoise(double eps, double sensitivity) {
        exponential_distribution<double> exp_dist(eps / sensitivity);
        double noise = exp_dist(rng);
        return (rng() % 2 == 0) ? noise : -noise;  // Randomly flip sign
    }

    static double gaussianNoise(double sigma) {
        normal_distribution<double> norm_dist(0.0, sigma);
        return norm_dist(rng);
    }

protected:
    static mt19937 rng;

    struct Cmp {
        bool operator()(const pair<int,double>& a, const pair<int,double>& b) const {
            return a.second > b.second;
        }
    };


};

mt19937 SketchHH::rng(random_device{}());

#endif //SKETCHHH_H
