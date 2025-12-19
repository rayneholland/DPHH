
#ifndef INDEXMINHEAP_H
#define INDEXMINHEAP_H

#pragma once
#include <vector>
#include <unordered_map>
#include <utility>
#include <stdexcept>

template<typename KeyT, typename ValT>
class IndexMinHeap {
public:
    using Pair = std::pair<KeyT, ValT>;

    explicit IndexMinHeap(size_t capacity) : cap(capacity) {}

    bool contains(const KeyT& key) const {
        return index_map.find(key) != index_map.end();
    }

    size_t size() const { return heap.size(); }
    bool full() const { return heap.size() >= cap; }

    const Pair& top() const {
        if (heap.empty()) throw std::runtime_error("Heap empty");
        return heap[0];
    }

    ValT min_value() const {
        if (heap.empty()) throw std::runtime_error("Heap empty");
        return heap[0].second;
    }

    const std::vector<Pair>& items() const { return heap; }

    void insert(const KeyT& key, ValT val) {
        if (contains(key)) {
            update(key, val);
            return;
        }
        if (full()) throw std::runtime_error("Heap full");
        heap.push_back({key, val});
        int idx = heap.size() - 1;
        index_map[key] = idx;
        heapify_up(idx);
    }

    void update(const KeyT& key, ValT val) {
        auto it = index_map.find(key);
        if (it == index_map.end()) return;
        int idx = it->second;
        heap[idx].second = val;
        heapify_down(idx);
        heapify_up(idx);
    }

    void replace_top(const KeyT& key, ValT val) {
        if (heap.empty()) {
            insert(key, val);
            return;
        }
        index_map.erase(heap[0].first);
        heap[0] = {key, val};
        index_map[key] = 0;
        heapify_down(0);
    }

private:
    size_t cap;
    std::vector<Pair> heap;                 // binary heap
    std::unordered_map<KeyT, int> index_map; // key → heap index

    void swap_nodes(int i, int j) {
        std::swap(heap[i], heap[j]);
        index_map[heap[i].first] = i;
        index_map[heap[j].first] = j;
    }

    void heapify_up(int idx) {
        while (idx > 0) {
            int parent = (idx - 1) / 2;
            if (heap[parent].second <= heap[idx].second) break;
            swap_nodes(parent, idx);
            idx = parent;
        }
    }

    void heapify_down(int idx) {
        int n = heap.size();
        while (true) {
            int left = 2 * idx + 1, right = 2 * idx + 2, smallest = idx;
            if (left < n && heap[left].second < heap[smallest].second) smallest = left;
            if (right < n && heap[right].second < heap[smallest].second) smallest = right;
            if (smallest == idx) break;
            swap_nodes(idx, smallest);
            idx = smallest;
        }
    }
};


#endif //INDEXMINHEAP_H
