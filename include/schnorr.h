#ifndef SCHNORR_H
#define SCHNORR_H

// MazeChain — Schnorr Signatures (BIP-340) & Taproot (BIP-341/342)
// Schnorr: deterministic, batch-verifiable, key aggregation
// Taproot: key-path + script-path spending, Merkle script trees
// Miniscript: policy language for complex spending conditions

#include <string>
#include <vector>
#include <cstdint>
#include <openssl/sha.h>
#include <openssl/bn.h>
#include <openssl/ec.h>
#include <openssl/obj_mac.h>
#include <sstream>
#include <iomanip>
#include <stdexcept>

namespace Schnorr {

    using Bytes = std::vector<uint8_t>;

    // secp256k1 field prime p
    static const char* SECP256K1_P_HEX =
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEFFFFFC2F";
    static const char* SECP256K1_N_HEX =
        "FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFFEBAAEDCE6AF48A03BBFD25E8CD0364141";

    inline Bytes sha256(const Bytes& data) {
        Bytes out(32);
        SHA256(data.data(), data.size(), out.data());
        return out;
    }

    inline Bytes sha256(const std::string& s) {
        return sha256(Bytes(s.begin(), s.end()));
    }

    // Tagged hash (BIP-340): SHA256(SHA256(tag) || SHA256(tag) || msg)
    inline Bytes taggedHash(const std::string& tag, const Bytes& msg) {
        Bytes tagHash = sha256(tag);
        Bytes preimage;
        preimage.insert(preimage.end(), tagHash.begin(), tagHash.end());
        preimage.insert(preimage.end(), tagHash.begin(), tagHash.end());
        preimage.insert(preimage.end(), msg.begin(), msg.end());
        return sha256(preimage);
    }

    inline Bytes fromHex(const std::string& hex) {
        Bytes out;
        for (size_t i = 0; i+1 < hex.size(); i += 2)
            out.push_back((uint8_t)std::stoi(hex.substr(i,2), nullptr, 16));
        return out;
    }

    inline std::string toHex(const Bytes& v) {
        std::ostringstream ss;
        for (uint8_t b : v) ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        return ss.str();
    }

    // ── Schnorr Key Pair ───────────────────────────────────────────────────────
    struct KeyPair {
        Bytes privkey; // 32 bytes
        Bytes pubkey;  // 32 bytes (x-only, BIP-340)
    };

    // Generate Schnorr keypair from 32-byte private key
    inline KeyPair makeKeyPair(const Bytes& privkey32) {
        if (privkey32.size() != 32) throw std::invalid_argument("privkey must be 32 bytes");
        KeyPair kp;
        kp.privkey = privkey32;

        EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
        EC_POINT* point = EC_POINT_new(group);
        BIGNUM*   priv  = BN_bin2bn(privkey32.data(), 32, nullptr);
        BN_CTX*   ctx   = BN_CTX_new();

        EC_POINT_mul(group, point, priv, nullptr, nullptr, ctx);

        BIGNUM* x = BN_new();
        BIGNUM* y = BN_new();
        EC_POINT_get_affine_coordinates(group, point, x, y, ctx);

        // BIP-340: if y is odd, negate the private key
        if (BN_is_odd(y)) {
            BIGNUM* n = BN_new();
            BN_hex2bn(&n, SECP256K1_N_HEX);
            BN_sub(priv, n, priv);
            BN_free(n);
        }

        // x-only pubkey (32 bytes)
        kp.pubkey.resize(32);
        BN_bn2binpad(x, kp.pubkey.data(), 32);

        BN_free(priv); BN_free(x); BN_free(y);
        BN_CTX_free(ctx);
        EC_POINT_free(point);
        EC_GROUP_free(group);
        return kp;
    }

