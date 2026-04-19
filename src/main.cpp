#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include "../include/transaction.h"
#include <iostream>
#include <sys/stat.h>
#include <string>
#include <iomanip>
#include <fstream>
#include <vector>

#ifdef _WIN32
    #include <direct.h>
    #define MKDIR(path) _mkdir(path)
#else
    #include <sys/types.h>
    #define MKDIR(path) mkdir(path, 0777)
#endif

void print_full_usage() {
    std::cout << "==========================================\n";
    std::cout << "          MAZECHAIN CORE v2.1              \n";
    std::cout << "    ECONOMY: 20M MZ | UNIT: MITS          \n";
    std::cout << "==========================================\n";
    std::cout << "Uso: ./mazechain [comando] [argumentos]\n\n";
    std::cout << "Comandos de Carteira:\n";
    std::cout << "  wallet create                - Gera nova carteira (Endereço + Seed)\n";
    std::cout << "  wallet from-seed \"<seed>\"    - Recupera endereço através da seed\n";
    std::cout << "\nComandos de Rede:\n";
    std::cout << "  mine [endereco]              - Minera um bloco para o endereço\n";
    std::cout << "  balance [endereco]           - Consulta saldo e unidades Mits\n";
    std::cout << "  send [de] [para] [qtd] [seed] - Envia MZ tokens\n";
    std::cout << "  mempool                      - Lista transações aguardando mineração\n";
    std::cout << "  chain                        - Lista o histórico de blocos\n";
    std::cout << "  chain stats                  - Estatísticas e Supply da rede\n";
    std::cout << "  chain validate               - Valida integridade total da chain\n";
    std::cout << "==========================================\n";
}

