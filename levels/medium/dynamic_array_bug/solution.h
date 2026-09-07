#pragma once
#include <cstddef>
#include <stdexcept>

// This file already compiles and mostly works. Find and fix the ONE bug
// described in problem.md — do not rewrite the class from scratch.

class DynamicArray {
public:
    explicit DynamicArray(size_t size) : size_(size), data_(new int[size]{}) {}

    DynamicArray(const DynamicArray& other)
        : size_(other.size_), data_(other.data_) {
        // BUG: this just copies the pointer - both objects now point at
        // the same underlying buffer instead of each owning their own.
    }

    DynamicArray& operator=(const DynamicArray& other) {
        size_ = other.size_;
        data_ = other.data_;
        // BUG: same issue as the copy constructor above.
        return *this;
    }

    ~DynamicArray() {
        delete[] data_;
    }

    int& at(size_t index) {
        if (index >= size_) throw std::out_of_range("index out of range");
        return data_[index];
    }

    const int& at(size_t index) const {
        if (index >= size_) throw std::out_of_range("index out of range");
        return data_[index];
    }

    size_t size() const { return size_; }

private:
    size_t size_;
    int* data_;
};
