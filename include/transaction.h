#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <vector>

struct TxInput {
    std::string txId;
    int outputIndex;
    std::string address;
};

struct TxOutput {
    std::string address;
    double amount;
};

class Transaction {
public:
    std::vector<TxInput> inputs;
    std::vector<TxOutput> outputs;

    std::string id;

    Transaction();
};

#endif
