#include "../include/storage.h"
#include <fstream>
#include <iostream>
#include <vector>

// Função auxiliar para garantir leitura segura de números
static bool safeReadInt(std::ifstream &file, int &out) {
    if (!(file >> out)) return false;
    file.ignore(10000, '\n'); // Limpa o buffer até a próxima linha
    return true;
}

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc); // Abre sobrescrevendo

    if (!file.is_open()) {
        std::cerr << "❌ Erro ao abrir arquivo para salvar blockchain\n";
        return;
    }

    std::vector<Block> chain = bc.getChain();

    for (const auto& block : chain) {
        file << "BLOCK\n";
        file << block.index << "\n";
        file << block.prevHash << "\n";
        file << block.hash << "\n";
        file << block.nonce << "\n";

        for (const auto& tx : block.transactions) {
            file << "TX\n";
            file << tx.vin.size() << "\n";
            for (const auto& in : tx.vin) {
                file << in.txid << "\n";
                file << in.index << "\n";
            }

            file << tx.vout.size() << "\n";
            for (const auto& out : tx.vout) {
                file << out.address << "\n";
                file << out.amount << "\n";
            }
        }
        file << "END_BLOCK\n";
    }
    file.close();
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) return;

    bc.clearChain();

    std::string line;
    // Variáveis temporárias para armazenar os dados do bloco atual
    int tIdx = 0, tNonce = 0;
    std::string tPrev, tHash;
    std::vector<Transaction> tTxs;

    while (std::getline(file, line)) {
        if (line == "BLOCK") {
            tTxs.clear(); // Limpa transações do bloco anterior
            if (!safeReadInt(file, tIdx)) break;
            std::getline(file, tPrev);
            std::getline(file, tHash);
            if (!safeReadInt(file, tNonce)) break;
        } 
        else if (line == "TX") {
            int vinSize, voutSize;
            if (!safeReadInt(file, vinSize)) break;
            
            // Segurança: se o arquivo estiver corrompido, evita alocar memória infinita
            if (vinSize < 0 || vinSize > 1000) continue;

            std::vector<TxIn> vin;
            for (int i = 0; i < vinSize; i++) {
                TxIn in;
                std::getline(file, in.txid);
                int idx;
                if (!safeReadInt(file, idx)) break;
                in.index = idx;
                vin.push_back(in);
            }

            if (!safeReadInt(file, voutSize)) break;
            if (voutSize < 0 || voutSize > 1000) continue;

            std::vector<TxOut> vout;
            for (int i = 0; i < voutSize; i++) {
                TxOut out;
                std::getline(file, out.address);
                if (!(file >> out.amount)) break;
                file.ignore(10000, '\n');
                vout.push_back(out);
            }

            tTxs.push_back(Transaction(vin, vout));
        } 
        else if (line == "END_BLOCK") {
            // Cria o bloco com os dados lidos
            Block loadedBlock(tIdx, tPrev, tTxs);
            loadedBlock.hash = tHash;
            loadedBlock.nonce = tNonce;

            // Validação de integridade antes de aceitar o bloco
            if (loadedBlock.hash == loadedBlock.calculateHash()) {
                bc.addBlock(loadedBlock);
            } else {
                std::cerr << "❌ Bloco " << tIdx << " corrompido no arquivo e ignorado.\n";
            }
        }
    }
    file.close();
}
