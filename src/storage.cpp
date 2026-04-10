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
        int hSize = block.hash.size();
        file.write((char*)&hSize, sizeof(int));
        file.write(block.hash.c_str(), hSize);
        int phSize = block.prevHash.size();
        file.write((char*)&phSize, sizeof(int));
        file.write(block.prevHash.c_str(), phSize);
        file.write((char*)&block.nonce, sizeof(long));
        int txCount = block.transactions.size();
        file.write((char*)&txCount, sizeof(int));
        for (const auto& tx : block.transactions) {
            int vCount = tx.vout.size();
            file.write((char*)&vCount, sizeof(int));
            for (const auto& out : tx.vout) {
                int aSize = out.address.size();
                file.write((char*)&aSize, sizeof(int));
                file.write(out.address.c_str(), aSize);
                file.write((char*)&out.amount, sizeof(double));
            }
        }
    }
    file.close();
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return;
    int chainSize;
    if (!file.read((char*)&chainSize, sizeof(int)) || chainSize < 0 || chainSize > 1000000) return;
    bc.clearChain();
    for (int i = 0; i < chainSize; i++) {
        int index; long timestamp; long nonce;
        file.read((char*)&index, sizeof(int));
        file.read((char*)&timestamp, sizeof(long));
        
        int hSize; file.read((char*)&hSize, sizeof(int));
        if(hSize < 0 || hSize > 1024) break;
        std::string h(hSize, ' '); file.read(&h[0], hSize);
        
        int phSize; file.read((char*)&phSize, sizeof(int));
        if(phSize < 0 || phSize > 1024) break;
        std::string ph(phSize, ' '); file.read(&ph[0], phSize);
        
        file.read((char*)&nonce, sizeof(long));
        std::vector<Transaction> txs;
        int txCount; file.read((char*)&txCount, sizeof(int));
        if(txCount < 0 || txCount > 1000) break;

        for (int j = 0; j < txCount; j++) {
            int vCount; file.read((char*)&vCount, sizeof(int));
            std::vector<TxOut> vouts; // CORRIGIDO: de VOut para TxOut
            for (int k = 0; k < vCount; k++) {
                int aSize; file.read((char*)&aSize, sizeof(int));
                if(aSize < 0 || aSize > 512) break;
                std::string a(aSize, ' '); file.read(&a[0], aSize);
                double amt; file.read((char*)&amt, sizeof(double));
                vouts.push_back({a, amt});
            }
            txs.push_back(Transaction({}, vouts));
        }
        Block b(index, ph, txs); b.hash = h; b.timestamp = timestamp; b.nonce = nonce;
        bc.addBlock(b);
    }
    file.close();
}

void Storage::saveMempool(const Transaction& tx, const std::string& filename) {
    std::ofstream file(filename, std::ios::app | std::ios::binary);
    if (file.is_open()) {
        int vCount = tx.vout.size();
        file.write((char*)&vCount, sizeof(int));
        for (const auto& out : tx.vout) {
            int aSize = out.address.size();
            file.write((char*)&aSize, sizeof(int));
            file.write(out.address.c_str(), aSize);
            file.write((char*)&out.amount, sizeof(double));
        }
    }
}

std::vector<Transaction> Storage::loadMempool(const std::string& filename) {
    std::vector<Transaction> txs;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return txs;
    while (file.peek() != EOF) {
        int vCount; if(!file.read((char*)&vCount, sizeof(int))) break;
        if(vCount < 0 || vCount > 1000) break;
        std::vector<TxOut> vouts; // CORRIGIDO: de VOut para TxOut
        for (int k = 0; k < vCount; k++) {
            int aSize; file.read((char*)&aSize, sizeof(int));
            if(aSize < 0 || aSize > 512) break;
            std::string a(aSize, ' '); file.read(&a[0], aSize);
            double amt; file.read((char*)&amt, sizeof(double));
            vouts.push_back({a, amt});
        }
        txs.push_back(Transaction({}, vouts));
    }
    return txs;
}

void Storage::clearMempool(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
}
