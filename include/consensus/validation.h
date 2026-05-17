#ifndef MAZECHAIN_VALIDATION_H
#define MAZECHAIN_VALIDATION_H

#include <arith_uint256.h>
// #include <attributes.h>       // Comentado se não houver o arquivo
// #include <chain.h>            // Comentado se causar erro de inclusão
// #include "checkqueue.h"       // Comentado se causar erro de inclusão
#include <coins.h>
// #include <consensus/amount.h>  // REMOVIDO: Causa erro de compilação
#include <consensus/params.h>
#include <uint256.h>
#include <string>
#include <memory>
#include <vector>

/** * DEFINIÇÃO MANUAL DE TIPOS (Para substituir o consensus/amount.h ausente)
 */
typedef int64_t CAmount;
static constexpr CAmount COIN = 100000000; // 1 MZ = 100,000,000 mits

/** CONSTANTES */
static constexpr CAmount MAZE_INITIAL_REWARD = 150 * COIN;
static constexpr int MAZE_HALVING_INTERVAL = 10000;
static constexpr CAmount MAZE_MAX_SUPPLY = 20000000 * COIN;
extern const std::string MAZE_RESERVE_FUND;

/**
 * 🔥 REGRA DE DIFICULDADE ATUALIZADA (DIFICULDADE 5 DESDE O BLOCO 1)
 */
inline unsigned int GetMazeRequiredDifficulty(int nHeight, const Consensus::Params& params) {

    // ============================================================
    // 🔥 NOVA REGRA: Dificuldade 5 Fixa até o bloco 10.000
    // ============================================================

    // FASE 1: Bloco 1 ao 9.999 -> Dificuldade 5
    if (nHeight < 10000) {
        return 5; 
    }

    // FASE 2: 10.000 ao 19.999 -> Dinâmica Progressiva
    if (nHeight >= 10000 && nHeight < 20000) {
        if (nHeight <= 15000) return 5; // Mantém 5 para estabilidade
        return 6;                       // Sobe para 6 no bloco 15k
    }

    // FASE 3: 20.000 em diante -> Salto para 7 e Retargeting Dinâmico
    if (nHeight >= 20000) {
        return 7;
    }

    return 5; // Fallback de segurança
}

/** Protótipos de Funções */
CAmount GetMazeBlockSubsidy(int nHeight, const Consensus::Params& params, const CAmount& nTotalSupply, const CAmount& nReserveBalance);

// Mock básico para CheckBlock e POW se os headers originais falharem
bool CheckProofOfWork(uint256 hash, unsigned int nBits, int nHeight, const Consensus::Params& params);

/**
 * ESTRUTURAS DE ESTADO (Simplificadas para permitir compilação)
 */
class Chainstate {
public:
    int nHeight;
    CAmount nChainSupply;

    // Conectores simplificados
    bool ConnectBlock(int height);
};

class ChainstateManager {
public:
    std::vector<std::unique_ptr<Chainstate>> m_chainstates;

    int GetSafetyFloor() const {
        return 10000; // Piso de segurança MazeChain
    }
};

#endif // MAZECHAIN_VALIDATION_H