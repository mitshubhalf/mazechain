#include "../include/checkpoints.h"
#include <iostream>
#include <fstream>
#include <sstream>

namespace Checkpoints {

    // Bloco Gênesis: imutável, hardcoded para sempre
    const MapCheckpoints genesisCheckpoints = {
        { 0, "8c6a56813b90f33cce1ec54afba2ef689fcccf24d7a35521ff02b57bd3075309" }
    };

    // Mapa dinâmico: começa com o gênesis e cresce com cada halving
    static MapCheckpoints runtimeCheckpoints = {
        { 0, "8c6a56813b90f33cce1ec54afba2ef689fcccf24d7a35521ff02b57bd3075309" }
    };

    bool IsHalvingBlock(int height) {
        return (height > 0 && height % 10000 == 0);
    }

    void AddRuntimeCheckpoint(int height, const std::string& hash) {
        runtimeCheckpoints[height] = hash;
        std::cout << "🔒 [CHECKPOINT SALVO] Bloco #" << height
                  << " registrado como ponto seguro de halving." << std::endl;
    }

    void SaveCheckpoints(const std::string& path) {
        std::ofstream f(path);
        if (!f.is_open()) {
            std::cerr << "[CHECKPOINT] Falha ao salvar: " << path << std::endl;
            return;
        }
        for (const auto& kv : runtimeCheckpoints) {
            f << kv.first << " " << kv.second << "\n";
        }
    }

    void LoadCheckpoints(const std::string& path) {
        std::ifstream f(path);
        if (!f.is_open()) return; // arquivo ainda não existe — normal no primeiro boot

        int height;
        std::string hash;
        int count = 0;
        while (f >> height >> hash) {
            runtimeCheckpoints[height] = hash;
            count++;
        }
        if (count > 0) {
            std::cout << "🔒 [CHECKPOINTS] " << count << " ponto(s) seguro(s) carregado(s)." << std::endl;
        }
    }

    bool CheckBlock(int height, const std::string& hash) {
        auto it = runtimeCheckpoints.find(height);

        if (it != runtimeCheckpoints.end()) {
            if (hash != it->second) {
                std::cout << "🚨 [CHECKPOINT REJEITOU] Bloco #" << height << " inválido!" << std::endl;
                std::cout << "   Esperado : " << it->second << std::endl;
                std::cout << "   Recebido : " << hash << std::endl;
                return false;
            }
            std::cout << "🔒 [CHECKPOINT OK] Bloco #" << height << " validado." << std::endl;
            return true;
        }

        // Bloco acima do último checkpoint: aceito pelo consenso PoW
        return true;
    }

    int GetLastCheckpointHeight() {
        if (runtimeCheckpoints.empty()) return 0;
        return runtimeCheckpoints.rbegin()->first;
    }

    double GetProgressFraction(int height) {
        int lastHeight = GetLastCheckpointHeight();
        if (lastHeight <= 0) return 1.0;
        double progress = (double)height / lastHeight;
        return (progress > 1.0) ? 1.0 : progress;
    }

} // namespace Checkpoints
