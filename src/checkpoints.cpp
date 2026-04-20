#include "../include/checkpoints.h"

namespace Checkpoints {
    bool CheckBlock(int height, const std::string& hash) {
        auto it = mapCheckpoints.find(height);
        if (it == mapCheckpoints.end()) return true; // Não há checkpoint para esta altura
        
        return hash == it->second; // Retorna true se o hash bater com o oficial
    }

    int GetLastCheckpointHeight() {
        if (mapCheckpoints.empty()) return 0;
        return mapCheckpoints.rbegin()->first;
    }
}
