#ifndef MAZECHAIN_CHAIN_H
#define MAZECHAIN_CHAIN_H

#include <vector>
#include <stdint.h>
#include "uint256.h"

/** Estrutura que representa a posição de um bloco na corrente (em memória) */
class CBlockIndex {
public:
    uint256 phashBlock;
    CBlockIndex* pprev;
    int nHeight;
    unsigned int nBits;
    uint32_t nTime;

    CBlockIndex() {
        pprev = nullptr;
        nHeight = 0;
        nBits = 0;
        nTime = 0;
    }

    CBlockIndex(uint256 hashIn, int heightIn) : phashBlock(hashIn), nHeight(heightIn) {
        pprev = nullptr;
    }

    uint256 GetBlockHash() const { return phashBlock; }
};

#endif // MAZECHAIN_CHAIN_H
