#pragma once

// Working for single-object use, but unsafe to copy or move - see
// problem.md. Refactor this to be safe under copying and moving without
// changing the public interface (constructor, get, set, destructor).

class ResourceHandle {
public:
    explicit ResourceHandle(int value) : data_(new int(value)) {}

    ~ResourceHandle() {
        delete data_;
    }

    int get() const { return *data_; }
    void set(int value) { *data_ = value; }

private:
    int* data_;
    // No copy constructor, copy assignment, move constructor, or move
    // assignment are declared - the compiler-generated defaults for a
    // raw-pointer-owning class are exactly what's unsafe here.
};
