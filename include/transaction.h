#pragma once
#include <string>

struct Transaction {
    std::string from;
    std::string to;
    double amount;
    std::string signature;

    std::string toString() const {
        return from + to + std::to_string(amount);
    }
};
