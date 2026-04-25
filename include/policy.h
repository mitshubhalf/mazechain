#ifndef POLICY_H
#define POLICY_H

#include <string>

namespace Policy {
    // Valor mínimo para uma transação (evita spam de frações irrelevantes)
    const double DUST_THRESHOLD = 0.00001;

    // O teto máximo de moedas da MazeChain (20 Milhões)
    const double MAX_SUPPLY = 20000000.0;

    /**
     * @brief Verifica se a taxa paga é adequada conforme a regra da era atual.
     * @param amount Valor líquido enviado.
     * @param fee Taxa oferecida.
     * @param currentFeePercent Percentual exigido pelo Blockchain::getDynamicFeePercentage.
     */
    bool IsFeeAdequate(double amount, double fee, double currentFeePercent);
    
    // Verifica se o valor enviado é maior que zero e menor que o supply total
    bool IsValidAmount(double amount);
}

#endif
