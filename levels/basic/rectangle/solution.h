#pragma once
#include <stdexcept>

// TODO: implement Rectangle per problem.md

class Rectangle {
public:
    Rectangle(double width, double height) : width_(width), height_(height) {
        if (width <= 0 || height <= 0)
            throw std::invalid_argument("Height or width is less than 0");
    }

    double area() const {
        return width_ * height_;
    }

    double perimeter() const {
        return 2*(width_ + height_);
    }

    double getHeight() const {
        return this->height_;
    }

    double getWidth() const {
        return this->width_;
    }

    bool operator==(const Rectangle& other) const {
        return (other.getHeight() == this->height_ && other.getWidth() == this->width_);
    }

private:
    double width_ = 0.0;
    double height_ = 0.0;
};
