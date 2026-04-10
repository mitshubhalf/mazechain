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

    // Carrega a blockchain existente para ter os saldos atualizados
    Storage::loadChain(bc, "data/blockchain.dat");

    if (argc < 2) {
        std::cout << "--- 🔗 MAZECHAIN CLI V2 ---" << std::endl;
        std::cout << "Usage: ./mazechain [command] [args]\n\n";
        std::cout << "Commands:\n";
        std::cout << "  wallet create           - Gera uma nova carteira (12 palavras)\n";
        std::cout << "  wallet load <12 words>  - Recupera endereço MZ através da seed\n";
        std::cout << "  mine <address>          - Minera um bloco para recompensar um endereço\n";
        std::cout << "  balance <address>       - Consulta o saldo real (entradas - saídas)\n";
        std::cout << "  send <from> <to> <amt>  - Envia moedas (taxa de 1% inclusa)\n";
        return 0;
    }

    std::string cmd = argv[1];

    try {
        // --- COMANDOS DE CARTEIRA (WALLET) ---
        if (cmd == "wallet" && argc > 2) {
            std::string sub = argv[2];
            
            if (sub == "create") {
                // Chamada static: não precisa de objeto 'Wallet w'
                std::string mnem = Wallet::generateMnemonic();
                std::string addr = Wallet::deriveAddress(mnem);
                
                std::cout << "\n✨ NOVA CARTEIRA GERADA!" << std::endl;
                std::cout << "------------------------------------------" << std::endl;
                std::cout << "📝 SEED (12 palavras):\n   " << mnem << std::endl;
                std::cout << "------------------------------------------" << std::endl;
                std::cout << "💳 ENDEREÇO: " << addr << std::endl;
                std::cout << "⚠️  AVISO: Se perder a seed, perderá suas moedas!" << std::endl;
            } 
            else if (sub == "load") {
                if (argc < 14 && argc != 4) { 
                    std::cout << "❌ Erro: Forneça as 12 palavras da sua seed.\n";
                } else {
                    std::string mnem = "";
                    // Se as palavras vierem separadas por espaços
                    if (argc >= 14) {
                        for(int i = 3; i < 15; i++) {
                            mnem += argv[i];
                            if(i < 14) mnem += " ";
                        }
                    } else { // Se vierem entre aspas como um único argumento
                        mnem = argv[3];
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
            double bal = bc.getBalance(argv[2]);
            std::cout << "💰 Endereço: " << argv[2] << "\n💵 Saldo: " << bal << " MZ\n";
        }
        else if (cmd == "send" && argc > 4) {
            // A função send no blockchain.cpp já cuida da taxa de 1%
            bc.send(argv[2], argv[3], std::stod(argv[4]));
        }
        else {
            std::cout << "❌ Comando inválido ou argumentos insuficientes.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "❌ Erro de Execução: " << e.what() << "\n";
    }

    return 0;
}
