#pragma once
#include <queue>
#include <vector>
#include <functional>

// TODO: implement RunningMedian per problem.md — must be O(log n) insert,
// O(1) median, or the performance test will time out on large input.

class RunningMedian {
public:
    void insert(int value) {
        // TODO
    }

    double median() const {
        // TODO
        return 0.0;
    }

private:
    // TODO: e.g. two heaps - a max-heap for the lower half, a min-heap
    // for the upper half, kept balanced in size after every insert.
};
