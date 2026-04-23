#ifndef UTXO_H
#define UTXO_H

#include <string>
#include <unordered_map>
#include <vector>

struct UTXO {
    std::string txid;
    int vout_index;
    std::string address;
    double amount;
};

class UTXOSet {
public:
    // Mapeia "txid:index" para o objeto UTXO (Busca O(1))
    std::unordered_map<std::string, UTXO> utxoMap;

    // Cache de saldos por endereço para o Dashboard carregar instantâneo
    std::unordered_map<std::string, double> addressBalances;

    void update(const struct Transaction& tx);
    double getBalance(const std::string& address);
    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);

private:
    const double EPSILON = 0.000000001;
};

#endif