#ifndef MAZECHAIN_COINS_H
#define MAZECHAIN_COINS_H

#include <cstdint>
#include <vector>
#include <string>

// Estrutura simplificada para representar uma saída de transação
struct CTxOut {
    std::string address;
    double amount;
};

/**
 * Coin: Representa uma entrada no conjunto UTXO
 */
class Coin {
public:
    CTxOut out;
    uint32_t nHeight;
    bool fCoinBase;

    Coin() : nHeight(0), fCoinBase(false) {}
    Coin(CTxOut outIn, uint32_t nHeightIn, bool fCoinBaseIn) : 
        out(outIn), nHeight(nHeightIn), fCoinBase(fCoinBaseIn) {}

    // Serialização manual simples (sem depender de serialize.h externo)
    bool IsSpent() const { return out.amount <= 0; }
};

#endif // MAZECHAIN_COINS_H