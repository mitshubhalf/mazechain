#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>

struct Transaction {
    std::string from;
    std::string to;
    int amount;

    Transaction();
    Transaction(std::string from, std::string to, int amount);

    std::string toString() const;
};

#endif
