#include "../include/validation.h"
#include "../include/checkpoints.h"
#include <iostream>

namespace Validation {

    bool CheckProofOfWork(const std::string& hash, int height) {
        // Regra Dinâmica Corrigida: 
        // Antes do bloco 5000: Aceita a dificuldade calculada pelo ajuste normal (mínimo 0 para validação ampla).
        // Depois do bloco 5000: Exige obrigatoriamente no mínimo 5 zeros (Piso de Segurança).
        int requiredZeros = (height > 5000) ? 5 : 0;
        
        if (requiredZeros == 0) return true;

        // Verifica se a string do hash começa com a quantidade necessária de zeros
        std::string target(requiredZeros, '0');
        return (hash.substr(0, requiredZeros) == target);
    }

    bool ValidateBlock(const Block& block) {
        // 1. Verificação de Checkpoints (DNA da rede - Proteção contra reescrita)
        if (!Checkpoints::CheckBlock(block.index, block.hash)) {
            std::cout << "🚨 [CRÍTICO] Violação de Checkpoint no bloco #" << block.index << "!" << std::endl;
            return false;
        }

        // 2. Verificação de Proof of Work (Piso de dificuldade mínima 5 após bloco 5000)
        if (!CheckProofOfWork(block.hash, block.index)) {
            std::cout << "⚠️ [REJEITADO] Bloco #" << block.index << " abaixo do piso de dificuldade (5)." << std::endl;
            return false;
        }

        // 3. Verificação de Integridade Matemática (Recalcula o Hash)
        if (block.hash != block.calculateHash()) {
            std::cout << "⚠️ [REJEITADO] Hash calculado não confere no bloco #" << block.index << std::endl;
            return false;
        }

        return true;
    }
}
