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
    std::cout << "  chain validate                - Valida integridade total da chain\n";
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

    // 3. Inicializa Blockchain
    // O construtor de Blockchain já chama Storage::loadChain e faz a filtragem resiliente.
    // Não precisamos chamar Storage::loadChain aqui de novo, pois isso causaria duplicidade ou erros.
    Blockchain bc;

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

    // ---------------- COMANDO MINE (REFORMULADO) ----------------
    else if (cmd == "mine" && argc > 2) {
        // Agora confiamos na validação interna da classe Blockchain.
        // Se o construtor carregou a chain, o que está na memória é VÁLIDO.
        // O mineBlock usará a chain da memória para criar o próximo bloco e 
        // o Storage::saveChain irá limpar o lixo do arquivo .dat automaticamente.

        std::string minerAddress = argv[2];
        int height = (int)bc.getChain().size();

        std::cout << "⛏️ Preparando mineração do bloco #" << height << "...\n";

        // Verificação de segurança em memória antes de minerar
        if (!bc.isChainValid()) {
            std::cout << "❌ ERRO CRÍTICO: A chain em memória está inconsistente.\n";
            return 1;
        }

        bc.mineBlock(minerAddress);

        // Note: bc.mineBlock já deve chamar Storage::saveChain internamente conforme seu blockchain.cpp
        // Mas mantemos aqui para garantir a persistência imediata.
        Storage::saveChain(bc, "data/blockchain.dat");
        Storage::clearMempool("data/mempool.dat");

        std::cout << "✅ Operação concluída.\n";
        return 0;
    }

    // ---------------- COMANDO MEMPOOL ----------------
    else if (cmd == "mempool") {
        std::cout << "--- TRANSAÇÕES PENDENTES (MEMPOOL) ---\n";
        auto pending = Storage::loadMempool("data/mempool.dat");
        if (pending.empty()) {
            std::cout << "Nenhuma transação aguardando mineração.\n";
        } else {
            for (const auto& tx : pending) {
                if (!tx.vout.empty()) {
                    std::cout << "ID: " << tx.id.substr(0,10) << "..."
                              << " | Destino: " << tx.vout[0].address 
                              << " | Valor: " << tx.vout[0].amount << " MZ\n";
                    if (tx.vout.size() > 1) {
                        std::cout << "   [+] Possui mais " << tx.vout.size() - 1 << " saídas vinculadas.\n";
                    }
                }
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
            // Transações de envio geralmente vão para o mempool e não salvam a chain direto,
            // mas mantemos conforme sua lógica original.
            std::cout << "✅ Transação enviada para o mempool com sucesso.\n";
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
            bc.printStats();
        } 
        else if (sub == "validate") {
            std::cout << "\n🔍 Verificando integridade da memória RAM...\n";
            if (bc.isChainValid()) {
                std::cout << "✅ Resultado: Blockchain carregada está íntegra.\n";
            } else {
                std::cout << "❌ CRÍTICO: Integridade comprometida!\n";
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