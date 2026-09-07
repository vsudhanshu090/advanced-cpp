#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../../framework/test_common.h"
#include "solution.h"

TEST_CASE("area computes width * height") {
    Rectangle r(4.0, 5.0);
    CHECK(r.area() == doctest::Approx(20.0));
}

TEST_CASE("perimeter computes 2*(w+h)") {
    Rectangle r(4.0, 5.0);
    CHECK(r.perimeter() == doctest::Approx(18.0));
}

TEST_CASE("works with non-integer dimensions") {
    Rectangle r(2.5, 3.2);
    CHECK(r.area() == doctest::Approx(8.0));
    CHECK(r.perimeter() == doctest::Approx(11.4));
}

TEST_CASE("zero width throws invalid_argument") {
    CHECK_THROWS_AS(Rectangle(0.0, 5.0), std::invalid_argument);
}

TEST_CASE("zero height throws invalid_argument") {
    CHECK_THROWS_AS(Rectangle(5.0, 0.0), std::invalid_argument);
}

TEST_CASE("negative width throws invalid_argument") {
    CHECK_THROWS_AS(Rectangle(-1.0, 5.0), std::invalid_argument);
}

TEST_CASE("negative height throws invalid_argument") {
    CHECK_THROWS_AS(Rectangle(5.0, -1.0), std::invalid_argument);
}

TEST_CASE("both dimensions negative throws invalid_argument") {
    CHECK_THROWS_AS(Rectangle(-2.0, -3.0), std::invalid_argument);
}

TEST_CASE("equal rectangles compare equal") {
    Rectangle a(3.0, 4.0);
    Rectangle b(3.0, 4.0);
    CHECK(a == b);
}

TEST_CASE("different width means not equal, even with same area") {
    Rectangle a(2.0, 6.0);
    Rectangle b(3.0, 4.0);  // same area (12) but different dimensions
    CHECK_FALSE(a == b);
}

TEST_CASE("order matters: 2x3 is not equal to 3x2") {
    Rectangle a(2.0, 3.0);
    Rectangle b(3.0, 2.0);
    CHECK_FALSE(a == b);
}

TEST_CASE("a rectangle equals itself") {
    Rectangle a(7.0, 9.0);
    CHECK(a == a);
}

TEST_CASE("area, perimeter, and operator== all work through a const reference") {
    const Rectangle r(6.0, 7.0);
    const Rectangle same(6.0, 7.0);
    CHECK(r.area() == doctest::Approx(42.0));
    CHECK(r.perimeter() == doctest::Approx(26.0));
    CHECK(r == same);
}

TEST_CASE("very small positive dimensions do not throw") {
    CHECK_NOTHROW(Rectangle(0.0001, 0.0001));
}
