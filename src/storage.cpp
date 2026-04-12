#include "../include/storage.h"
#include "../include/blockchain.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <sys/stat.h>

// Função para garantir que a pasta de dados exista
void ensure_directory() {
#ifdef _WIN32
    _mkdir("data");
#else
    mkdir("data", 0777);
#endif
}

void Storage::saveMempool(const Transaction& tx, const std::string& filename) {
    ensure_directory();
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        std::cerr << "[ERRO] Falha ao abrir mempool: " << filename << std::endl;
        return;
    }

    int idSize = static_cast<int>(tx.id.size());
    file.write(reinterpret_cast<const char*>(&idSize), sizeof(int));
    file.write(tx.id.c_str(), idSize);

    int sigSize = static_cast<int>(tx.signature.size());
    file.write(reinterpret_cast<const char*>(&sigSize), sizeof(int));
    file.write(tx.signature.c_str(), sigSize);

    int pubSize = static_cast<int>(tx.publicKey.size());
    file.write(reinterpret_cast<const char*>(&pubSize), sizeof(int));
    file.write(tx.publicKey.c_str(), pubSize);

    int vCount = static_cast<int>(tx.vout.size());
    file.write(reinterpret_cast<const char*>(&vCount), sizeof(int));
    for (const auto& out : tx.vout) {
        int aSize = static_cast<int>(out.address.size());
        file.write(reinterpret_cast<const char*>(&aSize), sizeof(int));
        file.write(out.address.c_str(), aSize);
        file.write(reinterpret_cast<const char*>(&out.amount), sizeof(double));
    }
    file.flush();
    file.close();
}

std::vector<Transaction> Storage::loadMempool(const std::string& filename) {
    std::vector<Transaction> txs;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return txs;

    while (file.peek() != EOF) {
        int idSize;
        if (!file.read(reinterpret_cast<char*>(&idSize), sizeof(int))) break;
        std::string id(idSize, ' ');
        file.read(&id[0], idSize);

        int sigSize; file.read(reinterpret_cast<char*>(&sigSize), sizeof(int));
        std::string sig(sigSize, ' '); file.read(&sig[0], sigSize);

        int pubSize; file.read(reinterpret_cast<char*>(&pubSize), sizeof(int));
        std::string pub(pubSize, ' '); file.read(&pub[0], pubSize);

        int vCount; file.read(reinterpret_cast<char*>(&vCount), sizeof(int));
        std::vector<TxOut> vouts;
        for (int k = 0; k < vCount; k++) {
            int aSize; file.read(reinterpret_cast<char*>(&aSize), sizeof(int));
            std::string a(aSize, ' '); file.read(&a[0], aSize);
            double amt; file.read(reinterpret_cast<char*>(&amt), sizeof(double));
            vouts.push_back({a, amt});
        }
        Transaction t; t.id = id; t.signature = sig; t.publicKey = pub; t.vout = vouts;
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
    ensure_directory();
    std::ofstream file(filename, std::ios::binary | std::ios::trunc);
    if (!file.is_open()) {
        std::cerr << "[ERRO] Falha ao salvar Blockchain em " << filename << std::endl;
        return;
    }

    std::vector<Block> chain = bc.getChain();
    int chainSize = static_cast<int>(chain.size());
    file.write(reinterpret_cast<const char*>(&chainSize), sizeof(int));

    for (const auto& block : chain) {
        file.write(reinterpret_cast<const char*>(&block.index), sizeof(int));
        long long ts = block.timestamp;
        file.write(reinterpret_cast<const char*>(&ts), sizeof(long long));
        
        int hSize = static_cast<int>(block.hash.size());
        file.write(reinterpret_cast<const char*>(&hSize), sizeof(int));
        file.write(block.hash.c_str(), hSize);
        
        int phSize = static_cast<int>(block.prevHash.size());
        file.write(reinterpret_cast<const char*>(&phSize), sizeof(int));
        file.write(block.prevHash.c_str(), phSize);
        
        file.write(reinterpret_cast<const char*>(&block.nonce), sizeof(int));
        
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
    std::cout << "[INFO] Blockchain persistida: " << chainSize << " blocos." << std::endl;
}

void Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cout << "[INFO] Arquivo nao encontrado. Iniciando Genesis." << std::endl;
        return;
    }

    int chainSize;
    if (!file.read(reinterpret_cast<char*>(&chainSize), sizeof(int))) return;

    bc.clearChain();
    for (int i = 0; i < chainSize; i++) {
        int index; long long timestamp; int nonce;
        file.read(reinterpret_cast<char*>(&index), sizeof(int));
        file.read(reinterpret_cast<char*>(&timestamp), sizeof(long long));
        int hSize; file.read(reinterpret_cast<char*>(&hSize), sizeof(int));
        std::string h(hSize, ' '); file.read(&h[0], hSize);
        int phSize; file.read(reinterpret_cast<char*>(&phSize), sizeof(int));
        std::string ph(phSize, ' '); file.read(&ph[0], phSize);
        file.read(reinterpret_cast<char*>(&nonce), sizeof(int));
        int txCount; file.read(reinterpret_cast<char*>(&txCount), sizeof(int));
        std::vector<Transaction> txs;
        for (int j = 0; j < txCount; j++) {
            int idS; file.read(reinterpret_cast<char*>(&idS), sizeof(int));
            std::string id(idS, ' '); file.read(&id[0], idS);
            int sS; file.read(reinterpret_cast<char*>(&sS), sizeof(int));
            std::string sig(sS, ' '); file.read(&sig[0], sS);
            int pS; file.read(reinterpret_cast<char*>(&pS), sizeof(int));
            std::string pub(pS, ' '); file.read(&pub[0], pS);
            int outCount; file.read(reinterpret_cast<char*>(&outCount), sizeof(int));
            std::vector<TxOut> vouts;
            for (int k = 0; k < outCount; k++) {
                int aSize; file.read(reinterpret_cast<char*>(&aSize), sizeof(int));
                std::string a(aSize, ' '); file.read(&a[0], aSize);
                double amt; file.read(reinterpret_cast<char*>(&amt), sizeof(double));
                vouts.push_back({a, amt});
            }
            Transaction t; t.id = id; t.signature = sig; t.publicKey = pub; t.vout = vouts;
            txs.push_back(t);
        }
        Block b(index, ph, txs);
        b.hash = h; b.timestamp = timestamp; b.nonce = nonce;
        bc.addBlock(b);
    }
    file.close();
    std::cout << "[INFO] Blockchain carregada com sucesso." << std::endl;
}
