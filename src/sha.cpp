#include "crypto/sha256.h"
#include <algorithm>
#include <cstring>
#include <string>

// --- UTILITÁRIOS DE ENDIANNESS ---
inline uint32_t ReadBE32(const unsigned char* ptr) {
    uint32_t x;
    memcpy(&x, ptr, 4);
#if defined(__linux__) || defined(__FreeBSD__)
    return __builtin_bswap32(x);
#else
    return ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | ((x >> 24) & 0xFF);
#endif
}

inline void WriteBE32(unsigned char* ptr, uint32_t x) {
#if defined(__linux__) || defined(__FreeBSD__)
    uint32_t v = __builtin_bswap32(x);
#else
    uint32_t v = ((x & 0xFF) << 24) | ((x & 0xFF00) << 8) | ((x >> 8) & 0xFF00) | ((x >> 24) & 0xFF);
#endif
    memcpy(ptr, &v, 4);
}

// --- IMPLEMENTAÇÃO DA CLASSE CSHA256 ---

CSHA256::CSHA256() { Reset(); }

CSHA256& CSHA256::Reset() {
    s[0] = 0x6a09e667ul; s[1] = 0xbb67ae85ul; s[2] = 0x3c6ef372ul; s[3] = 0xa54ff53aul;
    s[4] = 0x510e527ful; s[5] = 0x9b05688cul; s[6] = 0x1f83d9abul; s[7] = 0x5be0cd19ul;
    bytes = 0;
    return *this;
}

namespace {
    inline uint32_t Ch(uint32_t x, uint32_t y, uint32_t z) { return z ^ (x & (y ^ z)); }
    inline uint32_t Maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) | (z & (x | y)); }
    inline uint32_t Sigma0(uint32_t x) { return (x >> 2 | x << 30) ^ (x >> 13 | x << 19) ^ (x >> 22 | x << 10); }
    inline uint32_t Sigma1(uint32_t x) { return (x >> 6 | x << 26) ^ (x >> 11 | x << 21) ^ (x >> 25 | x << 7); }
    inline uint32_t sigma0(uint32_t x) { return (x >> 7 | x << 25) ^ (x >> 18 | x << 14) ^ (x >> 3); }
    inline uint32_t sigma1(uint32_t x) { return (x >> 17 | x << 15) ^ (x >> 19 | x << 13) ^ (x >> 10); }

    const uint32_t K[64] = {
        0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
        0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
        0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
        0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
        0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
        0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
        0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
        0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
    };

    inline void Round(uint32_t a, uint32_t b, uint32_t c, uint32_t& d, uint32_t e, uint32_t f, uint32_t g, uint32_t& h, uint32_t k) {
        uint32_t t1 = h + Sigma1(e) + Ch(e, f, g) + k;
        uint32_t t2 = Sigma0(a) + Maj(a, b, c);
        d += t1;
        h = t1 + t2;
    }
}

void Transform(uint32_t* s, const unsigned char* chunk, size_t blocks) {
    while (blocks--) {
        uint32_t a = s[0], b = s[1], c = s[2], d = s[3], e = s[4], f = s[5], g = s[6], h = s[7];
        uint32_t w[64];

        for (int i = 0; i < 16; i++) w[i] = ReadBE32(chunk + (i * 4));
        for (int i = 16; i < 64; i++) w[i] = sigma1(w[i - 2]) + w[i - 7] + sigma0(w[i - 15]) + w[i - 16];

        for (int i = 0; i < 64; i += 8) {
            Round(a, b, c, d, e, f, g, h, K[i + 0] + w[i + 0]);
            Round(h, a, b, c, d, e, f, g, K[i + 1] + w[i + 1]);
            Round(g, h, a, b, c, d, e, f, K[i + 2] + w[i + 2]);
            Round(f, g, h, a, b, c, d, e, K[i + 3] + w[i + 3]);
            Round(e, f, g, h, a, b, c, d, K[i + 4] + w[i + 4]);
            Round(d, e, f, g, h, a, b, c, K[i + 5] + w[i + 5]);
            Round(c, d, e, f, g, h, a, b, K[i + 6] + w[i + 6]);
            Round(b, c, d, e, f, g, h, a, K[i + 7] + w[i + 7]);
        }

        s[0] += a; s[1] += b; s[2] += c; s[3] += d;
        s[4] += e; s[5] += f; s[6] += g; s[7] += h;
        chunk += 64;
    }
}

CSHA256& CSHA256::Write(const unsigned char* data, size_t len) {
    const unsigned char* end = data + len;
    size_t buf_fill = bytes % 64;
    bytes += len;

    if (buf_fill + len >= 64) {
        if (buf_fill) {
            memcpy(buf + buf_fill, data, 64 - buf_fill);
            Transform(s, buf, 1);
            data += (64 - buf_fill);
        }
        size_t blocks = (end - data) / 64;
        Transform(s, data, blocks);
        data += blocks * 64;
        buf_fill = 0;
    }
    memcpy(buf + buf_fill, data, end - data);
    return *this;
}

void CSHA256::Finalize(unsigned char hash[32]) {
    static const unsigned char pad[64] = {0x80};
    unsigned char sizedata[8];
    WriteBE32(sizedata, bytes >> 29);
    WriteBE32(sizedata + 4, bytes << 3);
    Write(pad, 1 + ((119 - (bytes % 64)) % 64));
    Write(sizedata, 8);
    for (int i = 0; i < 8; i++) WriteBE32(hash + i * 4, s[i]);
}

std::string SHA256AutoDetect(sha256_implementation::UseImplementation use) {
    return "standard (MazeChain Optimized)";
}

void SHA256D64(unsigned char* output, const unsigned char* input, size_t blocks) {
    while (blocks--) {
        CSHA256().Write(input, 64).Finalize(output);
        CSHA256().Write(output, 32).Finalize(output);
        output += 32;
        input += 64;
    }
}