#include "../include/storage.h"
#include <fstream>
#include <iostream>
#include <vector>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc); // Sobrescreve para evitar lixo

    if (!file.is_open()) {
        std::cerr << "❌ Erro ao abrir arquivo para salvar!\n";
        return;
    }

    for (const auto& block : bc.getChain()) {
        file << "BLOCK\n" << block.index << "\n" << block.prevHash << "\n" 
             << block.hash << "\n" << block.nonce << "\n";

        for (const auto& tx : block.transactions) {
            file << "TX\n" << tx.vin.size() << "\n";
            for (const auto& in : tx.vin) {
                file << in.txid << "\n" << in.index << "\n";
            }
            file << tx.vout.size() << "\n";
            for (const auto& out : tx.vout) {
                file << out.address << "\n" << out.amount << "\n";
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

    // Usaremos um bloco temporário para carregar os dados com segurança
    int tempIdx = 0, tempNonce = 0;
    std::string tempPrev, tempHash;
    std::vector<Transaction> tempTxs;

    while (std::getline(file, line)) {
        if (line == "BLOCK") {
            tempTxs.clear();
            if (!(file >> tempIdx)) break;
            file.ignore();
            std::getline(file, tempPrev);
            std::getline(file, tempHash);
            if (!(file >> tempNonce)) break;
            file.ignore();
        } 
        else if (line == "TX") {
            int vinSize, voutSize;
            if (!(file >> vinSize)) break;
            file.ignore();
            
            // Proteção contra bad_alloc: limita transações malucas
            if (vinSize < 0 || vinSize > 1000) continue; 

            std::vector<TxIn> vins;
            for (int i = 0; i < vinSize; i++) {
                TxIn in;
                std::getline(file, in.txid);
                file >> in.index; file.ignore();
                vins.push_back(in);
            }

            file >> voutSize; file.ignore();
            if (voutSize < 0 || voutSize > 1000) continue;

            std::vector<TxOut> vouts;
            for (int i = 0; i < voutSize; i++) {
                TxOut out;
                std::getline(file, out.address);
                file >> out.amount; file.ignore();
                vouts.push_back(out);
            }
            tempTxs.push_back(Transaction(vins, vouts));
        } 
        else if (line == "END_BLOCK") {
            Block newBlock(tempIdx, tempPrev, tempTxs);
            newBlock.hash = tempHash;
            newBlock.nonce = tempNonce;

            // Validação de integridade antes de aceitar o bloco carregado
            if (newBlock.calculateHash() != newBlock.hash) {
                std::cerr << "⚠️ Bloco " << tempIdx << " corrompido no arquivo. Parando carga.\n";
                break; 
            }
            bc.addBlock(newBlock);
        }
    }
    file.close();
}
