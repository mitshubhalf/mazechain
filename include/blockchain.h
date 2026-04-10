#ifndef BLOCKCHAIN_H
#define BLOCKCHAIN_H

#include <vector>
#include <string>

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
    std::string signature; // Assinatura DER em Hex
    std::string publicKey; // Chave pública para o minerador verificar
};

// ... (Restante das definições de classe)
#endif
