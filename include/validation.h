#ifndef VALIDATION_H
#define VALIDATION_H

#include "block.h"
#include <string>

namespace Validation {
    // Valida se o bloco atende à dificuldade mínima (Piso 5 após bloco 1000)
    bool CheckProofOfWork(const std::string& hash, int height);

    // Valida o bloco completo contra Checkpoints e Regras de Consenso
    bool ValidateBlock(const Block& block);
}

#endif
