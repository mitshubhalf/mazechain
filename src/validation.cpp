#include "../include/validation.h"
#include "../include/checkpoints.h"
#include <iostream>

namespace Validation {

    bool CheckProofOfWork(const std::string& hash, int height) {
        // Regra Dinâmica: Antes do 1000 aceita o que vier (mínimo 0), 
        // depois do 1000 exige no mínimo 5 zeros.
        int requiredZeros = (height > 1000) ? 5 : 0;
        
        if (requiredZeros == 0) return true;

        std::string target(requiredZeros, '0');
        return (hash.substr(0, requiredZeros) == target);
    }

    bool ValidateBlock(const Block& block) {
        // 1. Verificação de Checkpoints (Proteção contra reescrita da história)
        if (!Checkpoints::CheckBlock(block.index, block.hash)) {
            std::cout << "🚨 [CRÍTICO] Violação de Checkpoint no bloco #" << block.index << "!" << std::endl;
            return false;
        }

        // 2. Verificação de Proof of Work (Dificuldade mínima)
        if (!CheckProofOfWork(block.hash, block.index)) {
            std::cout << "⚠️ [REJEITADO] Bloco #" << block.index << " não atingiu a dificuldade mínima." << std::endl;
            return false;
        }

        // 3. Verificação de integridade do Hash
        if (block.hash != block.calculateHash()) {
            std::cout << "⚠️ [REJEITADO] Hash corrompido no bloco #" << block.index << std::endl;
            return false;
        }

        return true;
    }
}
