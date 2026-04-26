#include "../include/checkpoints.h"

namespace Checkpoints {

    bool CheckBlock(int height, const std::string& hash) {
        // Busca se existe um checkpoint definido para esta altura específica
        auto it = mapCheckpoints.find(height);

        // Se não existir checkpoint para esta altura, o bloco é considerado válido por enquanto
        if (it == mapCheckpoints.end()) {
            return true; 
        }

        // Se existir, o hash PRECISA ser exatamente igual ao do mapa
        return (hash == it->second);
    }

    int GetLastCheckpointHeight() {
        if (mapCheckpoints.empty()) {
            return 0;
        }
        // Retorna a chave (altura) do último elemento do mapa
        return mapCheckpoints.rbegin()->first;
    }
}