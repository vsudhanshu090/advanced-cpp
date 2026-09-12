#pragma once
#include <cmath>

// TODO: implement Vector2D per problem.md

class Vector2D {
public:
    Vector2D(double x, double y) : x_(x), y_(y) {}

    double x() const {
        return this->x_;
    }

    double y() const {
        return this->y_;
    }

    Vector2D operator+(const Vector2D& other) const {
        return Vector2D(x_ + other.x_, y_ + other.y_);
    }

    Vector2D operator-(const Vector2D& other) const {
        return Vector2D(x_ - other.x_, y_ - other.y_);
    }

    Vector2D operator*(double scalar) const {
        return Vector2D(x_ * scalar, y_ * scalar);
    }

    double dot(const Vector2D& other) const {
        return x_ * other.x_ + y_ * other.y_;
    }

    double magnitude() const {
        return sqrt(x_ * x_ + y_ * y_);
    }

    bool operator==(const Vector2D& other) const {
        return (x_ == other.x_ && y_ == other.y_);
    }

private:
    double x_ = 0.0;
    double y_ = 0.0;
};
