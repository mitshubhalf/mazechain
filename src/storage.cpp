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

// ---------------- SAVE MEMPOOL ----------------
void Storage::saveMempool(const Transaction& tx, const std::string& filename) {
    ensure_directory();
    std::ofstream file(filename, std::ios::binary | std::ios::app);
    if (!file.is_open()) return;

    int idSize = tx.id.size();
    file.write((char*)&idSize, sizeof(int));
    if (idSize > 0) file.write(tx.id.c_str(), idSize);

    int sigSize = tx.signature.size();
    file.write((char*)&sigSize, sizeof(int));
    if (sigSize > 0) file.write(tx.signature.c_str(), sigSize);

    int pubSize = tx.publicKey.size();
    file.write((char*)&pubSize, sizeof(int));
    if (pubSize > 0) file.write(tx.publicKey.c_str(), pubSize);

    int vCount = tx.vout.size();
    file.write((char*)&vCount, sizeof(int));

    for (const auto& out : tx.vout) {
        int aSize = out.address.size();
        file.write((char*)&aSize, sizeof(int));
        if (aSize > 0) file.write(out.address.c_str(), aSize);
        file.write((char*)&out.amount, sizeof(double));
    }

    file.close();
}

// ---------------- LOAD MEMPOOL ----------------
std::vector<Transaction> Storage::loadMempool(const std::string& filename) {
    std::vector<Transaction> txs;
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return txs;

    while (true) {
        int idSize;
        if (!file.read((char*)&idSize, sizeof(int))) break;
        if (idSize < 0 || idSize > 10000) break;

        std::string id(idSize, '\0');
        if (idSize > 0) file.read(&id[0], idSize);

        int sigSize;
        file.read((char*)&sigSize, sizeof(int));
        if (sigSize < 0 || sigSize > 10000) break;

        std::string sig(sigSize, '\0');
        if (sigSize > 0) file.read(&sig[0], sigSize);

        int pubSize;
        file.read((char*)&pubSize, sizeof(int));
        if (pubSize < 0 || pubSize > 10000) break;

        std::string pub(pubSize, '\0');
        if (pubSize > 0) file.read(&pub[0], pubSize);

        int vCount;
        file.read((char*)&vCount, sizeof(int));
        if (vCount < 0 || vCount > 1000) break;

        std::vector<TxOut> vouts;
        for (int k = 0; k < vCount; k++) {
            int aSize;
            file.read((char*)&aSize, sizeof(int));
            if (aSize < 0 || aSize > 10000) break;

            std::string a(aSize, '\0');
            if (aSize > 0) file.read(&a[0], aSize);

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

// ---------------- SAVE CHAIN ----------------
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
        if (hSize > 0) file.write(block.hash.c_str(), hSize);

        int phSize = block.prevHash.size();
        file.write((char*)&phSize, sizeof(int));
        if (phSize > 0) file.write(block.prevHash.c_str(), phSize);

        file.write((char*)&block.nonce, sizeof(int));

        int txCount = block.transactions.size();
        file.write((char*)&txCount, sizeof(int));

        for (const auto& tx : block.transactions) {
            int idS = tx.id.size();
            file.write((char*)&idS, sizeof(int));
            if (idS > 0) file.write(tx.id.c_str(), idS);

            int sS = tx.signature.size();
            file.write((char*)&sS, sizeof(int));
            if (sS > 0) file.write(tx.signature.c_str(), sS);

            int pS = tx.publicKey.size();
            file.write((char*)&pS, sizeof(int));
            if (pS > 0) file.write(tx.publicKey.c_str(), pS);

            int outCount = tx.vout.size();
            file.write((char*)&outCount, sizeof(int));

            for (const auto& out : tx.vout) {
                int aSize = out.address.size();
                file.write((char*)&aSize, sizeof(int));
                if (aSize > 0) file.write(out.address.c_str(), aSize);
                file.write((char*)&out.amount, sizeof(double));
            }
        }
    }

    file.close();
}

// ---------------- LOAD CHAIN ----------------
bool Storage::loadChain(Blockchain& bc, const std::string& filename) {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) return false;

    int chainSize;
    if (!file.read((char*)&chainSize, sizeof(int))) return false;
    if (chainSize < 0 || chainSize > 100000) return false;

    bc.clearChain();

    for (int i = 0; i < chainSize; i++) {
        int idx;
        long long ts;
        int nonce;

        if (!file.read((char*)&idx, sizeof(int))) break;
        file.read((char*)&ts, sizeof(long long));

        int hS;
        file.read((char*)&hS, sizeof(int));
        if (hS < 0 || hS > 10000) break;

        std::string h(hS, '\0');
        if (hS > 0) file.read(&h[0], hS);

        int phS;
        file.read((char*)&phS, sizeof(int));
        if (phS < 0 || phS > 10000) break;

        std::string ph(phS, '\0');
        if (phS > 0) file.read(&ph[0], phS);

        file.read((char*)&nonce, sizeof(int));

        int txC;
        file.read((char*)&txC, sizeof(int));
        if (txC < 0 || txC > 10000) break;

        std::vector<Transaction> txs;

        for (int j = 0; j < txC; j++) {
            int idS;
            file.read((char*)&idS, sizeof(int));
            if (idS < 0 || idS > 10000) break;

            std::string id(idS, '\0');
            if (idS > 0) file.read(&id[0], idS);

            int sS;
            file.read((char*)&sS, sizeof(int));
            if (sS < 0 || sS > 10000) break;

            std::string sig(sS, '\0');
            if (sS > 0) file.read(&sig[0], sS);

            int pS;
            file.read((char*)&pS, sizeof(int));
            if (pS < 0 || pS > 10000) break;

            std::string pub(pS, '\0');
            if (pS > 0) file.read(&pub[0], pS);

            int outC;
            file.read((char*)&outC, sizeof(int));
            if (outC < 0 || outC > 10000) break;

            std::vector<TxOut> vouts;

            for (int k = 0; k < outC; k++) {
                int aS;
                file.read((char*)&aS, sizeof(int));
                if (aS < 0 || aS > 10000) break;

                std::string a(aS, '\0');
                if (aS > 0) file.read(&a[0], aS);

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

        Block b(idx, ph, txs);
        b.hash = h;
        b.timestamp = ts;
        b.nonce = nonce;

        bc.addBlock(b);
    }

    file.close();

    std::cout << "✅ Blockchain carregada com sucesso. Altura: "
              << bc.getChain().size() << std::endl;

    return true;
}

// ---------------- CLEAR MEMPOOL ----------------
void Storage::clearMempool(const std::string& filename) {
    std::ofstream file(filename, std::ios::trunc);
    file.close();
}
