#include "../include/transaction.h"
#include <sstream>

Transaction::Transaction() {
    from = "";
    to = "";
    amount = 0.0;
}

Transaction::Transaction(std::string from, std::string to, double amount) {
    this->from = from;
    this->to = to;
    this->amount = amount;
}

std::string Transaction::toString() const {
    std::stringstream ss;
    ss << from << "->" << to << ":" << amount;
    return ss.str();
}
