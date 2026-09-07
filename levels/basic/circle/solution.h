#pragma once
#include <stdexcept>

// TODO: implement Circle per problem.md

class Circle {
public:
    explicit Circle(double radius) : radius_(radius) {
        if (radius <= 0)
            throw std::invalid_argument("radius is non positive");        
    }

    double area() const {
        return pi * radius_ * radius_;
    }

    double circumference() const {
        return (double)2 * pi * radius_;
    }

    bool operator==(const Circle& other) const {
        return this->radius_ == other.radius_;
    }

private:
    static constexpr double pi = 3.14;
    double radius_ = 0.0;
};
