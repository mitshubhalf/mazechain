#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

struct Transaction {
    std::string from;
    std::string to;
    double amount;

    Transaction();
    Transaction(std::string from, std::string to, double amount);

    std::string toString() const;
};

#endif
