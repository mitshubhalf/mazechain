#ifndef UTXO_H
#define UTXO_H

#include <string>
#include <vector>

// Forward declaration: Avisa que a struct existe antes de tentar usar
struct Transaction; 

struct UTXO {
    std::string txid;
    int vout_index;
    std::string address;
    double amount;
};

// Agora sim, a classe que usa a Transaction
class UTXOSet {
public:
    std::vector<UTXO> utxos;
    void update(const Transaction& tx); // O compilador agora sabe que Transaction é um tipo
    double getBalance(std::string address);
    void saveToFile(std::string filename);
    void loadFromFile(std::string filename);
};

#endif
