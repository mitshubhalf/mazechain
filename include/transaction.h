#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

struct Transaction {
    std::string from = "";
    std::string to = "";
    double amount = 0.0;

    Transaction() = default;

    Transaction(std::string from, std::string to, double amount)
        : from(from), to(to), amount(amount) {}

    std::string toString() const;
};

#endif
