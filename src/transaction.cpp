#include "../include/transaction.h"
#include <sstream>

// converter para string
std::string Transaction::toString() const {
    std::stringstream ss;
    ss << from << "->" << to << ":" << amount;
    return ss.str();
}
