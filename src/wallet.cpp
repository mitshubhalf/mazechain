#include "../include/wallet.h"
#include <openssl/sha.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <algorithm>

// Lista simplificada de palavras (No Bitcoin real são 2048 palavras)
const std::vector<std::string> wordlist = {
    "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract", "absurd", "abuse",
    "access", "accident", "account", "accuse", "achieve", "acid", "acoustic", "acquire", "across", "act",
    "action", "actor", "actress", "actual", "adapt", "add", "addict", "address", "adjust", "admit",
    "adult", "advance", "advice", "aerobic", "affair", "afford", "afraid", "again", "age", "agent"
    // ... adicione mais palavras se desejar
};

std::string Wallet::generateMnemonic() {
    std::string mnemonic = "";
    std::srand(std::time(0));
    for(int i = 0; i < 12; i++) {
        mnemonic += wordlist[std::rand() % wordlist.size()];
        if(i < 11) mnemonic += " ";
    }
    return mnemonic;
}

std::string Wallet::deriveAddress(std::string mnemonic) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)mnemonic.c_str(), mnemonic.length(), hash);

    std::stringstream ss;
    for(int i = 0; i < 20; i++) { // Usamos os primeiros 20 bytes para o endereço
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return "MZ" + ss.str(); // Prefixo da Mazechain
}
