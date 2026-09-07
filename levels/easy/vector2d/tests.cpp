#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../../framework/test_common.h"
#include "solution.h"

TEST_CASE("x() and y() return the constructed components") {
    Vector2D v(3.0, 4.0);
    CHECK(v.x() == doctest::Approx(3.0));
    CHECK(v.y() == doctest::Approx(4.0));
}

TEST_CASE("operator+ adds components pairwise") {
    Vector2D a(1.0, 2.0);
    Vector2D b(3.0, 5.0);
    Vector2D c = a + b;
    CHECK(c.x() == doctest::Approx(4.0));
    CHECK(c.y() == doctest::Approx(7.0));
}

TEST_CASE("operator+ does not modify either operand") {
    Vector2D a(1.0, 2.0);
    Vector2D b(3.0, 5.0);
    Vector2D c = a + b;
    (void)c;
    CHECK(a.x() == doctest::Approx(1.0));
    CHECK(a.y() == doctest::Approx(2.0));
    CHECK(b.x() == doctest::Approx(3.0));
    CHECK(b.y() == doctest::Approx(5.0));
}

TEST_CASE("operator- subtracts components pairwise") {
    Vector2D a(5.0, 9.0);
    Vector2D b(2.0, 4.0);
    Vector2D c = a - b;
    CHECK(c.x() == doctest::Approx(3.0));
    CHECK(c.y() == doctest::Approx(5.0));
}

TEST_CASE("operator* scales both components") {
    Vector2D a(2.0, -3.0);
    Vector2D c = a * 4.0;
    CHECK(c.x() == doctest::Approx(8.0));
    CHECK(c.y() == doctest::Approx(-12.0));
}

TEST_CASE("operator* by zero yields the zero vector") {
    Vector2D a(7.0, -2.0);
    Vector2D c = a * 0.0;
    CHECK(c.x() == doctest::Approx(0.0));
    CHECK(c.y() == doctest::Approx(0.0));
}

TEST_CASE("dot product of perpendicular vectors is zero") {
    Vector2D a(1.0, 0.0);
    Vector2D b(0.0, 1.0);
    CHECK(a.dot(b) == doctest::Approx(0.0));
}

TEST_CASE("dot product matches manual computation") {
    Vector2D a(2.0, 3.0);
    Vector2D b(4.0, -1.0);
    CHECK(a.dot(b) == doctest::Approx(2.0 * 4.0 + 3.0 * -1.0));
}

TEST_CASE("magnitude of a 3-4-5 triangle vector is 5") {
    Vector2D v(3.0, 4.0);
    CHECK(v.magnitude() == doctest::Approx(5.0));
}

TEST_CASE("magnitude of the zero vector is zero") {
    Vector2D v(0.0, 0.0);
    CHECK(v.magnitude() == doctest::Approx(0.0));
}

TEST_CASE("operator== compares both components") {
    Vector2D a(1.0, 2.0);
    Vector2D b(1.0, 2.0);
    Vector2D c(1.0, 3.0);
    CHECK(a == b);
    CHECK_FALSE(a == c);
}

TEST_CASE("chained operations compose correctly: (a + b) * 2") {
    Vector2D a(1.0, 1.0);
    Vector2D b(2.0, 3.0);
    Vector2D result = (a + b) * 2.0;
    CHECK(result.x() == doctest::Approx(6.0));
    CHECK(result.y() == doctest::Approx(8.0));
}

TEST_CASE("all methods are usable through a const reference") {
    const Vector2D a(3.0, 4.0);
    const Vector2D b(1.0, 1.0);
    CHECK(a.x() == doctest::Approx(3.0));
    CHECK((a + b).x() == doctest::Approx(4.0));
    CHECK((a - b).y() == doctest::Approx(3.0));
    CHECK((a * 2.0).x() == doctest::Approx(6.0));
    CHECK(a.dot(b) == doctest::Approx(7.0));
    CHECK(a.magnitude() == doctest::Approx(5.0));
    CHECK(a == a);
}
