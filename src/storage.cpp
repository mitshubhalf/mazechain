#include "../include/storage.h"
#include "../include/blockchain.h"
#include "../include/block.h"

#include <fstream>
#include <sstream>
#include <iostream>

namespace Storage {

// 💾 SAVE
void saveChain(const Blockchain &bc, const std::string& filename) {

    std::ofstream file(filename, std::ios::out | std::ios::trunc);

    if (!file.is_open()) {
        std::cout << "Erro ao salvar\n";
        return;
    }

    const std::vector<Block>& chain = bc.getChain();

    for (const Block& block : chain) {

        file << block.index << "|"
             << block.timestamp << "|"
             << block.hash << "|"
             << block.previousHash << "|"
             << block.nonce << "|";

        // 🔥 salvar transactions
        for (size_t i = 0; i < block.transactions.size(); i++) {
            const auto& tx = block.transactions[i];

            file << tx.from << "," << tx.to << "," << tx.amount;

            if (i != block.transactions.size() - 1)
                file << ";";
        }

        file << "\n";
    }

    file.flush();   // 🔥 garante escrita completa
    file.close();
}


// 📂 LOAD
void loadChain(Blockchain &bc, const std::string& filename) {

    std::ifstream file(filename);

    if (!file.is_open()) {
        std::cout << "Nenhum arquivo encontrado, iniciando nova chain\n";
        return;
    }

    bc.clearChain();

    std::string line;
    int loadedCount = 0;

    while (std::getline(file, line)) {

        if (line.empty()) continue;

        // 🔥 proteção contra linha corrompida
        if (line.find('|') == std::string::npos) {
            std::cout << "⚠️ Linha corrompida ignorada: " << line << "\n";
            continue;
        }

        std::stringstream ss(line);

        std::string indexStr, timestamp, hash, prevHash, nonceStr, txStr;

        std::getline(ss, indexStr, '|');
        std::getline(ss, timestamp, '|');
        std::getline(ss, hash, '|');
        std::getline(ss, prevHash, '|');
        std::getline(ss, nonceStr, '|');
        std::getline(ss, txStr);

        try {
            int index = std::stoi(indexStr);
            int nonce = std::stoi(nonceStr);

            std::vector<Transaction> txs;

            // 🔥 carregar transactions
            std::stringstream txStream(txStr);
            std::string txItem;

            while (std::getline(txStream, txItem, ';')) {

                if (txItem.empty()) continue;

                std::stringstream txSS(txItem);

                std::string from, to, amountStr;

                std::getline(txSS, from, ',');
                std::getline(txSS, to, ',');
                std::getline(txSS, amountStr);

                if (amountStr.empty()) continue;

                double amount = std::stod(amountStr);

                txs.push_back(Transaction(from, to, amount));
            }

            // 🔥 CRÍTICO: não recalcular hash
            Block b;
            b.index = index;
            b.transactions = txs;
            b.previousHash = prevHash;
            b.timestamp = timestamp;
            b.hash = hash;
            b.nonce = nonce;

            bc.addLoadedBlock(b);
            loadedCount++;

        } catch (...) {
            std::cout << "⚠️ Linha ignorada: " << line << "\n";
            continue;
        }
    }

    file.close();

    // 🔥 se não carregou nada → recria genesis
    if (loadedCount == 0) {
        std::cout << "Chain inválida, recriando genesis\n";
        bc.clearChain();
        bc.addLoadedBlock(Block(0, {}, "0"));
    }
}

} // namespace Storage
