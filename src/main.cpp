#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <iostream>
#include <sys/stat.h>
#include <string>
#include <iomanip>
#include <fstream>

int main(int argc, char* argv[]) {
    // 1. Garante a pasta de dados
    #ifdef _WIN32
        mkdir("data");
    #else
        mkdir("data", 0777);
    #endif

    // 2. Verificação de segurança da wordlist (BIP-39)
    std::ifstream check_wordlist("wordlist.txt");
    if (!check_wordlist.is_open()) {
        std::cerr << "❌ ERRO: wordlist.txt não encontrada na raiz!\n";
        std::cerr << "Execute: curl -L https://raw.githubusercontent.com/bitcoin/bips/master/bip-0039/english.txt -o wordlist.txt\n";
        return 1;
    }
    check_wordlist.close();

    Blockchain bc;
    // Tenta carregar a chain existente
    if (!Storage::loadChain(bc, "data/blockchain.dat")) {
        // Se não carregar, o bloco gênesis é criado automaticamente pelo construtor da Blockchain
    }

    if (argc < 2) {
        std::cout << "==========================================\n";
        std::cout << "          MAZECHAIN CORE v2.1             \n";
        std::cout << "    ECONOMY: 20M MZ | UNIT: MITS          \n";
        std::cout << "==========================================\n";
        std::cout << "Usage: ./mazechain [comando] [argumentos]\n\n";
        std::cout << "Comandos disponíveis:\n";
        std::cout << "  wallet create          - Gera uma nova carteira\n";
        std::cout << "  mine [endereco]        - Minera um bloco (Recompensa + Taxas)\n";
        std::cout << "  balance [endereco]     - Consulta o saldo em MZ e Mits\n";
        std::cout << "  send [de] [para] [qtd] [seed] - Envia moedas\n";
        std::cout << "  mempool                - Lista transações pendentes\n";
        std::cout << "  chain                  - Exibe a blockchain completa\n";
        std::cout << "  verify                 - Valida a integridade da rede\n";
        std::cout << "  stats                  - Estatísticas de fornecimento\n";
        return 0;
    }

    std::string cmd = argv[1];

    // --- COMANDO: WALLET CREATE ---
    if (cmd == "wallet" && argc > 2) {
        if (std::string(argv[2]) == "create") {
            Wallet w;
            w.create();
            std::cout << "------------------------------------------\n";
            std::cout << "✅ MAZECHAIN: CARTEIRA GERADA COM SUCESSO\n";
            std::cout << "ADDRESS: " << w.address << "\n";
            std::cout << "SEED   : " << w.seed << "\n";
            std::cout << "⚠️ Guarde sua SEED em lugar seguro!\n";
            std::cout << "------------------------------------------\n";
        }
    }
    // --- COMANDO: MINE ---
    else if (cmd == "mine" && argc > 2) {
        std::string minerAddress = argv[2];
        std::cout << "⛏️ Iniciando mineração para: " << minerAddress << "...\n";
        
        bc.mineBlock(minerAddress);
        
        // SALVAMENTO CRÍTICO: Após minerar, salvamos o progresso
        if (Storage::saveChain(bc, "data/blockchain.dat")) {
            Storage::clearMempool("data/mempool.dat");
            std::cout << "✅ Bloco salvo com sucesso no banco de dados.\n";
        } else {
            std::cerr << "❌ ERRO ao salvar a blockchain no disco!\n";
        }
    }
    // --- COMANDO: BALANCE ---
    else if (cmd == "balance" && argc > 2) {
        double balance = bc.getBalance(argv[2]);
        std::cout << "------------------------------------------\n";
        std::cout << "💰 Endereço: " << argv[2] << "\n";
        std::cout << "💰 Saldo: " << std::fixed << std::setprecision(8) << balance << " MZ\n";
        std::cout << "💎 Mits:  " << (long long)(balance * 100000000.0) << " units\n";
        std::cout << "------------------------------------------\n";
    }
    // --- COMANDO: SEND ---
    else if (cmd == "send" && argc > 5) {
        try {
            std::string from = argv[2];
            std::string to = argv[3];
            double amount = std::stod(argv[4]);
            std::string seed = argv[5];
            
            bc.send(from, to, amount, seed);
            // Salva a transação na mempool
            std::cout << "✅ Transação enviada para a mempool com sucesso.\n";
        } catch (const std::exception& e) {
            std::cerr << "❌ Erro na transação: " << e.what() << "\n";
        }
    }
    // --- COMANDO: MEMPOOL ---
    else if (cmd == "mempool") {
        auto pending = Storage::loadMempool("data/mempool.dat");
        std::cout << "📦 Transações aguardando mineração: " << pending.size() << "\n";
        for(const auto& tx : pending) {
            std::cout << " - De: " << tx.sender.substr(0,10) << "... Qtd: " << tx.amount << " MZ\n";
        }
    }
    // --- COMANDO: CHAIN ---
    else if (cmd == "chain") {
        std::cout << "--- HISTÓRICO DA BLOCKCHAIN ---\n";
        for (const auto& b : bc.getChain()) {
            std::cout << "Bloco #" << b.index 
                      << " | Hash: " << b.hash.substr(0,16) 
                      << " | Txs: " << b.transactions.size() 
                      << " | Miner: " << (b.transactions.empty() ? "N/A" : b.transactions[0].receiver.substr(0,8)) << "...\n";
        }
    }
    // --- COMANDO: VERIFY ---
    else if (cmd == "verify") {
        if (bc.isChainValid()) {
            std::cout << "✅ Rede 100% íntegra. Todos os Hashes e Merkle Roots conferem.\n";
        } else {
            std::cout << "🛑 ALERTA: Blockchain corrompida!\n";
        }
    }
    // --- COMANDO: STATS ---
    else if (cmd == "stats") {
        std::cout << "--- ESTATÍSTICAS DA REDE ---\n";
        std::cout << "Blocos atuais: " << bc.getChain().size() << "\n";
        std::cout << "Moedas em circulação: " << bc.getTotalSupply() << " MZ\n";
        std::cout << "Supply Máximo: 20,000,000.00 MZ\n";
    }
    
    return 0;
}
