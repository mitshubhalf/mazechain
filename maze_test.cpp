#include "include/wallet.h"
#include "include/blockchain.h"
#include "include/storage.h"
#include "include/transaction.h"
#include <iostream>
#include <string>
#include <vector>
#include <iomanip>

void print_usage() {
    std::cout << "\nUsage: ./maze_test <command> [subcommand] [args]\n\n";
    std::cout << "Commands:\n";
    std::cout << "  wallet create              Create a new wallet (address + seed)\n";
    std::cout << "  wallet from-seed <seed>    Recover wallet address from existing seed\n";
    std::cout << "  balance <address>          Check balance of an address\n";
    std::cout << "  chain stats                Show blockchain statistics\n";
    std::cout << "  chain validate             Validate the full chain\n";
    std::cout << "  mine <address>             Mine a new block for the given address\n";
    std::cout << "  send <from> <to> <amount> <seed>   Send MZ tokens\n";
    std::cout << "\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_usage();
        return 1;
    }

    std::string cmd = argv[1];

    // --- WALLET COMMANDS ---
    if (cmd == "wallet") {
        if (argc < 3) {
            std::cout << "Usage: ./maze_test wallet <create|from-seed>\n";
            return 1;
        }

        std::string sub = argv[2];

        if (sub == "create") {
            Wallet w;
            w.create();
            return 0;
        }

        if (sub == "from-seed") {
            if (argc < 4) {
                std::cout << "Usage: ./maze_test wallet from-seed \"<12-word seed>\"\n";
                return 1;
            }
            std::string seed = "";
            for (int i = 3; i < argc; i++) {
                if (i > 3) seed += " ";
                seed += argv[i];
            }
            Wallet w;
            w.fromSeed(seed);
            std::cout << "\nWallet recovered from seed:\n";
            std::cout << "ADDRESS: " << w.address << "\n";
            std::cout << "SEED   : " << w.seed << "\n";
            return 0;
        }

        std::cout << "Unknown wallet subcommand: " << sub << "\n";
        print_usage();
        return 1;
    }

    // --- BALANCE COMMAND ---
    if (cmd == "balance") {
        if (argc < 3) {
            std::cout << "Usage: ./maze_test balance <address>\n";
            return 1;
        }
        Blockchain bc;
        if (!Storage::loadChain(bc, "data/blockchain.dat")) {
            std::cout << "[INFO] No existing chain found. Starting fresh.\n";
        }
        std::string addr = argv[2];
        double bal = bc.getBalance(addr);
        std::cout << "\nAddress : " << addr << "\n";
        std::cout << std::fixed << std::setprecision(8);
        std::cout << "Balance : " << bal << " MZ\n";
        std::cout << "Mits    : " << (long long)(bal * 100000000) << " Mits\n";
        return 0;
    }

    // --- CHAIN COMMANDS ---
    if (cmd == "chain") {
        if (argc < 3) {
            std::cout << "Usage: ./maze_test chain <stats|validate>\n";
            return 1;
        }
        std::string sub = argv[2];

        Blockchain bc;
        if (!Storage::loadChain(bc, "data/blockchain.dat")) {
            std::cout << "[INFO] No existing chain found. Starting fresh.\n";
        }

        if (sub == "stats") {
            bc.printStats();
            return 0;
        }

        if (sub == "validate") {
            std::cout << "\nValidating chain...\n";
            bool valid = bc.isChainValid();
            if (valid) {
                std::cout << "✅ Chain is VALID. All blocks verified.\n";
            } else {
                std::cout << "❌ Chain is INVALID. Integrity check failed.\n";
            }
            return valid ? 0 : 1;
        }

        std::cout << "Unknown chain subcommand: " << sub << "\n";
        print_usage();
        return 1;
    }

    // --- MINE COMMAND ---
    if (cmd == "mine") {
        if (argc < 3) {
            std::cout << "Usage: ./maze_test mine <address>\n";
            return 1;
        }
        Blockchain bc;
        if (!Storage::loadChain(bc, "data/blockchain.dat")) {
            std::cout << "[INFO] No existing chain found. Starting fresh.\n";
        }
        std::string addr = argv[2];
        int height = (int)bc.getChain().size();
        std::cout << "\nMining block " << height << " for " << addr << "...\n";
        bc.mineBlock(addr);
        Storage::saveChain(bc, "data/blockchain.dat");
        Storage::clearMempool("data/mempool.dat");
        std::cout << "✅ Block mined! Reward: " << bc.getBlockReward(height) << " MZ\n";
        return 0;
    }

    // --- SEND COMMAND ---
    if (cmd == "send") {
        if (argc < 6) {
            std::cout << "Usage: ./maze_test send <from> <to> <amount> <seed>\n";
            return 1;
        }
        Blockchain bc;
        if (!Storage::loadChain(bc, "data/blockchain.dat")) {
            std::cout << "[INFO] No existing chain found. Starting fresh.\n";
        }
        std::string from   = argv[2];
        std::string to     = argv[3];
        double amount      = std::stod(argv[4]);
        std::string seed   = argv[5];
        try {
            bc.send(from, to, amount, seed);
            Storage::saveChain(bc, "data/blockchain.dat");
            std::cout << "✅ Transaction sent to mempool.\n";
        } catch (const std::exception& e) {
            std::cerr << "❌ Error: " << e.what() << "\n";
            return 1;
        }
        return 0;
    }

    std::cout << "Unknown command: " << cmd << "\n";
    print_usage();
    return 1;
}
