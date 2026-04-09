#ifndef TRANSACTION_H
#define TRANSACTION_H

#include <string>
#include <vector>

struct TxIn {
    std::string txid;
    int index;
};

struct TxOut {
    std::string address;
    double amount;
};

class Transaction {
public:
    std::string id;
    std::vector<TxIn> vin;
    std::vector<TxOut> vout;

    Transaction() {}
    Transaction(std::vector<TxIn> in, std::vector<TxOut> out);

    std::string calculateHash() const;
};

#endif
