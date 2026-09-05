#pragma once
#include <cstddef>
#include <stdexcept>
#include <vector>

// TODO: implement Stack<T> per problem.md
// Edit this file via the UI or your own editor, then click "Run Tests".

template <typename T>
class Stack {
public:
    void push(const T& value) {
        // TODO
    }

    void push(T&& value) {
        // TODO
    }

    T pop() {
        // TODO
        throw std::out_of_range("not implemented");
    }

    T& top() {
        // TODO
        throw std::out_of_range("not implemented");
    }

    bool empty() const {
        // TODO
        return true;
    }

    size_t size() const {
        // TODO
        return 0;
    }

private:
    std::vector<T> data_;
};
