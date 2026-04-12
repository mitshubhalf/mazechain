#include "../include/storage.h"
#include "../include/blockchain.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>

// =================================================================
// SALVAMENTO DA MEMPOOL
// =================================================================
void Storage::saveMempool(const Transaction& tx, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        std::cerr << "[STORAGE ERROR] Nao foi possivel abrir mempool para escrita." << std::endl;
        return;
    }

    // Grava ID da Transação
    int idSize = static_cast<int>(tx.id.size());
    file.write(reinterpret_cast<const char*>(&idSize), sizeof(int));
    file.write(tx.id.c_str(), idSize);

    // Grava Assinatura
    int sigSize = static_cast<int>(tx.signature.size());
    file.write(reinterpret_cast<const char*>(&sigSize), sizeof(int));
    file.write(tx.signature.c_str(), sigSize);

    // Grava Chave Publica / Seed
    int pubSize = static_cast<int>(tx.publicKey.size());
    file.write(reinterpret_cast<const char*>(&pubSize), sizeof(int));
    file.write(tx.publicKey.c_str(), pubSize);

    // Grava Contagem de Saidas (VOUT)
    int vCount = static_cast<int>(tx.vout.size());
    file.write(reinterpret_cast<const char*>(&vCount), sizeof(int));

    // Grava cada Saida individualmente
    for (const auto& out : tx.vout) {
        int aSize = static_cast<int>(out.address.size());
        file.write(reinterpret_cast<const char*>(&aSize), sizeof(int));
        file.write(out.address.c_str(), aSize);
        file.write(reinterpret_cast<const char*>(&out.amount), sizeof(double));
    }

    file.flush(); // Garante que os dados sairam do cache para o disco
    file.close();
}

