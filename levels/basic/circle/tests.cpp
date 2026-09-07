#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../../framework/test_common.h"
#include "solution.h"

namespace {
constexpr double kPi = 3.14159265358979323846;
}

TEST_CASE("area computes pi * r^2") {
    Circle c(2.0);
    CHECK(c.area() == doctest::Approx(kPi * 4.0).epsilon(0.001));
}

TEST_CASE("circumference computes 2 * pi * r") {
    Circle c(2.0);
    CHECK(c.circumference() == doctest::Approx(2 * kPi * 2.0).epsilon(0.001));
}

TEST_CASE("works with non-integer radius") {
    Circle c(1.5);
    CHECK(c.area() == doctest::Approx(kPi * 2.25).epsilon(0.001));
    CHECK(c.circumference() == doctest::Approx(2 * kPi * 1.5).epsilon(0.001));
}

TEST_CASE("zero radius throws invalid_argument") {
    CHECK_THROWS_AS(Circle(0.0), std::invalid_argument);
}

TEST_CASE("negative radius throws invalid_argument") {
    CHECK_THROWS_AS(Circle(-3.0), std::invalid_argument);
}

TEST_CASE("very small positive radius does not throw") {
    CHECK_NOTHROW(Circle(0.0001));
}

TEST_CASE("equal radii compare equal") {
    Circle a(5.0);
    Circle b(5.0);
    CHECK(a == b);
}

TEST_CASE("different radii do not compare equal") {
    Circle a(5.0);
    Circle b(5.1);
    CHECK_FALSE(a == b);
}

TEST_CASE("a circle equals itself") {
    Circle a(9.0);
    CHECK(a == a);
}

TEST_CASE("methods are usable through a const reference") {
    const Circle c(4.0);
    const Circle same(4.0);
    CHECK(c.area() == doctest::Approx(kPi * 16.0).epsilon(0.001));
    CHECK(c.circumference() == doctest::Approx(8 * kPi).epsilon(0.001));
    CHECK(c == same);
}

TEST_CASE("area and circumference scale correctly with radius") {
    Circle small(1.0);
    Circle big(2.0);
    // doubling radius quadruples area, doubles circumference
    CHECK(big.area() == doctest::Approx(small.area() * 4.0).epsilon(0.001));
    CHECK(big.circumference() == doctest::Approx(small.circumference() * 2.0).epsilon(0.001));
}