int main(int argc, char* argv[]) {
    // 1. Garante a pasta de dados
    MKDIR("data");

    // 2. Verificação da wordlist
    std::string wordlist_path = "wordlist.txt";
    std::ifstream check_wordlist(wordlist_path);
    if (!check_wordlist.is_open()) {
        check_wordlist.open("../wordlist.txt");
        if (check_wordlist.is_open()) {
            wordlist_path = "../wordlist.txt";
        } else {
            std::cerr << "❌ ERRO: wordlist.txt não encontrada!\n";
            return 1;
        }
    }
    check_wordlist.close();

    // 3. Inicializa e carrega blockchain (Silencioso se já existir)
    Blockchain bc;
    std::ifstream check_db("data/blockchain.dat");
    bool db_exists = check_db.good();
    check_db.close();

    // Tenta carregar. Se falhar e não existir arquivo, cria o gênesis.
    if (!Storage::loadChain(bc, "data/blockchain.dat")) {
        if (!db_exists) {
            // O Bloco Gênesis é criado automaticamente pelo construtor da Blockchain
            std::cout << "[SISTEMA] Bloco Gênesis estabelecido com sucesso.\n";
        }
    }

    if (argc < 2) {
        print_full_usage();
        return 0;
    }

    std::string cmd = argv[1];

    // ---------------- COMANDOS WALLET ----------------
    if (cmd == "wallet") {
        if (argc < 3) {
            std::cout << "Uso: ./mazechain wallet <create|from-seed>\n";
            return 1;
        }
        std::string sub = argv[2];

        if (sub == "create") {
            Wallet w;
            w.create();
            std::cout << "------------------------------------------\n";
            std::cout << "✅ MAZECHAIN: CARTEIRA GERADA COM SUCESSO\n";
            std::cout << "ADDRESS: " << w.address << "\n";
            std::cout << "SEED   : " << w.seed << "\n";
            std::cout << "⚠️ Guarde sua SEED em lugar seguro!\n";
            std::cout << "------------------------------------------\n";
            return 0;
        } 
        else if (sub == "from-seed") {
            if (argc < 4) {
                std::cout << "Erro: Forneça a seed entre aspas.\n";
                return 1;
            }
            std::string full_seed = "";
            for (int i = 3; i < argc; i++) {
                if (i > 3) full_seed += " ";
                full_seed += argv[i];
            }
            Wallet w;
            w.fromSeed(full_seed);
            std::cout << "\n✅ Carteira Recuperada:\n";
            std::cout << "ADDRESS: " << w.address << "\n";
            return 0;
        }
    }

    // ---------------- COMANDO MINE (COM VERIFICAÇÃO ANTI-FRAUDE) ----------------
    else if (cmd == "mine" && argc > 2) {
        // SEGURANÇA: Antes de minerar, valida a chain atual para evitar minerar sobre dados falsos
        if (!bc.isChainValid()) {
            std::cout << "❌ MINERAÇÃO REJEITADA: A blockchain atual contém dados fraudados!\n";
            std::cout << "⚠️ O sistema detectou alteração em blocos anteriores. Corrija a chain para prosseguir.\n";
            return 1;
        }

        std::string minerAddress = argv[2];
        int height = (int)bc.getChain().size();
        std::cout << "⛏️ Iniciando mineração do bloco #" << height << "...\n";
        
        bc.mineBlock(minerAddress);
        Storage::saveChain(bc, "data/blockchain.dat");
        Storage::clearMempool("data/mempool.dat");

        std::cout << "✅ Bloco minerado com sucesso!\n";
        std::cout << "🎁 Recompensa: " << bc.getBlockReward(height) << " MZ\n";
        return 0;
    }

    // ---------------- COMANDO MEMPOOL (CORRIGIDO) ----------------
    else if (cmd == "mempool") {
        std::cout << "--- TRANSAÇÕES PENDENTES (MEMPOOL) ---\n";
        auto pending = Storage::loadMempool("data/mempool.dat");
        if (pending.empty()) {
            std::cout << "Nenhuma transação aguardando mineração.\n";
        } else {
            for (const auto& tx : pending) {
                // Ajustado para nomes de membros comuns se 'sender' falhar
                // Se o seu Transaction.h usar outros nomes, ajuste aqui:
                std::cout << "Origem: " << tx.fromAddress 
                          << " | Destino: " << tx.toAddress 
                          << " | Valor: " << tx.value << " MZ\n";
            }
        }
        return 0;
    }

    // ---------------- COMANDO BALANCE ----------------
    else if (cmd == "balance" && argc > 2) {
        double balance = bc.getBalance(argv[2]);
        std::cout << "------------------------------------------\n";
        std::cout << "💰 Endereço: " << argv[2] << "\n";
        std::cout << std::fixed << std::setprecision(8);
        std::cout << "💰 Saldo:    " << balance << " MZ\n";
        std::cout << "💎 Mits:     " << (long long)(balance * 100000000.0) << " units\n";
        std::cout << "------------------------------------------\n";
        return 0;
    }

    // ---------------- COMANDO SEND ----------------
    else if (cmd == "send" && argc > 5) {
        try {
            double amount = std::stod(argv[4]);
            bc.send(argv[2], argv[3], amount, argv[5]);
            Storage::saveChain(bc, "data/blockchain.dat");
            // Nota: O send envia para o mempool, a gravação da chain aqui 
            // garante o estado atual, mas a tx só entra no bloco no comando 'mine'
            std::cout << "✅ Transação enviada para a rede com sucesso.\n";
        } catch (const std::exception& e) {
            std::cerr << "❌ Erro na transação: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    // ---------------- COMANDOS CHAIN ----------------
    else if (cmd == "chain") {
        std::string sub = (argc > 2) ? argv[2] : "";

        if (sub == "stats") {
            std::cout << "--- ESTATÍSTICAS DA REDE ---\n";
            std::cout << "Blocos Minerados: " << bc.getChain().size() << "\n";
            std::cout << std::fixed << std::setprecision(8);
            std::cout << "Circulação Total: " << bc.getTotalSupply() << " MZ\n";
            bc.printStats();
        } 
        else if (sub == "validate") {
            std::cout << "\n🔍 Verificando integridade da blockchain...\n";
            if (bc.isChainValid()) {
                std::cout << "✅ Resultado: Blockchain íntegra e validada.\n";
            } else {
                std::cout << "❌ CRÍTICO: Integridade da blockchain comprometida!\n";
            }
        } 
        else {
            std::cout << "--- HISTÓRICO DE BLOCOS ---\n";
            for (const auto& b : bc.getChain()) {
                std::cout << "Bloco #" << b.index 
                          << " | Hash: " << b.hash.substr(0,16) << "..."
                          << " | Txs: " << b.transactions.size() << "\n";
            }
        }
        return 0;
    }

    std::cout << "❓ Comando desconhecido. Digite apenas ./mazechain para ajuda.\n";
    return 0;
}