// =================================================================
// CARREGAMENTO DA MEMPOOL
// =================================================================
std::vector<Transaction> Storage::loadMempool(const std::string& filename) {
    std::vector<Transaction> txs;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return txs;

    while (file.peek() != EOF) {
        int idSize;
        if (!file.read(reinterpret_cast<char*>(&idSize), sizeof(int))) break;
        std::string id(idSize, ' ');
        file.read(&id[0], idSize);

        int sigSize; 
        if (!file.read(reinterpret_cast<char*>(&sigSize), sizeof(int))) break;
        std::string sig(sigSize, ' '); 
        file.read(&sig[0], sigSize);

        int pubSize; 
        if (!file.read(reinterpret_cast<char*>(&pubSize), sizeof(int))) break;
        std::string pub(pubSize, ' '); 
        file.read(&pub[0], pubSize);

        int vCount;
        if (!file.read(reinterpret_cast<char*>(&vCount), sizeof(int))) break;
        
        std::vector<TxOut> vouts;
        for (int k = 0; k < vCount; k++) {
            int aSize;
            file.read(reinterpret_cast<char*>(&aSize), sizeof(int));
            std::string a(aSize, ' ');
            file.read(&a[0], aSize);
            double amt;
            file.read(reinterpret_cast<char*>(&amt), sizeof(double));
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

// =================================================================
// SALVAMENTO DA BLOCKCHAIN (ESTRUTURA COMPLETA)
// =================================================================
void Storage::saveChain(const Blockchain& bc, const std::string& filename) {
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[STORAGE ERROR] Falha ao salvar Blockchain." << std::endl;
        return;
    }

    std::vector<Block> chain = bc.getChain();
    int chainSize = static_cast<int>(chain.size());
    file.write(reinterpret_cast<const char*>(&chainSize), sizeof(int));

    for (const auto& block : chain) {
        // Dados Basicos do Bloco
        file.write(reinterpret_cast<const char*>(&block.index), sizeof(int));
        long long ts = block.timestamp;
        file.write(reinterpret_cast<const char*>(&ts), sizeof(long long));
        
        // Hash do Bloco
        int hSize = static_cast<int>(block.hash.size());
        file.write(reinterpret_cast<const char*>(&hSize), sizeof(int));
        file.write(block.hash.c_str(), hSize);
        
        // Hash Anterior
        int phSize = static_cast<int>(block.prevHash.size());
        file.write(reinterpret_cast<const char*>(&phSize), sizeof(int));
        file.write(block.prevHash.c_str(), phSize);
        
        // Nonce de Mineracao
        file.write(reinterpret_cast<const char*>(&block.nonce), sizeof(int));
        
        // Transacoes dentro do Bloco
        int txCount = static_cast<int>(block.transactions.size());
        file.write(reinterpret_cast<const char*>(&txCount), sizeof(int));

        for (const auto& tx : block.transactions) {
            int idS = static_cast<int>(tx.id.size());
            file.write(reinterpret_cast<const char*>(&idS), sizeof(int));
            file.write(tx.id.c_str(), idS);

            int sS = static_cast<int>(tx.signature.size());
            file.write(reinterpret_cast<const char*>(&sS), sizeof(int));
            file.write(tx.signature.c_str(), sS);

            int pS = static_cast<int>(tx.publicKey.size());
            file.write(reinterpret_cast<const char*>(&pS), sizeof(int));
            file.write(tx.publicKey.c_str(), pS);

            int outCount = static_cast<int>(tx.vout.size());
            file.write(reinterpret_cast<const char*>(&outCount), sizeof(int));
            for (const auto& out : tx.vout) {
                int aSize = static_cast<int>(out.address.size());
                file.write(reinterpret_cast<const char*>(&aSize), sizeof(int));
                file.write(out.address.c_str(), aSize);
                file.write(reinterpret_cast<const char*>(&out.amount), sizeof(double));
            }
        }
    }
    file.flush();
    file.close();
}

// =================================================================
// CARREGAMENTO DA BLOCKCHAIN (ESTRUTURA COMPLETA)
// =================================================================
void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "[INFO] Nenhuma blockchain encontrada em disco. Criando nova." << std::endl;
        return;
    }

    int chainSize;
    if (!file.read(reinterpret_cast<char*>(&chainSize), sizeof(int))) return;

    bc.clearChain();
    for (int i = 0; i < chainSize; i++) {
        int index; 
        long long timestamp; 
        int nonce;

        file.read(reinterpret_cast<char*>(&index), sizeof(int));
        file.read(reinterpret_cast<char*>(&timestamp), sizeof(long long));
        
        int hSize; 
        file.read(reinterpret_cast<char*>(&hSize), sizeof(int));
        std::string h(hSize, ' '); 
        file.read(&h[0], hSize);
        
        // CORREÇÃO DA LINHA 168 (Onde ocorria o erro de parênteses no log)
        int phSize; 
        file.read(reinterpret_cast<char*>(&phSize), sizeof(int));
        std::string ph(phSize, ' '); 
        file.read(&ph[0], phSize);
        
        file.read(reinterpret_cast<char*>(&nonce), sizeof(int));
        
        int txCount; 
        file.read(reinterpret_cast<char*>(&txCount), sizeof(int));
        
        std::vector<Transaction> txs;
        for (int j = 0; j < txCount; j++) {
            int idS; 
            file.read(reinterpret_cast<char*>(&idS), sizeof(int));
            std::string id(idS, ' '); 
            file.read(&id[0], idS);

            int sS; 
            file.read(reinterpret_cast<char*>(&sS), sizeof(int));
            std::string sig(sS, ' '); 
            file.read(&sig[0], sS);

            int pS; 
            file.read(reinterpret_cast<char*>(&pS), sizeof(int));
            std::string pub(pS, ' '); 
            file.read(&pub[0], pS);

            int outCount; 
            file.read(reinterpret_cast<char*>(&outCount), sizeof(int));
            
            std::vector<TxOut> vouts;
            for (int k = 0; k < outCount; k++) {
                int aSize; 
                file.read(reinterpret_cast<char*>(&aSize), sizeof(int));
                std::string a(aSize, ' '); 
                file.read(&a[0], aSize);
                
                double amt; 
                file.read(reinterpret_cast<char*>(&amt), sizeof(double));
                vouts.push_back({a, amt});
            }
            Transaction t; 
            t.id = id; 
            t.signature = sig; 
            t.publicKey = pub; 
            t.vout = vouts;
            txs.push_back(t);
        }

        Block b(index, ph, txs);
        b.hash = h; 
        b.timestamp = timestamp; 
        b.nonce = nonce;
        bc.addBlock(b);
    }
    file.close();
}
