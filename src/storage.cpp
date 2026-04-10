#include "../include/storage.h"
#include <fstream>
#include <iostream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (file.is_open()) {
        int chainSize = bc.getChain().size();
        file.write((char*)&chainSize, sizeof(int));
        for (const auto& block : bc.getChain()) {
            file.write((char*)&block.index, sizeof(int));
            file.write((char*)&block.timestamp, sizeof(long));
            
            int hashSize = block.hash.size();
            file.write((char*)&hashSize, sizeof(int));
            file.write(block.hash.c_str(), hashSize);
            
            // CORREÇÃO: Usando prevHash em vez de previousHash
            int prevHashSize = block.prevHash.size();
            file.write((char*)&prevHashSize, sizeof(int));
            file.write(block.prevHash.c_str(), prevHashSize);
            
            file.write((char*)&block.nonce, sizeof(long));

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

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return;

    bc.clearChain();
    int chainSize;
    file.read((char*)&chainSize, sizeof(int));

    for (int i = 0; i < chainSize; i++) {
        int index; long timestamp; long nonce;
        file.read((char*)&index, sizeof(int));
        file.read((char*)&timestamp, sizeof(long));
        
        int hashSize; file.read((char*)&hashSize, sizeof(int));
        char* hashBuf = new char[hashSize + 1];
        file.read(hashBuf, hashSize); hashBuf[hashSize] = '\0';
        
        int prevHashSize; file.read((char*)&prevHashSize, sizeof(int));
        char* prevHashBuf = new char[prevHashSize + 1];
        file.read(prevHashBuf, prevHashSize); prevHashBuf[prevHashSize] = '\0';
        
        file.read((char*)&nonce, sizeof(long));

        std::vector<Transaction> txs;
        int txCount; file.read((char*)&txCount, sizeof(int));
        for (int j = 0; j < txCount; j++) {
            int voutCount; file.read((char*)&voutCount, sizeof(int));
            std::vector<VOut> vouts; // Agora o compilador reconhecerá VOut
            for (int k = 0; k < voutCount; k++) {
                int addrSize; file.read((char*)&addrSize, sizeof(int));
                char* addrBuf = new char[addrSize + 1];
                file.read(addrBuf, addrSize); addrBuf[addrSize] = '\0';
                double amount; file.read((char*)&amount, sizeof(double));
                
                vouts.push_back({std::string(addrBuf), amount});
                delete[] addrBuf;
            }
            txs.push_back({{}, vouts});
        }
        
        // CORREÇÃO: Passando os parâmetros para o construtor do Block
        Block b(index, std::string(prevHashBuf), txs);
        b.hash = std::string(hashBuf); 
        b.timestamp = timestamp; 
        b.nonce = nonce;
        
        bc.addBlock(b);
        delete[] hashBuf; 
        delete[] prevHashBuf;
    }
    file.close();
}

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

std::vector<Transaction> Storage::loadMempool(const std::string& filename) {
    std::vector<Transaction> txs;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return txs;

    while (file.peek() != EOF) {
        int voutCount;
        if(!file.read((char*)&voutCount, sizeof(int))) break;
        
        std::vector<VOut> vouts;
        for (int k = 0; k < voutCount; k++) {
            int addrSize; file.read((char*)&addrSize, sizeof(int));
            char* addrBuf = new char[addrSize + 1];
            file.read(addrBuf, addrSize); addrBuf[addrSize] = '\0';
            double amount; file.read((char*)&amount, sizeof(double));
            
            vouts.push_back({std::string(addrBuf), amount});
            delete[] addrBuf;
        }
        txs.push_back({{}, vouts});
    }
    file.close();
    return txs;
}

void Storage::clearMempool(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    file.close();
}
