
#ifndef COUNTSKETCH_H
#define COUNTSKETCH_H

#include <vector>
#include <cstdint>
#include <algorithm>
#include <limits>

#include "Sketch.h"

using namespace std;

class CS : public Sketch {
private:
    int width;
    int depth;
    uint32_t seed_index;
    uint32_t seed_sign;
    vector<vector<int>> table;

    static int signFunction(int item, uint32_t seed) {
        return (hash(item, seed) & 1) ? 1 : -1;
    }

public:

    CS(int width, int depth, uint32_t seed_index, uint32_t seed_sign)
        : width(width), depth(depth), seed_index(seed_index), seed_sign(seed_sign) {
        table.assign(depth, vector<int>(width, 0));
    }

    void update(int item, int count) override {
        for (int i = 0; i < depth; ++i) {
            uint32_t hashValue = hash(item, seed_index + i) % width;
            int sign = signFunction(item, seed_sign + i);
            table[i][hashValue] += sign * count;
        }
    }

    double query(int item) const override {
        vector<int> estimates;
        for (int i = 0; i < depth; ++i) {
            uint32_t hashValue = hash(item, seed_index + i) % width;
            int sign = signFunction(item, seed_sign + i);
            estimates.push_back(sign * table[i][hashValue]);
        }

        sort(estimates.begin(), estimates.end());
        return estimates[depth / 2];
    }
};


#endif //COUNTSKETCH_H
