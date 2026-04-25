#ifndef VALIDATION_H
#define VALIDATION_H

#include "block.h"
#include <string>

namespace Validation {
    /**
     * @brief Valida se o bloco atende à dificuldade mínima exigida pela rede.
     * Após o bloco 5000, o piso de dificuldade é fixado em 5.
     */
    bool CheckProofOfWork(const std::string& hash, int height);

    /**
     * @brief Valida o bloco completo contra Checkpoints, integridade de hash e PoW.
     */
    bool ValidateBlock(const Block& block);
}

#endif
