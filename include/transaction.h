#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <sstream>

struct Transaction {
    std::string from;
    std::string to;
    int amount;

    std::string signature;
    std::string publicKey; // 🔥 NOVO

    std::string toString() const {
        std::stringstream ss;
        ss << from << to << amount;
        return ss.str();
    }
};

#endif
