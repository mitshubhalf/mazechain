#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <iostream>
#include <sys/stat.h>
#include <string>
#include <iomanip>

int main(int argc, char* argv[]) {
    // Garante a pasta de dados
    #ifdef _WIN32
        mkdir("data");
    #else
        mkdir("data", 0777);
    #endif

    Blockchain bc;
    // Tenta carregar a chain existente
    Storage::loadChain(bc, "data/blockchain.dat");

    if (argc < 2) {
        std::cout << "==========================================\n";
        std::cout << "          MAZECHAIN CORE v2.1             \n";
        std::cout << "   ECONOMY: 20M MZ | UNIT: MITS           \n";
        std::cout << "==========================================\n";
        std::cout << "Usage: ./mazechain [comando] [argumentos]\n\n";
        std::cout << "Comandos disponíveis:\n";
        std::cout << "  wallet create          - Gera uma nova carteira\n";
        std::cout << "  mine [endereco]        - Minera um bloco (Recompensa + Taxas)\n";
        std::cout << "  balance [endereco]     - Consulta o saldo em MZ e Mits\n";
        std::cout << "  send [de] [para] [qtd] [seed] - Envia moedas (Taxa 1%)\n";
        std::cout << "  mempool                - Lista transações pendentes\n";
        std::cout << "  chain                  - Exibe a blockchain completa\n";
        std::cout << "  verify                 - Valida a integridade da rede\n";
        std::cout << "  stats                  - Estatísticas de fornecimento (Supply)\n";
        return 0;
    }

    std::string cmd = argv[1];

    if (cmd == "wallet" && argc > 2) {
        if (std::string(argv[2]) == "create") {
            Wallet w;
            w.create();
            // O Wallet::create() já deve imprimir o endereço e a seed na tela
        }
    }
    else if (cmd == "mine" && argc > 2) {
        // Agora usa a lógica de mineração manual que recompensa o endereço fornecido
        bc.mineBlock(argv[2]);
    }
    else if (cmd == "balance" && argc > 2) {
        double balance = bc.getBalance(argv[2]);
        std::cout << "------------------------------------------\n";
        std::cout << "💰 Endereço: " << argv[2] << "\n";
        std::cout << "💰 Saldo: " << std::fixed << std::setprecision(8) << balance << " MZ\n";
        std::cout << "💎 Mits:  " << (long long)(balance * 100000000) << " units\n";
        std::cout << "------------------------------------------\n";
    }
    else if (cmd == "send" && argc > 5) {
        std::string from = argv[2];
        std::string to = argv[3];
        double amount = std::stod(argv[4]);
        std::string seed = argv[5];
        
        bc.send(from, to, amount, seed);
    }
    else if (cmd == "mempool") {
        auto pending = Storage::loadMempool("data/mempool.dat");
        std::cout << "📦 Transações aguardando mineração: " << pending.size() << std::endl;
        for(const auto& tx : pending) {
            std::cout << " - ID: " << tx.id.substr(0,16) << "... [Pendente]\n";
        }
    }
    else if (cmd == "chain") {
        std::cout << "--- HISTÓRICO DA BLOCKCHAIN ---\n";
        for (const auto& b : bc.getChain()) {
            std::cout << "Bloco #" << b.index 
                      << " | Hash: " << b.hash.substr(0,16) << "..."
                      << " | Txs: " << b.transactions.size() << "\n";
        }
    }
    else if (cmd == "verify") {
        if (bc.isChainValid()) {
            std::cout << "✅ Rede 100% íntegra. Todos os Hashes e Merkle Roots conferem." << std::endl;
        } else {
            std::cout << "🛑 ALERTA: Blockchain corrompida ou alterada ilegalmente!" << std::endl;
        }
    }
    else if (cmd == "stats") {
        bc.printStats();
    }
    
    return 0;
}