    // ── Schnorr Sign (BIP-340) ─────────────────────────────────────────────────
    // Returns 64-byte signature (r || s)
    inline Bytes sign(const Bytes& privkey32, const Bytes& msg32) {
        if (msg32.size() != 32) throw std::invalid_argument("msg must be 32 bytes");

        Bytes aux(32, 0); // deterministic: use zeros for aux_rand in test
        // BIP-340: t = privkey XOR taggedHash("BIP0340/aux", aux_rand)
        Bytes auxHash = taggedHash("BIP0340/aux", aux);
        Bytes t(32);
        for (int i = 0; i < 32; i++) t[i] = privkey32[i] ^ auxHash[i];

        // rand = taggedHash("BIP0340/nonce", t || pubkey || msg)
        KeyPair kp = makeKeyPair(privkey32);
        Bytes nonceInput;
        nonceInput.insert(nonceInput.end(), t.begin(), t.end());
        nonceInput.insert(nonceInput.end(), kp.pubkey.begin(), kp.pubkey.end());
        nonceInput.insert(nonceInput.end(), msg32.begin(), msg32.end());
        Bytes kBytes = taggedHash("BIP0340/nonce", nonceInput);

        // k = int(k') mod n; if odd, k = n - k
        BIGNUM* k   = BN_bin2bn(kBytes.data(), 32, nullptr);
        BIGNUM* n   = BN_new(); BN_hex2bn(&n, SECP256K1_N_HEX);
        BN_mod(k, k, n, BN_CTX_new());

        EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
        EC_POINT* R     = EC_POINT_new(group);
        BN_CTX*   ctx   = BN_CTX_new();
        EC_POINT_mul(group, R, k, nullptr, nullptr, ctx);

        BIGNUM* rx = BN_new(); BIGNUM* ry = BN_new();
        EC_POINT_get_affine_coordinates(group, R, rx, ry, ctx);
        if (BN_is_odd(ry)) { BN_sub(k, n, k); }

        // e = taggedHash("BIP0340/challenge", r_bytes || pubkey || msg)
        Bytes rBytes(32); BN_bn2binpad(rx, rBytes.data(), 32);
        Bytes challengeInput;
        challengeInput.insert(challengeInput.end(), rBytes.begin(), rBytes.end());
        challengeInput.insert(challengeInput.end(), kp.pubkey.begin(), kp.pubkey.end());
        challengeInput.insert(challengeInput.end(), msg32.begin(), msg32.end());
        Bytes eBytes = taggedHash("BIP0340/challenge", challengeInput);
        BIGNUM* e = BN_bin2bn(eBytes.data(), 32, nullptr);

        // s = (k + e*privkey) mod n
        BIGNUM* priv = BN_bin2bn(privkey32.data(), 32, nullptr);
        BIGNUM* s    = BN_new();
        BN_mod_mul(s, e, priv, n, ctx);
        BN_mod_add(s, k, s, n, ctx);

        // sig = r(32) || s(32)
        Bytes sig(64);
        BN_bn2binpad(rx, sig.data(),    32);
        BN_bn2binpad(s,  sig.data()+32, 32);

        BN_free(k); BN_free(n); BN_free(rx); BN_free(ry);
        BN_free(e); BN_free(priv); BN_free(s);
        BN_CTX_free(ctx);
        EC_POINT_free(R); EC_GROUP_free(group);
        return sig;
    }

