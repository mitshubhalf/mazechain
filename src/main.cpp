#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/wallet.h"
#include <iostream>
#include <sys/stat.h>
#include <vector>
#include <string>

int main(int argc, char* argv[]) {
    // Cria a pasta data se ela não existir
    #ifdef _WIN32
        mkdir("data");
    #else
        mkdir("data", 0777);
    #endif

    Blockchain bc;

    // Carrega a blockchain existente
    Storage::loadChain(bc, "data/blockchain.dat");

    if (argc < 2) {
        std::cout << "--- 🔗 MAZECHAIN CLI ---" << std::endl;
        std::cout << "Usage: ./mazechain [command] [args]\n\n";
        std::cout << "Commands:\n";
        std::cout << "  wallet create           - Gera uma nova carteira (12 palavras)\n";
        std::cout << "  wallet load <palavras>  - Recupera endereço de 12 palavras\n";
        std::cout << "  mine <address>          - Minera um bloco para recompensar um endereço\n";
        std::cout << "  balance <address>       - Consulta o saldo de um endereço\n";
        std::cout << "  send <from> <to> <amt>  - Envia moedas (cobra 1% de taxa)\n";
        return 0;
    }

    std::string cmd = argv[1];

    try {
        // --- COMANDOS DE WALLET ---
        if (cmd == "wallet" && argc > 2) {
            std::string sub = argv[2];
            
            if (sub == "create") {
                std::string mnem = Wallet::generateMnemonic();
                std::string addr = Wallet::deriveAddress(mnem);
                std::cout << "✨ NOVA CARTEIRA GERADA!" << std::endl;
                std::cout << "📝 SEED (12 palavras): " << mnem << std::endl;
                std::cout << "💳 ENDEREÇO: " << addr << std::endl;
                std::cout << "⚠️  AVISO: Guarde sua SEED em local seguro!" << std::endl;
            } 
            else if (sub == "load") {
                if (argc < 15) { // "wallet" + "load" + 12 palavras = 14 argumentos extras
                    std::cout << "❌ Erro: Forneça as 12 palavras da sua seed.\n";
                } else {
                    std::string mnem = "";
                    for(int i = 3; i < 15; i++) {
                        mnem += argv[i];
                        if(i < 14) mnem += " ";
                    }
                    std::cout << "🔑 CARTEIRA CARREGADA!" << std::endl;
                    std::cout << "💳 ENDEREÇO: " << Wallet::deriveAddress(mnem) << std::endl;
                }
            }
        }
        // --- COMANDOS DE BLOCKCHAIN ---
        else if (cmd == "mine" && argc > 2) {
            bc.mineBlock(argv[2]);
        }
        else if (cmd == "balance" && argc > 2) {
            std::cout << "💰 Balance of " << argv[2] << ": " << bc.getBalance(argv[2]) << "\n";
        }
        else if (cmd == "send" && argc > 4) {
            bc.send(argv[2], argv[3], std::stod(argv[4]));
        }
        else {
            std::cout << "❌ Comando inválido ou argumentos faltando.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Erro: " << e.what() << "\n";
    }

    return 0;
}
