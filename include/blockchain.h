#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>
#include <ctime>

struct TxOut {
    std::string address;
    double amount;
};

struct TxIn {
    std::string prevTxId;
    int outIndex;
};

struct Transaction {
    std::string id;
    std::vector<TxIn> vin;
    std::vector<TxOut> vout;
    std::string signature; // <--- CAMPO ESSENCIAL PARA ECDSA
};

// ... (Restante das definições de Block e Blockchain) ...
// Adicione ou verifique se estas assinaturas existem na classe Blockchain:
// double getBalance(std::string address);
// bool verifyTransaction(const Transaction& tx);
#endif
