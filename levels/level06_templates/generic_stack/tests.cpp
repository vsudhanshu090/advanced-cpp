#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../../framework/doctest.h"
#include "solution.h"
#include <string>

// Move-counting helper type to verify push(T&&) actually moves
struct MoveCounter {
    int value;
    static inline int move_count = 0;
    static inline int copy_count = 0;

    MoveCounter(int v = 0) : value(v) {}
    MoveCounter(const MoveCounter& other) : value(other.value) { ++copy_count; }
    MoveCounter(MoveCounter&& other) noexcept : value(other.value) { ++move_count; }
    MoveCounter& operator=(const MoveCounter&) = default;
    MoveCounter& operator=(MoveCounter&&) noexcept = default;
};

TEST_CASE("push and top - basic int") {
    Stack<int> s;
    s.push(10);
    s.push(20);
    CHECK(s.top() == 20);
    CHECK(s.size() == 2);
    CHECK_FALSE(s.empty());
}

TEST_CASE("pop returns and removes top element, LIFO order") {
    Stack<int> s;
    s.push(1);
    s.push(2);
    s.push(3);
    CHECK(s.pop() == 3);
    CHECK(s.pop() == 2);
    CHECK(s.size() == 1);
    CHECK(s.pop() == 1);
    CHECK(s.empty());
}

TEST_CASE("works with std::string") {
    Stack<std::string> s;
    s.push("hello");
    s.push("world");
    CHECK(s.pop() == "world");
    CHECK(s.top() == "hello");
}

TEST_CASE("pop on empty stack throws out_of_range") {
    Stack<int> s;
    CHECK_THROWS_AS(s.pop(), std::out_of_range);
}

TEST_CASE("top on empty stack throws out_of_range") {
    Stack<std::string> s;
    CHECK_THROWS_AS(s.top(), std::out_of_range);
}

TEST_CASE("empty() and size() on freshly constructed stack") {
    Stack<double> s;
    CHECK(s.empty());
    CHECK(s.size() == 0);
}

TEST_CASE("push(T&&) actually moves, does not copy, for rvalues") {
    MoveCounter::move_count = 0;
    MoveCounter::copy_count = 0;

    Stack<MoveCounter> s;
    s.push(MoveCounter(42));   // rvalue -> must hit push(T&&) and move internally
    s.push(MoveCounter(7));

    CHECK(s.top().value == 7);
    CHECK(MoveCounter::move_count > 0);
    CHECK(MoveCounter::copy_count == 0);
}

TEST_CASE("push(const T&) copies for lvalues, top() reference is mutable") {
    Stack<int> s;
    int x = 5;
    s.push(x);          // lvalue -> copy path
    s.top() = 99;        // top() must return a mutable reference
    CHECK(s.pop() == 99);
}
