#pragma once
#include <cstddef>
#include <stdexcept>

// This file already compiles and mostly works. Find and fix the ONE bug
// described in problem.md — do not rewrite the class from scratch.

class DynamicArray {
public:
    explicit DynamicArray(size_t size) : size_(size), data_(new int[size]{}) {}

    DynamicArray(const DynamicArray& other) : size_(other.size_) {
        int* data = new int[size_]{};
        for (int i{};i<size_;i++)
            *(data+i) = *(other.data_ + i);
        data_ = data;
    }

    DynamicArray& operator=(const DynamicArray& other) {
        size_ = other.size_;
        int* data = new int[size_]{};
        for (int i{};i<size_;i++)
            *(data+i) = *(other.data_ + i);
        data_ = data;
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
