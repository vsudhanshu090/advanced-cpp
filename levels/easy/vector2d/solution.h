#pragma once
#include <cmath>

// TODO: implement Vector2D per problem.md

class Vector2D {
public:
    Vector2D(double x, double y) {
        // TODO
    }

    double x() const {
        // TODO
        return 0.0;
    }

    double y() const {
        // TODO
        return 0.0;
    }

    Vector2D operator+(const Vector2D& other) const {
        // TODO
        return Vector2D(0.0, 0.0);
    }

    Vector2D operator-(const Vector2D& other) const {
        // TODO
        return Vector2D(0.0, 0.0);
    }

    Vector2D operator*(double scalar) const {
        // TODO
        return Vector2D(0.0, 0.0);
    }

    double dot(const Vector2D& other) const {
        // TODO
        return 0.0;
    }

    double magnitude() const {
        // TODO
        return 0.0;
    }

    bool operator==(const Vector2D& other) const {
        // TODO
        return false;
    }

private:
    double x_ = 0.0;
    double y_ = 0.0;
};
