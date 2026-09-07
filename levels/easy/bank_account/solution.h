#pragma once
#include <stdexcept>

// TODO: implement BankAccount per problem.md

class BankAccount {
public:
    explicit BankAccount(double initial_balance = 0.0) : balance_(initial_balance) {
        if (initial_balance < 0)
            throw std::invalid_argument("initial balance is less than 0");
    }

    void deposit(double amount) {
        if (amount <= 0)
            throw std::invalid_argument("deposited amount is less than 0");
        this->balance_ += amount;
    }

    void withdraw(double amount) {
        if (this->balance_ - amount < 0)
            throw std::runtime_error("withdrawing amount is less than 0");
        if (amount <= 0)
            throw std::invalid_argument("not enough balance");
        this->balance_ -= amount;
    }

    double balance() const {
        return this->balance_;
    }

private:
    double balance_ = 0.0;
};
