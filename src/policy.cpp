#include "../include/policy.h"
#include <cmath>
#include <iostream>

namespace Policy {
    bool IsFeeAdequate(double amount, double fee) {
        if (amount < DUST_THRESHOLD) return false;
        
        // Calcula 1% do valor enviado
        double requiredFee = amount * FEE_PERCENTAGE;
        
        // Comparamos com uma pequena margem de erro para floats
        if (fee < (requiredFee - 0.00000001)) {
            std::cout << "⚠️ [POLICY] Taxa insuficiente! Esperado: " << requiredFee << " MZ" << std::endl;
            return false;
        }
        
        return true;
    }

    bool IsValidAmount(double amount) {
        // Não pode ser negativo, nem zero, nem maior que o supply total da moeda
        if (amount <= 0) return false;
        if (amount > MAX_SUPPLY) {
            std::cout << "⚠️ [POLICY] Valor excede o supply total da rede!" << std::endl;
            return false;
        }
        return true;
    }
}
