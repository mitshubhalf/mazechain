#include "../include/storage.h"
#include "../include/blockchain.h"
#include <fstream>
#include <iostream>

// Salva a Blockchain inteira em formato binário
void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        std::vector<Block> chain = bc.getChain();
        int chainSize = chain.size();
        file.write((char*)&chainSize, sizeof(int));

        for (const auto& block : chain) {
            file.write((char*)&block.index, sizeof(int));
            file.write((char*)&block.timestamp, sizeof(long));
            
            // Grava o Hash do bloco
            int hashSize = block.hash.size();
            file.write((char*)&hashSize, sizeof(int));
            file.write(block.hash.c_str(), hashSize);
            
            // Grava o Hash anterior (prevHash)
            int prevHashSize = block.prevHash.size();
            file.write((char*)&prevHashSize, sizeof(int));
            file.write(block.prevHash.c_str(), prevHashSize);
            
            file.write((char*)&block.nonce, sizeof(long));

            // Grava as transações do bloco
            int txCount = block.transactions.size();
            file.write((char*)&txCount, sizeof(int));
            for (const auto& tx : block.transactions) {
                int voutCount = tx.vout.size();
                file.write((char*)&voutCount, sizeof(int));
                for (const auto& out : tx.vout) {
                    int addrSize = out.address.size();
                    file.write((char*)&addrSize, sizeof(int));
                    file.write(out.address.c_str(), addrSize);
                    file.write((char*)&out.amount, sizeof(double));
                }
            }
        }
        file.close();
    }
}

// Carrega a Blockchain do arquivo para a memória
void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return;

    bc.clearChain();
    int chainSize;
    if (!file.read((char*)&chainSize, sizeof(int))) return;

    for (int i = 0; i < chainSize; i++) {
        int index; long timestamp; long nonce;
        file.read((char*)&index, sizeof(int));
        file.read((char*)&timestamp, sizeof(long));
        
        int hashSize; file.read((char*)&hashSize, sizeof(int));
        std::string hashStr(hashSize, ' ');
        file.read(&hashStr[0], hashSize);
        
        int prevHashSize; file.read((char*)&prevHashSize, sizeof(int));
        std::string prevHashStr(prevHashSize, ' ');
        file.read(&prevHashStr[0], prevHashSize);
        
        file.read((char*)&nonce, sizeof(long));

        std::vector<Transaction> txs;
        int txCount; file.read((char*)&txCount, sizeof(int));
        for (int j = 0; j < txCount; j++) {
            int voutCount; file.read((char*)&voutCount, sizeof(int));
            std::vector<VOut> vouts;
            for (int k = 0; k < voutCount; k++) {
                int addrSize; file.read((char*)&addrSize, sizeof(int));
                std::string addrStr(addrSize, ' ');
                file.read(&addrStr[0], addrSize);
                double amount; file.read((char*)&amount, sizeof(double));
                vouts.push_back({addrStr, amount});
            }
            txs.push_back(Transaction({}, vouts));
        }
        
        Block b(index, prevHashStr, txs);
        b.hash = hashStr; 
        b.timestamp = timestamp; 
        b.nonce = nonce;
        bc.addBlock(b);
    }
    file.close();
}

// Salva uma transação individual na Mempool (fila de espera)
void Storage::saveMempool(const Transaction& tx, const std::string& filename) {
    std::ofstream file(filename, std::ios::app | std::ios::binary);
    if (file.is_open()) {
        int voutCount = tx.vout.size();
        file.write((char*)&voutCount, sizeof(int));
        for (const auto& out : tx.vout) {
            int addrSize = out.address.size();
            file.write((char*)&addrSize, sizeof(int));
            file.write(out.address.c_str(), addrSize);
            file.write((char*)&out.amount, sizeof(double));
        }
        file.close();
    }
}

// Carrega todas as transações pendentes da Mempool
std::vector<Transaction> Storage::loadMempool(const std::string& filename) {
    std::vector<Transaction> txs;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return txs;

    while (file.peek() != EOF) {
        int voutCount;
        if(!file.read((char*)&voutCount, sizeof(int))) break;
        
        std::vector<VOut> vouts;
        for (int k = 0; k < voutCount; k++) {
            int addrSize; 
            if(!file.read((char*)&addrSize, sizeof(int))) break;
            std::string addrStr(addrSize, ' ');
            file.read(&addrStr[0], addrSize);
            double amount; 
            file.read((char*)&amount, sizeof(double));
            vouts.push_back({addrStr, amount});
        }
        txs.push_back(Transaction({}, vouts));
    }
    file.close();
    return txs;
}

// Limpa o arquivo da Mempool após a mineração
void Storage::clearMempool(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    file.close();
}
