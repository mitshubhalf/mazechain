#include "../include/storage.h"
#include <fstream>
#include <iostream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename);

    if (!file.is_open()) return;

    for (const auto& block : bc.getChain()) {
        file << block.hash << "|";
        file << block.previousHash << "\n";

        for (const auto& tx : block.transactions) {
            file << tx.id << "\n";
        }

        file << "---\n";
    }

    file.close();
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename);

    if (!file.is_open()) return;

    // Limpa cadeia atual (se existir método)
    // Se não existir, apenas ignore
    std::string line;

    while (std::getline(file, line)) {
        if (line == "---") continue;

        // Aqui você pode reconstruir blocos conforme seu modelo atual
        // (simplificado por enquanto)

        // Exemplo básico:
        Block b;
        // preencher campos manualmente conforme seu Block atual
        bc.addBlock(b);
    }

    file.close();
}
