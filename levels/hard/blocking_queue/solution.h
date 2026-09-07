#pragma once
#include <cstddef>
#include <queue>
#include <mutex>
#include <condition_variable>

// TODO: implement BlockingQueue<T> per problem.md

template <typename T>
class BlockingQueue {
public:
    explicit BlockingQueue(size_t capacity) : capacity_(capacity) {
        // TODO
    }

    void push(T value) {
        // TODO: block while full, then insert
    }

    T pop() {
        // TODO: block while empty, then remove and return the front item
        return T();
    }

    size_t size() const {
        // TODO
        return 0;
    }

    bool empty() const {
        // TODO
        return true;
    }

private:
    size_t capacity_;
    std::queue<T> data_;
    // TODO: add a mutex and condition variable(s)
};
