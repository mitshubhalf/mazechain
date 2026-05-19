#ifndef BECH32_H
#define BECH32_H

// MazeChain — Bech32 and Bech32m address encoding/decoding
// Mainnet HRP: "maze"  → maze1q... (P2WPKH), maze1z... (P2WSH), maze1p... (P2TR)
// Testnet HRP: "tmaze" → tmaze1q...
// BIP-0173 (Bech32) for SegWit v0, BIP-0350 (Bech32m) for SegWit v1+

#include <string>
#include <vector>
#include <cstdint>
#include <stdexcept>

namespace Bech32 {

static const std::string CHARSET = "qpzry9x8gf2tvdw0s3jn54khce6mua7l";
static const int8_t CHARSET_REV[128] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    15,-1,10,17,21,20,26,30, 7, 5,-1,-1,-1,-1,-1,-1,
    -1,29,-1,24,13,25, 9, 8,23,-1,18,22,31,27,19,-1,
     1, 0, 3,16,11,28,12,14, 6, 4, 2,-1,-1,-1,-1,-1,
    -1,29,-1,24,13,25, 9, 8,23,-1,18,22,31,27,19,-1,
     1, 0, 3,16,11,28,12,14, 6, 4, 2,-1,-1,-1,-1,-1
};

enum class Encoding { BECH32, BECH32M };
static const uint32_t BECH32M_CONST = 0x2bc830a3;

inline uint32_t polymod(const std::vector<uint8_t>& values) {
    uint32_t c = 1;
    for (uint8_t d : values) {
        uint8_t c0 = c >> 25;
        c = ((c & 0x1ffffff) << 5) ^ d;
        if (c0 & 1)  c ^= 0x3b6a57b2;
        if (c0 & 2)  c ^= 0x26508e6d;
        if (c0 & 4)  c ^= 0x1ea119fa;
        if (c0 & 8)  c ^= 0x3d4233dd;
        if (c0 & 16) c ^= 0x2a1462b3;
    }
    return c;
}

inline std::vector<uint8_t> hrpExpand(const std::string& hrp) {
    std::vector<uint8_t> ret;
    for (char c : hrp) ret.push_back(c >> 5);
    ret.push_back(0);
    for (char c : hrp) ret.push_back(c & 31);
    return ret;
}

inline bool verifyChecksum(const std::string& hrp, const std::vector<uint8_t>& data, Encoding enc) {
    auto exp = hrpExpand(hrp);
    exp.insert(exp.end(), data.begin(), data.end());
    uint32_t expected = (enc == Encoding::BECH32M) ? BECH32M_CONST : 1;
    return polymod(exp) == expected;
}

inline std::vector<uint8_t> createChecksum(const std::string& hrp, const std::vector<uint8_t>& data, Encoding enc) {
    auto values = hrpExpand(hrp);
    values.insert(values.end(), data.begin(), data.end());
    for (int i = 0; i < 6; i++) values.push_back(0);
    uint32_t mod = polymod(values) ^ ((enc == Encoding::BECH32M) ? BECH32M_CONST : 1);
    std::vector<uint8_t> ret(6);
    for (int i = 0; i < 6; i++) ret[i] = (mod >> (5*(5-i))) & 31;
    return ret;
}

// Convert bits: from fromBits-wide to toBits-wide values
inline bool convertBits(const std::vector<uint8_t>& in, int fromBits, int toBits,
                        bool pad, std::vector<uint8_t>& out) {
    int acc = 0, bits = 0;
    const int maxv = (1 << toBits) - 1;
    for (uint8_t v : in) {
        if (v >> fromBits) return false;
        acc = (acc << fromBits) | v;
        bits += fromBits;
        while (bits >= toBits) {
            bits -= toBits;
            out.push_back((acc >> bits) & maxv);
        }
    }
    if (pad) { if (bits) out.push_back((acc << (toBits - bits)) & maxv); }
    else if (bits >= fromBits || ((acc << (toBits - bits)) & maxv)) return false;
    return true;
}

