#include "../include/storage.h"
#include "../include/blockchain.h"
#include <fstream>
#include <iostream>

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return;

    std::vector<Block> chain = bc.getChain();
    int chainSize = chain.size();
    file.write((char*)&chainSize, sizeof(int));

    for (const auto& block : chain) {
        file.write((char*)&block.index, sizeof(int));
        file.write((char*)&block.timestamp, sizeof(long));
        
        int hashSize = block.hash.size();
        file.write((char*)&hashSize, sizeof(int));
        file.write(block.hash.c_str(), hashSize);
        
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

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return;

    int chainSize = 0;
    if (!file.read((char*)&chainSize, sizeof(int)) || chainSize <= 0 || chainSize > 1000000) {
        file.close();
        return; 
    }

    bc.clearChain();
    for (int i = 0; i < chainSize; i++) {
        int index; long timestamp; long nonce;
        if(!file.read((char*)&index, sizeof(int))) break;
        file.read((char*)&timestamp, sizeof(long));
        
        int hashSize; file.read((char*)&hashSize, sizeof(int));
        if (hashSize < 0 || hashSize > 1024) break; // Trava de segurança
        std::string hashStr(hashSize, ' ');
        file.read(&hashStr[0], hashSize);
        
        int prevHashSize; file.read((char*)&prevHashSize, sizeof(int));
        if (prevHashSize < 0 || prevHashSize > 1024) break; // Trava de segurança
        std::string prevHashStr(prevHashSize, ' ');
        file.read(&prevHashStr[0], prevHashSize);
        
        file.read((char*)&nonce, sizeof(long));

        std::vector<Transaction> txs;
        int txCount; file.read((char*)&txCount, sizeof(int));
        if (txCount < 0 || txCount > 10000) break;

        for (int j = 0; j < txCount; j++) {
            int voutCount; file.read((char*)&voutCount, sizeof(int));
            std::vector<VOut> vouts;
            for (int k = 0; k < voutCount; k++) {
                int addrSize; file.read((char*)&addrSize, sizeof(int));
                if (addrSize < 0 || addrSize > 512) break;
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

void Storage::saveMempool(const Transaction& tx, const std::string& filename) {
    std::ofstream file(filename, std::ios::app | std::ios::binary);
    if (!file.is_open()) return;
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

std::vector<Transaction> Storage::loadMempool(const std::string& filename) {
    std::vector<Transaction> txs;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return txs;

    while (file.peek() != EOF) {
        int voutCount;
        if(!file.read((char*)&voutCount, sizeof(int))) break;
        if (voutCount < 0 || voutCount > 1000) break;

        std::vector<VOut> vouts;
        for (int k = 0; k < voutCount; k++) {
            int addrSize; 
            if(!file.read((char*)&addrSize, sizeof(int))) break;
            if (addrSize < 0 || addrSize > 512) break;
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

void Storage::clearMempool(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    file.close();
}
