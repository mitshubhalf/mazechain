#ifndef UTXO_H
#define UTXO_H

#include <string>
#include <unordered_map>
#include <vector>

// Estrutura UTXO expandida para suporte a maturidade de recompensas
struct UTXO {
    std::string txid;
    int vout_index;
    std::string address;
    double amount;

    // Novos campos para controle de regras da blockchain
    bool isCoinbase;   // Identifica se a moeda veio de mineração
    int blockHeight;   // Altura do bloco em que a moeda foi gerada
};

class UTXOSet {
public:
    // Mapeia "txid:index" para o objeto UTXO (Busca O(1))
    std::unordered_map<std::string, UTXO> utxoMap;

    // Cache de saldos por endereço para o Dashboard carregar instantâneo
    std::unordered_map<std::string, double> addressBalances;

    // Ajustado: agora recebe o blockHeight do bloco processado
    void update(const struct Transaction& tx, int blockHeight);

    // Ajustado: agora recebe a altura atual da rede para calcular moedas maturadas
    double getBalance(const std::string& address, int currentHeight);

    void saveToFile(const std::string& filename);
    void loadFromFile(const std::string& filename);

private:
    const double EPSILON = 0.000000001;
};

#endif