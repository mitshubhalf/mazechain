#include "../include/storage.h"
#include "../include/blockchain.h"
#include <fstream>
#include <iostream>
#include <sys/stat.h>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(dir) _mkdir(dir)
#else
    #include <sys/types.h>
    #include <sys/stat.h>
    #define MKDIR(dir) mkdir(dir, 0777)
#endif

// Garante que a pasta de dados exista
void ensure_directory() { 
    struct stat info;
    if (stat("data", &info) != 0) {
        MKDIR("data");
    }
}

void Storage::saveMempool(const Transaction& tx, const std::string& filename) {
    ensure_directory();
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (!file.is_open()) return;

    int idSize = tx.id.size();
    file.write((char*)&idSize, sizeof(int));
    file.write(tx.id.c_str(), idSize);

    int sigSize = tx.signature.size();
    file.write((char*)&sigSize, sizeof(int));
    file.write(tx.signature.c_str(), sigSize);

    int pubSize = tx.publicKey.size();
    file.write((char*)&pubSize, sizeof(int));
    file.write(tx.publicKey.c_str(), pubSize);

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
        if(!file.read((char*)&idSize, sizeof(int))) break;
        std::string id(idSize, ' '); file.read(&id[0], idSize);
        
        int sigSize; file.read((char*)&sigSize, sizeof(int));
        std::string sig(sigSize, ' '); file.read(&sig[0], sigSize);
        
        int pubSize; file.read((char*)&pubSize, sizeof(int));
        std::string pub(pubSize, ' '); 
        if(pubSize > 0) file.read(&pub[0], pubSize); // Proteção contra pub vazia
        
        int vCount; file.read((char*)&vCount, sizeof(int));
        std::vector<TxOut> vouts;
        for (int k = 0; k < vCount; k++) {
            int aSize; file.read((char*)&aSize, sizeof(int));
            std::string a(aSize, ' '); file.read(&a[0], aSize);
            double amt; file.read((char*)&amt, sizeof(double));
            vouts.push_back({a, amt});
        }
        Transaction t; t.id = id; t.signature = sig; t.publicKey = pub; t.vout = vouts;
        txs.push_back(t);
    }
    file.close();
    return txs;
}

void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    ensure_directory();
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "❌ Erro ao abrir arquivo para salvar blockchain!" << std::endl;
        return;
    }

    auto chain = bc.getChain();
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
            int idS = tx.id.size(); file.write((char*)&idS, sizeof(int));
            file.write(tx.id.c_str(), idS);
            
            int sS = tx.signature.size(); file.write((char*)&sS, sizeof(int));
            file.write(tx.signature.c_str(), sS);
            
            int pS = tx.publicKey.size(); file.write((char*)&pS, sizeof(int));
            file.write(tx.publicKey.c_str(), pS);
            
            int outCount = tx.vout.size(); file.write((char*)&outCount, sizeof(int));
            for (const auto& out : tx.vout) {
                int aSize = out.address.size(); file.write((char*)&aSize, sizeof(int));
                file.write(out.address.c_str(), aSize);
                file.write((char*)&out.amount, sizeof(double));
            }
        }
    }
    file.close();
}

bool Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    int chainSize; 
    if(!file.read((char*)&chainSize, sizeof(int))) return false;

    bc.clearChain();

    for (int i = 0; i < chainSize; i++) {
        int idx; long long ts; int n;
        if(!file.read((char*)&idx, sizeof(int))) break;
        file.read((char*)&ts, sizeof(long long));
        
        int hS; file.read((char*)&hS, sizeof(int));
        std::string h(hS, ' '); file.read(&h[0], hS);
        
        int phS; file.read((char*)&phS, sizeof(int));
        std::string ph(phS, ' '); file.read(&ph[0], phS);
        
        file.read((char*)&n, sizeof(int));
        
        int txC; file.read((char*)&txC, sizeof(int));
        std::vector<Transaction> txs;
        for (int j = 0; j < txC; j++) {
            int idS; file.read((char*)&idS, sizeof(int));
            std::string id(idS, ' '); file.read(&id[0], idS);
            
            int sS; file.read((char*)&sS, sizeof(int));
            std::string sig(sS, ' '); file.read(&sig[0], sS);
            
            int pS; file.read((char*)&pS, sizeof(int));
            std::string pub(pS, ' '); 
            if(pS > 0) file.read(&pub[0], pS); // Melhorado aqui
            
            int outC; file.read((char*)&outC, sizeof(int));
            std::vector<TxOut> vouts;
            for (int k = 0; k < outC; k++) {
                int aS; file.read((char*)&aS, sizeof(int));
                std::string a(aS, ' '); file.read(&a[0], aS);
                double amt; file.read((char*)&amt, sizeof(double));
                vouts.push_back({a, amt});
            }
            Transaction t; t.id = id; t.signature = sig; t.publicKey = pub; t.vout = vouts;
            txs.push_back(t);
        }
        
        Block b(idx, ph, txs); 
        b.hash = h; 
        b.timestamp = ts; 
        b.nonce = n;
        
        bc.addBlock(b);
    }
    file.close();
    std::cout << "✅ Blockchain carregada com sucesso. Altura: " << bc.getChain().size() << std::endl;
    return true;
}

void Storage::clearMempool(const std::string& filename) { 
    std::ofstream file(filename, std::ios::trunc); 
    file.close();
}
