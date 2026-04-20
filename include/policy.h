#ifndef POLICY_H
#define POLICY_H

#include <string>

namespace Policy {
    // Valor mínimo para uma transação (evita criar "lixo" na rede)
    const double DUST_THRESHOLD = 0.00001;
    
    // Sua regra de ouro: 1% de taxa
    const double FEE_PERCENTAGE = 0.01;

    // O teto máximo de moedas da MazeChain
    const double MAX_SUPPLY = 20000000.0;

    // Verifica se a taxa de 1% foi paga corretamente
    bool IsFeeAdequate(double amount, double fee);
    
    // Verifica se o valor enviado é realista (nem zero, nem acima do total)
    bool IsValidAmount(double amount);
}

#endif
