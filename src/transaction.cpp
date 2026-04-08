#include "../include/transaction.h"
#include <sstream>

Transaction::Transaction() {
    id = "";
}

std::string Transaction::toString() const {
    std::stringstream ss;

    ss << "Transaction ID: " << id << "\n";

    for (const auto& in : inputs) {
        ss << "  Input: " << in.txId << ":" << in.outputIndex
           << " (" << in.address << ")\n";
    }

    for (const auto& out : outputs) {
        ss << "  Output: " << out.address << " -> " << out.amount << "\n";
    }

    return ss.str();
}
