#include "../include/checkpoints.h"

namespace Checkpoints {

    /**
     * MAPA DE CHECKPOINTS (Estilo Bitcoin Core)
     * Estes hashes servem como a "verdade absoluta" da rede.
     * Mesmo que alguém apague os dados, o nó só aceitará a chain se 
     * passar por esses pontos de verificação.
     */
    const MapCheckpoints mapCheckpoints = {
        // Bloco Genesis: O marco zero da sua MazeChain
        { 0, "8c6a56813b90f33cce1ec54afba2ef689fcccf24d7a35521ff02b57bd3075309" },

        // Checkpoint do Bloco 5000:
        // Assim que você minerar o bloco 5001 pela primeira vez e ele for aceito,
        // o hash do 5000 se tornará permanente aqui para proteção da rede.
        // { 5000, "COLE_AQUI_O_HASH_DO_NOVO_BLOCO_5000" }
    };

    bool CheckBlock(int height, const std::string& hash) {
        // Se o mapa estiver vazio, ignoramos a verificação
        if (mapCheckpoints.empty()) return true;

        auto it = mapCheckpoints.find(height);

        // Se não houver um checkpoint definido para esta altura, 
        // o bloco é aceito (a validação de PoW/Blockchain.cpp fará o resto)
        if (it == mapCheckpoints.end()) {
            return true;
        }

        // PROTEÇÃO: Se houver um checkpoint, o hash RECEBIDO precisa ser 
        // exatamente igual ao hash OFICIAL definido no mapa.
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
