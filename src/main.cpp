#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <iostream>
#include <sys/stat.h>
#include <string>

int main(int argc, char* argv[]) {
    // Garante que a pasta data existe
    #ifdef _WIN32
        mkdir("data");
    #else
        mkdir("data", 0777);
    #endif

    Blockchain bc;
    Storage::loadChain(bc, "data/blockchain.dat");

    if (argc < 2) {
        std::cout << "Usage: ./mazechain [mine/balance/send/wallet/chain/mempool/verify/stats]\n";
        return 0;
    }

    std::string cmd = argv[1];

    // Comando Wallet: Agora usa a função create() que sorteia as 2048 palavras
    if (cmd == "wallet" && argc > 2) {
        if (std::string(argv[2]) == "create") {
            Wallet w;
            w.create();
        }
    }
    // Comando Mine: Mineração com Halving e Taxas
    else if (cmd == "mine" && argc > 2) {
        bc.mineBlock(argv[2]);
    }
    // Comando Balance: Consulta saldo
    else if (cmd == "balance" && argc > 2) {
        std::cout << "💰 Saldo: " << bc.getBalance(argv[2]) << " MZ" << std::endl;
    }
    // Comando Send: Envia transação para a Mempool
    else if (cmd == "send" && argc > 4) {
        bc.send(argv[2], argv[3], std::stod(argv[4]));
    }
    // RESTAURADO: Comando Mempool
    else if (cmd == "mempool") {
        auto pending = Storage::loadMempool("data/mempool.dat");
        std::cout << "📦 Transações aguardando na Mempool: " << pending.size() << std::endl;
        for (const auto& tx : pending) {
            std::cout << " - ID: " << tx.id.substr(0,10) << "... [Assinada]" << std::endl;
        }
    }
    // Comando Chain: Lista blocos
    else if (cmd == "chain") {
        for (const auto& b : bc.getChain()) {
            std::cout << "Bloco #" << b.index << " | Hash: " << b.hash.substr(0,10) << " | TXs: " << b.transactions.size() << std::endl;
        }
    }
    // Comando Verify: Valida integridade
    else if (cmd == "verify") {
        if (bc.isChainValid()) std::cout << "✅ Blockchain íntegra!" << std::endl;
        else std::cout << "🚨 ALERTA: Fraude detectada na cadeia!" << std::endl;
    }
    // Comando Stats: Estatísticas Gerais
    else if (cmd == "stats") {
        bc.printStats();
    }
    else {
        std::cout << "❌ Comando desconhecido ou argumentos insuficientes." << std::endl;
    }

    return 0;
}
