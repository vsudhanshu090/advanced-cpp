---
title: BankAccount
topics: [Classes, Exception Safety, Encapsulation]
difficulty: easy
type: implement
---

## Task
Implement a `BankAccount` class in `solution.h` with the following public interface:

```cpp
class BankAccount {
public:
    explicit BankAccount(double initial_balance = 0.0);
    void deposit(double amount);
    void withdraw(double amount);
    double balance() const;
};
```

## Requirements
1. The constructor throws `std::invalid_argument` if `initial_balance < 0`.
2. `deposit(amount)` throws `std::invalid_argument` if `amount <= 0`. Otherwise
   it adds `amount` to the balance.
3. `withdraw(amount)` throws `std::invalid_argument` if `amount <= 0`.
4. `withdraw(amount)` throws `std::runtime_error` if `amount > balance()`
   (insufficient funds). Validate the amount first: a zero/negative amount
   must raise `std::invalid_argument` even if it also happens to exceed the
   balance — check argument validity before checking funds.
5. `balance()` must be callable on a `const BankAccount&` and must reflect
   every successful deposit/withdrawal so far, in order.
6. A **failed** `deposit` or `withdraw` call (one that throws) must leave the
   balance completely unchanged — no partial effects.

## Notes
- Floating point comparisons in the tests use a small tolerance, so exact
  binary equality isn't required — just correct accumulation.
