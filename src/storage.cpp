#include "../include/storage.h"
#include "../include/blockchain.h"
#include <fstream>
#include <iostream>

// Salva a transação na Mempool (Agora salva assinaturas)
void Storage::saveMempool(const Transaction& tx, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (!file.is_open()) return;

    // ID
    int idSize = tx.id.size();
    file.write((char*)&idSize, sizeof(int));
    file.write(tx.id.c_str(), idSize);

    // Signature
    int sigSize = tx.signature.size();
    file.write((char*)&sigSize, sizeof(int));
    file.write(tx.signature.c_str(), sigSize);

    // Public Key (Seed)
    int pubSize = tx.publicKey.size();
    file.write((char*)&pubSize, sizeof(int));
    file.write(tx.publicKey.c_str(), pubSize);

    // Saídas
    int vCount = tx.vout.size();
    file.write((char*)&vCount, sizeof(int));
    for (const auto& out : tx.vout) {
        int aSize = out.address.size();
        file.write((char*)&aSize, sizeof(int));
        file.write(out.address.c_str(), aSize);
        file.write((char*)&out.amount, sizeof(double));
    }
    file.close();
}

std::vector<Transaction> Storage::loadMempool(const std::string& filename) {
    std::vector<Transaction> txs;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return txs;

    while (file.peek() != EOF) {
        int idSize;
        if (!file.read((char*)&idSize, sizeof(int))) break;
        std::string id(idSize, ' ');
        file.read(&id[0], idSize);

        int sigSize; file.read((char*)&sigSize, sizeof(int));
        std::string sig(sigSize, ' '); file.read(&sig[0], sigSize);

        int pubSize; file.read((char*)&pubSize, sizeof(int));
        std::string pub(pubSize, ' '); file.read(&pub[0], pubSize);

        int vCount;
        file.read((char*)&vCount, sizeof(int));
        std::vector<TxOut> vouts;
        for (int k = 0; k < vCount; k++) {
            int aSize;
            file.read((char*)&aSize, sizeof(int));
            std::string a(aSize, ' ');
            file.read(&a[0], aSize);
            double amt;
            file.read((char*)&amt, sizeof(double));
            vouts.push_back({a, amt});
        }
        Transaction t;
        t.id = id;
        t.signature = sig;
        t.publicKey = pub;
        t.vout = vouts;
        txs.push_back(t);
    }
    file.close();
    return txs;
}

void Storage::clearMempool(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    file.close();
}

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) return;

    std::vector<Block> chain = bc.getChain();
    int chainSize = chain.size();
    file.write((char*)&chainSize, sizeof(int));

    for (const auto& block : chain) {
        file.write((char*)&block.index, sizeof(int));
        long long ts = block.timestamp;
        file.write((char*)&ts, sizeof(long long));
        
        int hSize = block.hash.size();
        file.write((char*)&hSize, sizeof(int));
        file.write(block.hash.c_str(), hSize);
        
        int phSize = block.prevHash.size();
        file.write((char*)&phSize, sizeof(int));
        file.write(block.prevHash.c_str(), phSize);
        
        file.write((char*)&block.nonce, sizeof(int));
        
        int txCount = block.transactions.size();
        file.write((char*)&txCount, sizeof(int));
        for (const auto& tx : block.transactions) {
            int idS = tx.id.size();
            file.write((char*)&idS, sizeof(int));
            file.write(tx.id.c_str(), idS);

            int sS = tx.signature.size();
            file.write((char*)&sS, sizeof(int));
            file.write(tx.signature.c_str(), sS);

            int pS = tx.publicKey.size();
            file.write((char*)&pS, sizeof(int));
            file.write(tx.publicKey.c_str(), pS);

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
    if (!file.read((char*)&chainSize, sizeof(int))) return;

    bc.clearChain();
    for (int i = 0; i < chainSize; i++) {
        int index; long long timestamp; int nonce;
        file.read((char*)&index, sizeof(int));
        file.read((char*)&timestamp, sizeof(long long));
        
        int hSize; file.read((char*)&hSize, sizeof(int));
        std::string h(hSize, ' '); file.read(&h[0], hSize);
        
        int phSize; file.read((char*)&phSize, sizeof(int));
        std::string ph(phSize, ' '); file.read(&ph[0], phSize);
        
        file.read((char*)&nonce, sizeof(int));
        
        int txCount; file.read((char*)&txCount, sizeof(int));
        std::vector<Transaction> txs;
        for (int j = 0; j < txCount; j++) {
            int idS; file.read((char*)&idS, sizeof(int));
            std::string id(idS, ' '); file.read(&id[0], idS);

            int sS; file.read((char*)&sS, sizeof(int));
            std::string sig(sS, ' '); file.read(&sig[0], sS);

            int pS; file.read((char*)&pS, sizeof(int));
            std::string pub(pS, ' '); file.read(&pub[0], pS);

            int vCount; file.read((char*)&vCount, sizeof(int));
            std::vector<TxOut> vouts;
            for (int k = 0; k < vCount; k++) {
                int aSize; file.read((char*)&aSize, sizeof(int));
                std::string a(aSize, ' '); file.read(&a[0], aSize);
                double amt; file.read((char*)&amt, sizeof(double));
                vouts.push_back({a, amt});
            }
            Transaction t; 
            t.id = id; t.signature = sig; t.publicKey = pub; t.vout = vouts;
            txs.push_back(t);
        }
        Block b(index, ph, txs);
        b.hash = h; b.timestamp = timestamp; b.nonce = nonce;
        bc.addBlock(b);
    }
    file.close();
}
