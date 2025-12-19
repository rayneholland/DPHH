

#ifndef CMSSO_H
#define CMSSO_H

#include <vector>
#include <cstdint>
#include <algorithm>

#include "Sketch.h"

using namespace std;

class CMSSO : public Sketch {

private:
    int width;
    int depth;
    uint32_t seed;
    vector<vector<double>> table;

public:

    CMSSO(int width, int depth, double epsilon,  uint32_t seed)
    : width(width), depth(depth), seed(seed) {
        table = vector<vector<double>>(depth, vector<double>(width, 0));
        for (int i = 0; i < depth; i++) {
            for (int j = 0; j < width; j++) {
                table[i][j] = laplaceNoise(epsilon, 2*depth);
            }
        }
    }

    void update(int item, int count) override {
        for (int i = 0; i < depth; ++i) {
            uint32_t hashValue = hash(item, seed+i) % width;
            table[i][hashValue] += count;
        }
    }

    double update_estimate(int item, int count) {
        double estimate = std::numeric_limits<double>::max();

        for (int i = 0; i < depth; ++i) {
            uint32_t hashValue = hash(item, seed + i) % width;
            table[i][hashValue] += count;
            estimate = min(estimate, table[i][hashValue]);
        }
        return estimate;
    }

    double query(int item) const override {
        double minCount = INT_MAX;
        for (int i = 0; i < depth; ++i) {
            uint32_t hashValue = hash(item, seed+i) % width;
            double estimate = table[i][hashValue];
            minCount = (minCount < estimate) ? minCount : estimate;
        }
        return minCount;
    }

    void printTable() const {
        for (int i = 0; i < depth; ++i) {
            for (int j = 0; j < width; ++j) {
                cout << table[i][j] << " ";
            }
            printf("Table width: %llu", table[i].size());
            cout << std::endl;
        }
    }
};


#endif //CMSSO_H
