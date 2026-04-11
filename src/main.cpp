#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <iostream>
#include <sys/stat.h>
#include <string>
#include <iomanip>

int main(int argc, char* argv[]) {
    // Garante que a pasta data existe (Compatibilidade Windows/Linux)
    #ifdef _WIN32
        mkdir("data");
    #else
        mkdir("data", 0777);
    #endif

    Blockchain bc;
    // Carrega a chain do arquivo logo na inicialização
    Storage::loadChain(bc, "data/blockchain.dat");

    if (argc < 2) {
        std::cout << "==========================================\n";
        std::cout << "          MAZECHAIN CORE v2.0             \n";
        std::cout << "==========================================\n";
        std::cout << "Usage: ./mazechain [comando] [argumentos]\n\n";
        std::cout << "Comandos disponíveis:\n";
        std::cout << "  wallet create          - Gera uma nova carteira (Seed + Endereço)\n";
        std::cout << "  mine [endereco]        - Minera transações da Mempool para sua conta\n";
        std::cout << "  balance [endereco]     - Consulta o saldo total de um endereço\n";
        std::cout << "  send [de] [para] [qtd] - Envia moedas (assina com sua Seed)\n";
        std::cout << "  mempool                - Lista transações aguardando mineração\n";
        std::cout << "  chain                  - Exibe todos os blocos da rede\n";
        std::cout << "  verify                 - Valida a integridade da Blockchain\n";
        std::cout << "  stats                  - Estatísticas de circulação e dificuldade\n";
        return 0;
    }

    std::string cmd = argv[1];

    // --- COMANDO: WALLET ---
    if (cmd == "wallet" && argc > 2) {
        if (std::string(argv[2]) == "create") {
            Wallet w;
            w.create();
        }
    }
    
    // --- COMANDO: MINE ---
    else if (cmd == "mine" && argc > 2) {
        bc.mineBlock(argv[2]);
    }
    
    // --- COMANDO: BALANCE ---
    else if (cmd == "balance" && argc > 2) {
        std::cout << "💰 Saldo: " << std::fixed << std::setprecision(8) << bc.getBalance(argv[2]) << " MZ" << std::endl;
    }
    
    // --- COMANDO: SEND ---
    else if (cmd == "send" && argc > 4) {
        bc.send(argv[2], argv[3], std::stod(argv[4]));
    }
    
    // --- COMANDO: MEMPOOL ---
    else if (cmd == "mempool") {
        auto pending = Storage::loadMempool("data/mempool.dat");
        std::cout << "📦 Transações aguardando na Mempool: " << pending.size() << std::endl;
        for (const auto& tx : pending) {
            std::cout << "------------------------------------------\n";
            std::cout << "ID: " << tx.id << "\n";
            std::cout << "Status: Aguardando Mineração (Assinada)\n";
        }
    }
    
    // --- COMANDO: CHAIN ---
    else if (cmd == "chain") {
        std::cout << "⛓️  CADEIA DE BLOCOS:\n";
        for (const auto& b : bc.getChain()) {
            std::cout << "------------------------------------------\n";
            std::cout << "Bloco #" << b.index << "\n";
            std::cout << "Hash: " << b.hash << "\n";
            std::cout << "Prev: " << b.prevHash << "\n";
            std::cout << "Transações: " << b.transactions.size() << "\n";
            std::cout << "Nonce: " << b.nonce << "\n";
        }
    }
    
    // --- COMANDO: VERIFY ---
    else if (cmd == "verify") {
        if (bc.isChainValid()) {
            std::cout << "✅ Blockchain íntegra! Todos os hashes e raízes de Merkle batem." << std::endl;
        } else {
            std::cout << "🚨 ALERTA: Fraude detectada ou arquivo corrompido!" << std::endl;
        }
    }
    
    // --- COMANDO: STATS ---
    else if (cmd == "stats") {
        bc.printStats();
        std::cout << "Dificuldade Atual: " << bc.getDifficulty() << std::endl;
    }
    
    else {
        std::cout << "❌ Comando desconhecido ou argumentos insuficientes." << std::endl;
    }

    return 0;
}
