#include "../../include/compressor.h"
#include <vector>
#include <cstdint>

// --- Lógica de Compressão de Valores (Amounts) ---
// O Bitcoin comprime valores para economizar espaço em disco/rede.
// Valores pequenos usam menos bytes no VARINT.

uint64_t CompressAmount(uint64_t n) {
    if (n == 0) return 0;
    uint64_t e = 0;
    while (((n % 10) == 0) && e < 9) {
        n /= 10;
        e++;
    }
    if (e < 9) {
        uint64_t d = n % 10;
        assert(d >= 1 && d <= 9);
        n /= 10;
        return 1 + (n * 9 + d - 1) * 10 + e;
    } else {
        return 1 + (n - 1) * 10 + 9;
    }
}

uint64_t DecompressAmount(uint64_t x) {
    if (x == 0) return 0;
    x--;
    uint64_t e = x % 10;
    x /= 10;
    uint64_t n = 0;
    if (e < 9) {
        uint64_t d = (x % 9) + 1;
        x /= 9;
        n = x * 10 + d;
    } else {
        n = x + 1;
    }
    while (e > 0) {
        n *= 10;
        e--;
    }
    return n;
}

// --- Lógica de Compressão de Scripts ---
// Aqui simplificamos para a MazeChain. Se o script for um padrão P2PKH ou P2SH,
// ele ocupa apenas 21 bytes em vez de 25+.

bool CompressScript(const std::vector<uint8_t>& script, std::vector<uint8_t>& out) {
    // Padrão P2PKH: OP_DUP OP_HASH160 <20-byte-hash> OP_EQUALVERIFY OP_CHECKSIG
    if (script.size() == 25 && script[0] == 0x76 && script[1] == 0xa9 && 
        script[2] == 0x14 && script[23] == 0x88 && script[24] == 0xac) {
        out.resize(21);
        out[0] = 0x00; // Marcador de P2PKH
        memcpy(&out[1], &script[3], 20);
        return true;
    }

    // Padrão P2SH: OP_HASH160 <20-byte-hash> OP_EQUAL
    if (script.size() == 23 && script[0] == 0xa9 && script[1] == 0x14 && script[22] == 0x87) {
        out.resize(21);
        out[0] = 0x01; // Marcador de P2SH
        memcpy(&out[1], &script[2], 20);
        return true;
    }

    return false; // Não é um script padrão compressível
}

bool DecompressScript(std::vector<uint8_t>& script, unsigned int nSize, const std::vector<uint8_t>& in) {
    if (nSize == 0x00) { // P2PKH
        script.resize(25);
        script[0] = 0x76; script[1] = 0xa9; script[2] = 0x14;
        memcpy(&script[3], &in[0], 20);
        script[23] = 0x88; script[24] = 0xac;
        return true;
    }
    if (nSize == 0x01) { // P2SH
        script.resize(23);
        script[0] = 0xa9; script[1] = 0x14;
        memcpy(&script[2], &in[0], 20);
        script[22] = 0x87;
        return true;
    }
    return false;
}