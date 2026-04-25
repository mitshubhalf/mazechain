#include "../include/policy.h"
#include <cmath>
#include <iostream>

namespace Policy {

    bool IsFeeAdequate(double amount, double fee, double currentFeePercent) {
        if (amount < DUST_THRESHOLD) return false;
        
        // Calcula a taxa baseada no percentual dinâmico da rede (0.01, 0.02, etc)
        double requiredFee = amount * currentFeePercent;
        
        // Comparamos com uma pequena margem de erro para precisão de ponto flutuante
        if (fee < (requiredFee - 0.00000001)) {
            std::cout << "⚠️ [POLICY] Taxa abaixo do consenso para esta Era! " 
                      << "Esperado: " << requiredFee << " MZ (" << (currentFeePercent * 100) << "%)" << std::endl;
            return false;
        }
        
        return true;
    }

    bool IsValidAmount(double amount) {
        // Bloqueia valores negativos ou zerados
        if (amount <= 0) return false;

        // Bloqueia valores que tentem movimentar mais do que jamais existirá
        if (amount > MAX_SUPPLY) {
            std::cout << "⚠️ [POLICY] Valor absurdo! Excede o supply total da rede." << std::endl;
            return false;
        }
        return true;
    }
}
