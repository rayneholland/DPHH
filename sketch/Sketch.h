

#ifndef SKETCH_H
#define SKETCH_H
#include <cstdint>
#include <bits/random.h>
#include <random>

#include "../hash/MurmurHash.h"

using namespace std;

class Sketch {

protected:
    static mt19937 rng;

public:

    virtual ~Sketch() = default;

    virtual void update(int item, int count) = 0;
    virtual double query(int item) const = 0;

    static uint32_t hash(uint32_t item, unsigned int seed) {
        uint32_t hash_val;
        MurmurHash3_x86_32(&item, sizeof(item), seed, &hash_val);
        return hash_val;
    }

    static double laplaceNoise(double eps, double sensitivity) {
        exponential_distribution<double> exp_dist(eps / sensitivity);
        double noise = exp_dist(rng);
        return (rng() % 2 == 0) ? noise : -noise;
    }

    static double gaussianNoise(double sigma) {
        normal_distribution<double> norm_dist(0.0, sigma);
        return norm_dist(rng);
    }
};

mt19937 Sketch::rng(random_device{}());


#endif //SKETCH_H
