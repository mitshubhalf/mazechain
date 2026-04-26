#include "../include/checkpoints.h"

namespace Checkpoints {

    /**
     * MAPA DE CHECKPOINTS (Estilo Bitcoin Core)
     * * Para resolver o erro de "Falha de Checkpoint", siga estes passos:
     * 1. Deixe o mapa vazio: { };
     * 2. Compile e rode o nó.
     * 3. Pegue o hash do bloco #0 no log do terminal.
     * 4. Cole o hash aqui no lugar do exemplo abaixo.
     */
    const MapCheckpoints mapCheckpoints = {
        // { altura, "hash_oficial" }
        { 0, "f174fa8fdb08a30b11009cd943ad97f54e6a6193ec0ffe383080958aa5486614" } // Substitua pelo hash real do seu terminal
    };

    bool CheckBlock(int height, const std::string& hash) {
        // Se o mapa estiver vazio, ignoramos a verificação (bom para testes iniciais)
        if (mapCheckpoints.empty()) return true;

        auto it = mapCheckpoints.find(height);

        // Se não houver um checkpoint definido para esta altura, 
        // o bloco é aceito (a validação de PoW fará o trabalho pesado)
        if (it == mapCheckpoints.end()) {
            return true;
        }

        // Se houver um checkpoint, o hash PRECISA ser idêntico
        if (hash != it->second) {
            return false;
        }

        return true;
    }

    int GetLastCheckpointHeight() {
        if (mapCheckpoints.empty()) return 0;

        // Retorna a chave (altura) do último elemento inserido no mapa
        return mapCheckpoints.rbegin()->first;
    }

    double GetProgressFraction(int height) {
        if (mapCheckpoints.empty()) return 0.0;

        int lastHeight = mapCheckpoints.rbegin()->first;
        if (lastHeight == 0) return 1.0;

        double progress = (double)height / lastHeight;
        return (progress > 1.0) ? 1.0 : progress;
    }
}