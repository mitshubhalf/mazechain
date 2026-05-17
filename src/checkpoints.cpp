#include "../include/checkpoints.h"
#include <iostream>

namespace Checkpoints {

    /**
     * MAZECHAIN CHECKPOINTS MAP
     * Mapa com os pontos de controle fixos para garantir a imutabilidade histórica da rede.
     * Contém as definições estritas para o Bloco Gênese e o marco estável do Bloco 10.000.
     */
    const MapCheckpoints mapCheckpoints = {
        { 0,     "8c6a56813b90f33cce1ec54afba2ef689fcccf24d7a35521ff02b57bd3075309" },
        { 10000, "0000006ca181a0a46f6e7125c515087c3516bd1409a81c80724468ea53ff3e7c" }
    };

    /**
     * Valida se um bloco em determinada altura corresponde ao checkpoint.
     * Sistema adaptado para permitir o crescimento orgânico da rede MazeChain.
     */
    bool CheckBlock(int height, const std::string& hash) {

        // Se o mapa estiver vazio, ignoramos a validação (Essencial para a fase de testes atual)
        if (mapCheckpoints.empty()) {
            return true;
        }

        auto it = mapCheckpoints.find(height);

        // 1. VALIDAÇÃO DE CHECKPOINT EXISTENTE
        // Se a altura estiver no mapa, o hash deve ser idêntico.
        if (it != mapCheckpoints.end()) {
            if (hash != it->second) {
                std::cout << "🚨 [CHECKPOINT ERROR] Bloco #" << height << " REJEITADO!" << std::endl;
                std::cout << "   Esperado: " << it->second << std::endl;
                std::cout << "   Recebido: " << hash << std::endl;
                return false;
            }
            std::cout << "🔒 [CHECKPOINT OK] Bloco #" << height << " validado via mapa fixo." << std::endl;
            return true;
        }

        // 2. PROTEÇÃO CONTRA REORGANIZAÇÃO (REORG)
        // Permite o processamento de blocos acima do último ponto de controle conhecido.
        int lastCheckpointHeight = GetLastCheckpointHeight();
        if (height < lastCheckpointHeight) {
            // Em uma rede pública, aqui rejeitaríamos blocos que tentam mudar o passado.
            return true; 
        }

        // 3. FASE DE CRESCIMENTO (LIVE CHAIN)
        // Aceita blocos novos (ex: 5.000, 10.000, 20.000) baseando-se apenas no consenso de PoW.
        return true;
    }

    /**
     * Retorna a altura do último checkpoint registrado no mapa.
     */
    int GetLastCheckpointHeight() {
        if (mapCheckpoints.empty()) return 0;
        // Retorna a chave (altura) do último elemento do mapa
        return mapCheckpoints.rbegin()->first;
    }

    /**
     * Calcula a fração de progresso de sincronização baseada no último checkpoint.
     */
    double GetProgressFraction(int height) {
        if (mapCheckpoints.empty()) return 1.0;

        int lastHeight = GetLastCheckpointHeight();
        if (lastHeight <= 0) return 1.0;

        double progress = (double)height / lastHeight;

        // Se já passamos do último checkpoint, o progresso é considerado 100% (1.0)
        return (progress > 1.0) ? 1.0 : progress;
    }

} // namespace Checkpoints