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

struct Transaction {
    std::string id;
    std::string signature;
    std::string publicKey;
    std::vector<TxOut> vout;
    std::vector<TxIn> vin; // Para futura implementação de UTXO completo
};

#endif
