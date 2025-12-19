
#ifndef CSSO_H
#define CSSO_H


#include <vector>
#include <cstdint>
#include <algorithm>
#include <iostream>

#include "Sketch.h"

using namespace std;

class CSSO : public Sketch {

private:
    int width;
    int depth;
    uint32_t seed_index;
    uint32_t seed_sign;
    vector<vector<double>> table;

    static int signFunction(int item, uint32_t seed) {
        return (hash(item, seed) & 1) ? 1 : -1;
    }

public:

    CSSO(int width, int depth, double epsilon,  uint32_t seed_index, uint32_t seed_sign)
    : width(width), depth(depth), seed_index(seed_index), seed_sign(seed_sign) {
        table = vector<vector<double>>(depth, vector<double>(width, 0));
        for (int i = 0; i < depth; i++) {
            for (int j = 0; j < width; j++) {
                table[i][j] = laplaceNoise(epsilon, 2*depth);
            }
        }
    }


    void update(int item, int count) override {
        for (int i = 0; i < depth; ++i) {
            uint32_t hashValue = hash(item, seed_index + i) % width;
            int sign = signFunction(item, seed_sign + i);
            table[i][hashValue] += sign * count;
        }
    }

    double update_estimate(int item, int count) {
        vector<int> estimates(depth);

        for (int i = 0; i < depth; ++i) {
            uint32_t hashValue = hash(item, seed_index + i) % width;
            int sign = signFunction(item, seed_sign + i);

            table[i][hashValue] += sign * count;
            estimates[i] = sign * table[i][hashValue];
        }

        nth_element(
            estimates.begin(),
            estimates.begin() + depth / 2,
            estimates.end()
        );
        return estimates[depth / 2];
    }

    [[nodiscard]] double query(int item) const override {
        vector<int> estimates;
        for (int i = 0; i < depth; ++i) {
            uint32_t hashValue = hash(item, seed_index + i) % width;
            int sign = signFunction(item, seed_sign + i);
            estimates.push_back(sign * table[i][hashValue]);
        }

        sort(estimates.begin(), estimates.end());
        return estimates[depth / 2];
    }
    
    [[nodiscard]] double queryF2() const {
        std::vector<double> rowEstimates;
        rowEstimates.reserve(depth);

        for (int r = 0; r < depth; ++r) {
            double Sr = 0.0;
            for (int b = 0; b < width; ++b) {
                double a = table[r][b];
                Sr += a * a;
            }
            rowEstimates.push_back(Sr);
        }

        const size_t mid = rowEstimates.size() / 2;
        nth_element(rowEstimates.begin(),
                         rowEstimates.begin() + mid,
                         rowEstimates.end());

        if (rowEstimates.size() % 2 == 1) {
            return rowEstimates[mid];
        } else {

            const double upper = rowEstimates[mid];
            nth_element(rowEstimates.begin(),
                             rowEstimates.begin() + mid - 1,
                             rowEstimates.end());
            const double lower = rowEstimates[mid - 1];
            return 0.5 * (lower + upper);
        }
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


#endif //CSSO_H
