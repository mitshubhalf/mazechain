#include "checkpoints.h"

namespace Checkpoints {
    // Atualize o valor abaixo com o hash que apareceu no seu console
    const MapCheckpoints mapCheckpoints = {
        { 0, "INSIRA_O_HASH_QUE_APARECEU_NO_CONSOLE_AQUI" }
    };

    bool CheckBlock(int height, const std::string& hash) {
        auto it = mapCheckpoints.find(height);
        if (it == mapCheckpoints.end()) return true; // Sem checkpoint para esta altura
        return hash == it->second;
    }

    int GetLastCheckpointHeight() {
        if (mapCheckpoints.empty()) return 0;
        return mapCheckpoints.rbegin()->first;
    }
}