    // ── Schnorr Verify (BIP-340) ───────────────────────────────────────────────
    inline bool verify(const Bytes& pubkey32, const Bytes& msg32, const Bytes& sig64) {
        if (pubkey32.size() != 32 || msg32.size() != 32 || sig64.size() != 64) return false;

        Bytes rBytes(sig64.begin(), sig64.begin()+32);
        Bytes sBytes(sig64.begin()+32, sig64.end());

        EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
        BN_CTX*   ctx   = BN_CTX_new();
        BIGNUM*   p     = BN_new(); BN_hex2bn(&p, SECP256K1_P_HEX);
        BIGNUM*   n     = BN_new(); BN_hex2bn(&n, SECP256K1_N_HEX);

        BIGNUM* r = BN_bin2bn(rBytes.data(), 32, nullptr);
        BIGNUM* s = BN_bin2bn(sBytes.data(), 32, nullptr);
        if (BN_cmp(r, p) >= 0 || BN_cmp(s, n) >= 0) {
            BN_free(r); BN_free(s); BN_free(p); BN_free(n);
            BN_CTX_free(ctx); EC_GROUP_free(group); return false;
        }

        // e = taggedHash("BIP0340/challenge", r || pubkey || msg)
        Bytes challengeInput;
        challengeInput.insert(challengeInput.end(), rBytes.begin(), rBytes.end());
        challengeInput.insert(challengeInput.end(), pubkey32.begin(), pubkey32.end());
        challengeInput.insert(challengeInput.end(), msg32.begin(), msg32.end());
        Bytes eBytes = taggedHash("BIP0340/challenge", challengeInput);
        BIGNUM* e = BN_bin2bn(eBytes.data(), 32, nullptr);

        // Lift pubkey x to curve point P
        // (simplified: use x-coordinate to recover point)
        EC_POINT* P = EC_POINT_new(group);
        BIGNUM*   px= BN_bin2bn(pubkey32.data(), 32, nullptr);
        // Compute y^2 = x^3 + 7 mod p
        BIGNUM* y2 = BN_new(); BIGNUM* tmp = BN_new();
        BN_mod_sqr(tmp, px, p, ctx);
        BN_mod_mul(y2, tmp, px, p, ctx);
        BIGNUM* seven = BN_new(); BN_set_word(seven, 7);
        BN_mod_add(y2, y2, seven, p, ctx);
        BIGNUM* y = BN_new();
        // y = y2^((p+1)/4) mod p (works since p ≡ 3 mod 4)
        BIGNUM* exp = BN_new();
        BN_add_word(exp, 1); // p+1
        BIGNUM* p_copy = BN_dup(p);
        BN_add_word(p_copy, 1);
        BN_rshift(exp, p_copy, 2); // (p+1)/4
        BN_mod_exp(y, y2, exp, p, ctx);
        if (BN_is_odd(y)) { BN_sub(y, p, y); } // ensure even y

        EC_POINT_set_affine_coordinates(group, P, px, y, ctx);

        // R = s*G - e*P
        EC_POINT* sG = EC_POINT_new(group);
        EC_POINT* eP = EC_POINT_new(group);
        EC_POINT* R  = EC_POINT_new(group);
        EC_POINT_mul(group, sG, s, nullptr, nullptr, ctx);
        EC_POINT_mul(group, eP, nullptr, P, e, ctx);
        EC_POINT_invert(group, eP, ctx);
        EC_POINT_add(group, R, sG, eP, ctx);

        if (EC_POINT_is_at_infinity(group, R)) {
            BN_free(r); BN_free(s); BN_free(e); BN_free(p); BN_free(n);
            BN_free(px); BN_free(y); BN_free(y2); BN_free(tmp);
            BN_free(seven); BN_free(exp); BN_free(p_copy);
            BN_CTX_free(ctx);
            EC_POINT_free(P); EC_POINT_free(sG); EC_POINT_free(eP); EC_POINT_free(R);
            EC_GROUP_free(group); return false;
        }

        BIGNUM* rx = BN_new(); BIGNUM* ry = BN_new();
        EC_POINT_get_affine_coordinates(group, R, rx, ry, ctx);
        bool valid = !BN_is_odd(ry) && (BN_cmp(rx, r) == 0);

        BN_free(r); BN_free(s); BN_free(e); BN_free(p); BN_free(n);
        BN_free(px); BN_free(y); BN_free(y2); BN_free(tmp);
        BN_free(seven); BN_free(exp); BN_free(p_copy); BN_free(rx); BN_free(ry);
        BN_CTX_free(ctx);
        EC_POINT_free(P); EC_POINT_free(sG); EC_POINT_free(eP); EC_POINT_free(R);
        EC_GROUP_free(group);
        return valid;
    }

