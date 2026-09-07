#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "../../../framework/test_common.h"
#include "solution.h"

TEST_CASE("default-constructed account has zero balance") {
    BankAccount acc;
    CHECK(acc.balance() == doctest::Approx(0.0));
}

TEST_CASE("constructor accepts a positive initial balance") {
    BankAccount acc(100.0);
    CHECK(acc.balance() == doctest::Approx(100.0));
}

TEST_CASE("constructor throws on negative initial balance") {
    CHECK_THROWS_AS(BankAccount(-50.0), std::invalid_argument);
}

TEST_CASE("constructor accepts exactly zero") {
    CHECK_NOTHROW(BankAccount(0.0));
}

TEST_CASE("deposit increases balance") {
    BankAccount acc(10.0);
    acc.deposit(5.0);
    CHECK(acc.balance() == doctest::Approx(15.0));
}

TEST_CASE("multiple deposits accumulate correctly, in order") {
    BankAccount acc(0.0);
    acc.deposit(10.0);
    acc.deposit(20.0);
    acc.deposit(5.5);
    CHECK(acc.balance() == doctest::Approx(35.5));
}

TEST_CASE("deposit of zero throws invalid_argument") {
    BankAccount acc(10.0);
    CHECK_THROWS_AS(acc.deposit(0.0), std::invalid_argument);
}

TEST_CASE("deposit of negative amount throws invalid_argument") {
    BankAccount acc(10.0);
    CHECK_THROWS_AS(acc.deposit(-5.0), std::invalid_argument);
}

TEST_CASE("failed deposit leaves balance unchanged") {
    BankAccount acc(50.0);
    CHECK_THROWS_AS(acc.deposit(-1.0), std::invalid_argument);
    CHECK(acc.balance() == doctest::Approx(50.0));
}

TEST_CASE("withdraw decreases balance") {
    BankAccount acc(100.0);
    acc.withdraw(30.0);
    CHECK(acc.balance() == doctest::Approx(70.0));
}

TEST_CASE("withdrawing the full balance brings it to exactly zero") {
    BankAccount acc(42.0);
    acc.withdraw(42.0);
    CHECK(acc.balance() == doctest::Approx(0.0));
}

TEST_CASE("withdraw more than balance throws runtime_error") {
    BankAccount acc(20.0);
    CHECK_THROWS_AS(acc.withdraw(20.01), std::runtime_error);
}

TEST_CASE("withdraw from a zero balance throws runtime_error") {
    BankAccount acc(0.0);
    CHECK_THROWS_AS(acc.withdraw(1.0), std::runtime_error);
}

TEST_CASE("withdraw of zero throws invalid_argument, not runtime_error") {
    BankAccount acc(10.0);
    CHECK_THROWS_AS(acc.withdraw(0.0), std::invalid_argument);
}

TEST_CASE("withdraw of negative amount throws invalid_argument") {
    BankAccount acc(10.0);
    CHECK_THROWS_AS(acc.withdraw(-5.0), std::invalid_argument);
}

TEST_CASE("failed withdrawal (insufficient funds) leaves balance unchanged") {
    BankAccount acc(10.0);
    CHECK_THROWS_AS(acc.withdraw(50.0), std::runtime_error);
    CHECK(acc.balance() == doctest::Approx(10.0));
}

TEST_CASE("failed withdrawal (invalid amount) leaves balance unchanged") {
    BankAccount acc(10.0);
    CHECK_THROWS_AS(acc.withdraw(-1.0), std::invalid_argument);
    CHECK(acc.balance() == doctest::Approx(10.0));
}

TEST_CASE("interleaved deposits and withdrawals track balance correctly") {
    BankAccount acc(100.0);
    acc.deposit(50.0);      // 150
    acc.withdraw(30.0);     // 120
    acc.deposit(10.0);      // 130
    acc.withdraw(130.0);    // 0
    CHECK(acc.balance() == doctest::Approx(0.0));
}

TEST_CASE("balance() is callable on a const reference") {
    const BankAccount acc(75.0);
    CHECK(acc.balance() == doctest::Approx(75.0));
}

TEST_CASE("a failed operation followed by a valid one still works correctly") {
    BankAccount acc(10.0);
    CHECK_THROWS_AS(acc.withdraw(100.0), std::runtime_error);
    acc.deposit(5.0);
    CHECK(acc.balance() == doctest::Approx(15.0));
}
