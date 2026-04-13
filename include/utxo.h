#ifndef UTXO_H
#define UTXO_H

#include <string>
#include <vector>

// Estrutura para representar uma saída não gasta
struct UTXO {
    std::string txid;
    int vout_index;
    std::string address;
    double amount;
};

// Precisamos incluir o header que define a estrutura da Transação
#include "transaction.h"

class UTXOSet {
public:
    std::vector<UTXO> utxos;

    void update(const Transaction& tx);
    double getBalance(std::string address);
    void saveToFile(std::string filename);
    void loadFromFile(std::string filename);
};

#endif
