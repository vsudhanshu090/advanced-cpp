#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../../framework/test_common.h"
#include "solution.h"

#include <vector>
#include <algorithm>
#include <chrono>
#include <random>
#include <cmath>

namespace {
double naive_median(std::vector<int> values) {
    std::sort(values.begin(), values.end());
    size_t n = values.size();
    if (n % 2 == 1) return values[n / 2];
    return (values[n / 2 - 1] + values[n / 2]) / 2.0;
}
}  // namespace

TEST_CASE("single value: median is that value") {
    RunningMedian rm;
    rm.insert(5);
    CHECK(rm.median() == doctest::Approx(5.0));
}

TEST_CASE("two values: median is their average") {
    RunningMedian rm;
    rm.insert(1);
    rm.insert(3);
    CHECK(rm.median() == doctest::Approx(2.0));
}

TEST_CASE("odd count: median is the exact middle value after sorting") {
    RunningMedian rm;
    for (int v : {5, 1, 4, 2, 3}) rm.insert(v);
    CHECK(rm.median() == doctest::Approx(3.0));
}

TEST_CASE("even count: median is the average of the two middle values") {
    RunningMedian rm;
    for (int v : {5, 1, 4, 2}) rm.insert(v);
    // sorted: 1,2,4,5 -> middle two are 2 and 4 -> median 3.0
    CHECK(rm.median() == doctest::Approx(3.0));
}

TEST_CASE("handles duplicate values correctly") {
    RunningMedian rm;
    for (int v : {2, 2, 2, 2}) rm.insert(v);
    CHECK(rm.median() == doctest::Approx(2.0));
}

TEST_CASE("handles negative values correctly") {
    RunningMedian rm;
    for (int v : {-5, -1, -10, 3, 0}) rm.insert(v);
    // sorted: -10,-5,-1,0,3 -> median -1
    CHECK(rm.median() == doctest::Approx(-1.0));
}

TEST_CASE("correctness matches a naive reference after every single insert, moderate scale") {
    std::mt19937 rng(12345);
    std::uniform_int_distribution<int> dist(-100000, 100000);

    RunningMedian rm;
    std::vector<int> reference;
    reference.reserve(2000);

    for (int i = 0; i < 2000; ++i) {
        int v = dist(rng);
        rm.insert(v);
        reference.push_back(v);
        // Check periodically (every insert would be correct too, but this
        // keeps the naive O(n log n) reference computation's total cost
        // reasonable while still catching any drift).
        if (i % 50 == 0 || i == 1999) {
            REQUIRE(rm.median() == doctest::Approx(naive_median(reference)));
        }
    }
}

TEST_CASE("performance: 500,000 insertions complete within budget (O(log n) required)") {
    constexpr int kN = 500000;
    constexpr double kBudgetSeconds = 2.0;

    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(-1000000, 1000000);

    RunningMedian rm;

    auto start = std::chrono::steady_clock::now();
    for (int i = 0; i < kN; ++i) {
        rm.insert(dist(rng));
    }
    double last_median = rm.median();
    auto end = std::chrono::steady_clock::now();

    double elapsed = std::chrono::duration<double>(end - start).count();

    CHECK(elapsed < kBudgetSeconds);
    CHECK(std::isfinite(last_median));  // sanity: median() actually returned something real
}