    // ── Taproot Key Tweak (BIP-341) ────────────────────────────────────────────
    // Tweak an internal pubkey P with a Merkle root t
    // Q = P + hash_TapTweak(P || t) * G
    inline Bytes taprootTweak(const Bytes& internalPubkey32, const Bytes& merkleRoot) {
        Bytes tweakInput;
        tweakInput.insert(tweakInput.end(), internalPubkey32.begin(), internalPubkey32.end());
        tweakInput.insert(tweakInput.end(), merkleRoot.begin(), merkleRoot.end());
        Bytes t = taggedHash("TapTweak", tweakInput);

        EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
        BN_CTX*   ctx   = BN_CTX_new();
        BIGNUM*   p     = BN_new(); BN_hex2bn(&p, SECP256K1_P_HEX);

        // Lift internal key
        BIGNUM* px = BN_bin2bn(internalPubkey32.data(), 32, nullptr);
        BIGNUM* y2 = BN_new(); BIGNUM* tmp = BN_new();
        BN_mod_sqr(tmp, px, p, ctx);
        BN_mod_mul(y2, tmp, px, p, ctx);
        BIGNUM* seven = BN_new(); BN_set_word(seven, 7);
        BN_mod_add(y2, y2, seven, p, ctx);
        BIGNUM* y = BN_new();
        BIGNUM* exp = BN_new(); BIGNUM* p1 = BN_dup(p); BN_add_word(p1,1); BN_rshift(exp,p1,2);
        BN_mod_exp(y, y2, exp, p, ctx);
        if (BN_is_odd(y)) BN_sub(y, p, y);

        EC_POINT* P = EC_POINT_new(group);
        EC_POINT_set_affine_coordinates(group, P, px, y, ctx);

        // t*G
        BIGNUM* tv = BN_bin2bn(t.data(), 32, nullptr);
        EC_POINT* tG = EC_POINT_new(group);
        EC_POINT_mul(group, tG, tv, nullptr, nullptr, ctx);

        // Q = P + t*G
        EC_POINT* Q = EC_POINT_new(group);
        EC_POINT_add(group, Q, P, tG, ctx);

        BIGNUM* qx = BN_new(); BIGNUM* qy = BN_new();
        EC_POINT_get_affine_coordinates(group, Q, qx, qy, ctx);
        Bytes result(32);
        BN_bn2binpad(qx, result.data(), 32);

        BN_free(px); BN_free(y); BN_free(y2); BN_free(tmp); BN_free(seven);
        BN_free(exp); BN_free(p1); BN_free(tv); BN_free(qx); BN_free(qy); BN_free(p);
        BN_CTX_free(ctx);
        EC_POINT_free(P); EC_POINT_free(tG); EC_POINT_free(Q); EC_GROUP_free(group);
        return result;
    }

    // ── Miniscript policies (simplified) ─────────────────────────────────────
    enum class PolicyType { PK, PKH, OLDER, AFTER, SHA256, AND, OR, THRESH };

    struct Policy {
        PolicyType type;
        std::string param;
        std::vector<Policy> children;
        int k = 0; // for THRESH

        std::string describe() const {
            switch (type) {
                case PolicyType::PK:    return "pk(" + param + ")";
                case PolicyType::PKH:   return "pkh(" + param + ")";
                case PolicyType::OLDER: return "older(" + param + ")";
                case PolicyType::AFTER: return "after(" + param + ")";
                case PolicyType::SHA256:return "sha256(" + param + ")";
                case PolicyType::AND:   {
                    std::string s = "and(";
                    for (size_t i=0; i<children.size(); i++) {
                        if (i) s += ","; s += children[i].describe();
                    }
                    return s + ")";
                }
                case PolicyType::OR: {
                    std::string s = "or(";
                    for (size_t i=0; i<children.size(); i++) {
                        if (i) s += ","; s += children[i].describe();
                    }
                    return s + ")";
                }
                case PolicyType::THRESH: {
                    std::string s = "thresh(" + std::to_string(k);
                    for (const auto& c : children) s += "," + c.describe();
                    return s + ")";
                }
            }
            return "unknown";
        }
    };

    inline Policy pk(const std::string& key)   { return {PolicyType::PK,    key}; }
    inline Policy pkh(const std::string& key)  { return {PolicyType::PKH,   key}; }
    inline Policy older(int n)  { return {PolicyType::OLDER, std::to_string(n)}; }
    inline Policy after(int n)  { return {PolicyType::AFTER, std::to_string(n)}; }
    inline Policy andPolicy(std::vector<Policy> c) { return {PolicyType::AND, "", c}; }
    inline Policy orPolicy(std::vector<Policy> c)  { return {PolicyType::OR,  "", c}; }
    inline Policy thresh(int k, std::vector<Policy> c) {
        Policy p; p.type = PolicyType::THRESH; p.k = k; p.children = c; return p;
    }

} // namespace Schnorr

#endif
