#include "../include/transaction.h"

Transaction::Transaction() {
    from = "";
    to = "";
    amount = 0;
}

Transaction::Transaction(std::string from, std::string to, int amount) {
    this->from = from;
    this->to = to;
    this->amount = amount;
}

std::string Transaction::toString() const {
    return from + "->" + to + ":" + std::to_string(amount);
}
