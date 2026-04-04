#include "../include/transaction.h"
#include <sstream>

// construtor vazio
Transaction::Transaction() {
    from = "";
    to = "";
    amount = 0;
}

// construtor principal
Transaction::Transaction(std::string from, std::string to, double amount) {
    this->from = from;
    this->to = to;
    this->amount = amount;
}

// converter para string
std::string Transaction::toString() const {
    std::stringstream ss;
    ss << from << "->" << to << ":" << amount;
    return ss.str();
}