// Encode: hrp + witness version (0-16) + witness program bytes
inline std::string encode(const std::string& hrp, int witver, const std::vector<uint8_t>& witprog) {
    Encoding enc = (witver == 0) ? Encoding::BECH32 : Encoding::BECH32M;
    std::vector<uint8_t> data;
    data.push_back((uint8_t)witver);
    std::vector<uint8_t> conv;
    if (!convertBits(witprog, 8, 5, true, conv)) return "";
    data.insert(data.end(), conv.begin(), conv.end());
    auto checksum = createChecksum(hrp, data, enc);
    std::string result = hrp + "1";
    for (uint8_t d : data) result += CHARSET[d];
    for (uint8_t d : checksum) result += CHARSET[d];
    return result;
}

struct DecodeResult {
    bool valid = false;
    int witver = -1;
    std::vector<uint8_t> witprog;
    std::string error;
};

inline DecodeResult decode(const std::string& str) {
    DecodeResult res;
    bool has_lower = false, has_upper = false;
    for (char c : str) {
        if (c < 33 || c > 126) { res.error = "Invalid char"; return res; }
        if (c >= 'a' && c <= 'z') has_lower = true;
        if (c >= 'A' && c <= 'Z') has_upper = true;
    }
    if (has_lower && has_upper) { res.error = "Mixed case"; return res; }
    std::string s = str;
    for (char& c : s) c = tolower(c);

    size_t pos = s.rfind('1');
    if (pos == std::string::npos || pos < 1 || pos + 7 > s.size()) {
        res.error = "Invalid separator"; return res;
    }
    std::string hrp = s.substr(0, pos);
    std::vector<uint8_t> data;
    for (size_t i = pos+1; i < s.size(); i++) {
        if ((uint8_t)s[i] >= 128 || CHARSET_REV[(uint8_t)s[i]] < 0) {
            res.error = "Invalid data char"; return res;
        }
        data.push_back((uint8_t)CHARSET_REV[(uint8_t)s[i]]);
    }
    if (data.size() < 6) { res.error = "Too short"; return res; }

    std::vector<uint8_t> payload(data.begin(), data.end()-6);
    bool okB   = verifyChecksum(hrp, data, Encoding::BECH32);
    bool okBM  = verifyChecksum(hrp, data, Encoding::BECH32M);
    if (!okB && !okBM) { res.error = "Invalid checksum"; return res; }

    if (payload.empty()) { res.error = "Empty payload"; return res; }
    res.witver = payload[0];
    if (res.witver > 16) { res.error = "Invalid witness version"; return res; }

    std::vector<uint8_t> prog;
    if (!convertBits(std::vector<uint8_t>(payload.begin()+1, payload.end()), 5, 8, false, prog)) {
        res.error = "Bit conversion failed"; return res;
    }
    if (prog.size() < 2 || prog.size() > 40) { res.error = "Invalid program length"; return res; }
    if (res.witver == 0 && prog.size() != 20 && prog.size() != 32) {
        res.error = "Invalid v0 program length"; return res;
    }

    Encoding expected = (res.witver == 0) ? Encoding::BECH32 : Encoding::BECH32M;
    if ((expected == Encoding::BECH32 && !okB) ||
        (expected == Encoding::BECH32M && !okBM)) {
        res.error = "Wrong encoding for version"; return res;
    }

    res.witprog = prog;
    res.valid = true;
    return res;
}

// Convenience: mainnet/testnet HRP
inline std::string mainnetHRP() { return "maze"; }
inline std::string testnetHRP() { return "tmaze"; }

// Encode P2WPKH (20-byte hash)
inline std::string encodeP2WPKH(const std::vector<uint8_t>& hash20, bool testnet = false) {
    return encode(testnet ? testnetHRP() : mainnetHRP(), 0, hash20);
}
// Encode P2WSH (32-byte hash)
inline std::string encodeP2WSH(const std::vector<uint8_t>& hash32, bool testnet = false) {
    return encode(testnet ? testnetHRP() : mainnetHRP(), 0, hash32);
}
// Encode P2TR (32-byte tweaked pubkey)
inline std::string encodeP2TR(const std::vector<uint8_t>& key32, bool testnet = false) {
    return encode(testnet ? testnetHRP() : mainnetHRP(), 1, key32);
}

// Validate any Bech32/Bech32m address
inline bool isValid(const std::string& addr) { return decode(addr).valid; }

} // namespace Bech32

#endif
