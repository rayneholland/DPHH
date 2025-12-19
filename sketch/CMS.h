

#ifndef COUNTMINSKETCH_H
#define COUNTMINSKETCH_H

#include <vector>
#include <cstdint>
#include <algorithm>
#include <limits>

#include "Sketch.h"

using namespace std;

class CMS : public Sketch {

private:
    int width;
    int depth;
    uint32_t seed;
    vector<vector<int>> table;

public:

    CMS(int width, int depth, uint32_t seed)
    : width(width), depth(depth), seed(seed) {
        table = vector<vector<int>>(depth, vector<int>(width, 0));
    }

    void update(int item, int count) override {
        for (int i = 0; i < depth; ++i) {
            uint32_t hashValue = hash(item, seed+i) % width;
            table[i][hashValue] += count;
        }
    }

    double query(int item) const override {
        int minCount = INT_MAX;
        for (int i = 0; i < depth; ++i) {
            int hashValue = hash(item, seed+i) % width;
            int estimate = table[i][hashValue];
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


#endif //COUNTMINSKETCH_H
