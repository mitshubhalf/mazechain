#ifndef UTXO_H
#define UTXO_H

#include <string>
#include <vector>

struct UTXO {
    std::string txid;    // ID da transação que gerou este valor
    int vout_index;      // Posição no vetor de saídas
    std::string address; // Dono da moeda
    double amount;       // Valor
};

class UTXOSet {
public:
    std::vector<UTXO> utxos;

    // Adiciona novos UTXOs e remove os que foram gastos
    void update(const Transaction& tx);
    
    // Calcula saldo rápido
    double getBalance(std::string address);
    
    // Persistência
    void saveToFile(std::string filename);
    void loadFromFile(std::string filename);
};

#endif
