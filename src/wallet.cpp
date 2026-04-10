#include "../include/wallet.h"
#include <openssl/sha.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>

// No Bitcoin real, essa lista tem 2048 palavras (BIP-39 english)
// Aqui está uma amostra representativa. 
const std::vector<std::string> bip39_words = {
    "abandon", "ability", "able", "about", "above", "absent", "absorb", "abstract", "absurd", "abuse",
    "access", "accident", "account", "accuse", "achieve", "acid", "acoustic", "acquire", "across", "act",
    "action", "actor", "actress", "actual", "adapt", "add", "addict", "address", "adjust", "admit",
    "adult", "advance", "advice", "aerobic", "affair", "afford", "afraid", "again", "age", "agent",
    "agree", "ahead", "aim", "air", "airport", "aisle", "alarm", "album", "alcohol", "alert"
    // Para 2048 palavras, o ideal é carregar um arquivo externo .txt
};

std::string Wallet::generateMnemonic() {
    std::string mnemonic = "";
    std::srand(std::time(0));
    
    for(int i = 0; i < 12; i++) {
        // No sistema real: random index entre 0 e 2047
        int index = std::rand() % bip39_words.size(); 
        mnemonic += bip39_words[index];
        if(i < 11) mnemonic += " ";
    }
    return mnemonic;
}

std::string Wallet::deriveAddress(std::string mnemonic) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256((const unsigned char*)mnemonic.c_str(), mnemonic.length(), hash);

    std::stringstream ss;
    // Pegamos os primeiros 20 bytes do hash SHA256 para gerar o endereço
    for(int i = 0; i < 20; i++) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)hash[i];
    }
    return "MZ" + ss.str(); // Endereço estilo Mazechain
}
