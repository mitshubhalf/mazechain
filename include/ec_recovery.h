#ifndef EC_RECOVERY_H
#define EC_RECOVERY_H

// MazeChain — EC Key Recovery & RFC6979 Deterministic Signatures
// Recover public key from ECDSA signature + message
// RFC6979: deterministic k generation (prevents k-reuse attacks)
// RIPEMD160 utility wrapper

#include <string>
#include <vector>
#include <cstdint>
#include <sstream>
#include <iomanip>
#include <openssl/ec.h>
#include <openssl/ecdsa.h>
#include <openssl/bn.h>
#include <openssl/sha.h>
#include <openssl/ripemd.h>
#include <openssl/hmac.h>
#include <openssl/obj_mac.h>

namespace ECRecovery {

    using Bytes = std::vector<uint8_t>;

    inline std::string toHex(const Bytes& v) {
        std::ostringstream ss;
        for (uint8_t b : v) ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        return ss.str();
    }

    inline Bytes fromHex(const std::string& hex) {
        Bytes out;
        for (size_t i = 0; i+1 < hex.size(); i += 2)
            out.push_back((uint8_t)std::stoi(hex.substr(i,2), nullptr, 16));
        return out;
    }

    // ── RIPEMD160 ──────────────────────────────────────────────────────────────
    inline Bytes ripemd160(const Bytes& data) {
        Bytes out(20);
        RIPEMD160(data.data(), data.size(), out.data());
        return out;
    }

    inline std::string ripemd160Hex(const std::string& hexData) {
        Bytes data = fromHex(hexData);
        Bytes h(20);
        RIPEMD160(data.data(), data.size(), h.data());
        return toHex(h);
    }

    // ── SHA256d (double SHA256) ────────────────────────────────────────────────
    inline Bytes sha256d(const Bytes& data) {
        Bytes tmp(32), out(32);
        SHA256(data.data(), data.size(), tmp.data());
        SHA256(tmp.data(), 32, out.data());
        return out;
    }

    // ── Hash160 = RIPEMD160(SHA256(data)) ─────────────────────────────────────
    inline Bytes hash160(const Bytes& data) {
        Bytes sha(32);
        SHA256(data.data(), data.size(), sha.data());
        return ripemd160(sha);
    }

    // ── RFC6979 — Deterministic k generation ──────────────────────────────────
    // Returns 32-byte deterministic k for given privkey + msg_hash
    inline Bytes rfc6979_k(const Bytes& privkey32, const Bytes& msgHash32) {
        // RFC6979 Section 3.2
        Bytes V(32, 0x01), K(32, 0x00);
        Bytes privMsg;
        privMsg.insert(privMsg.end(), privkey32.begin(), privkey32.end());
        privMsg.insert(privMsg.end(), msgHash32.begin(), msgHash32.end());

        // K = HMAC_SHA256(K, V || 0x00 || privkey || msg)
        auto hmac = [](const Bytes& key, const Bytes& msg) -> Bytes {
            Bytes out(32);
            unsigned int len = 32;
            HMAC(EVP_sha256(), key.data(), (int)key.size(),
                 msg.data(), msg.size(), out.data(), &len);
            return out;
        };

        Bytes input;
        input.insert(input.end(), V.begin(), V.end());
        input.push_back(0x00);
        input.insert(input.end(), privMsg.begin(), privMsg.end());
        K = hmac(K, input);
        V = hmac(K, V);

        input.clear();
        input.insert(input.end(), V.begin(), V.end());
        input.push_back(0x01);
        input.insert(input.end(), privMsg.begin(), privMsg.end());
        K = hmac(K, input);
        V = hmac(K, V);
        V = hmac(K, V);
        return V; // use as k
    }

    // ── EC Key Recovery ────────────────────────────────────────────────────────
    // Given DER-encoded signature + message hash, recover candidate public keys
    // Returns empty string if recovery fails
    inline std::string recoverPubkey(const Bytes& derSig, const Bytes& msgHash,
                                      int recoveryId = 0) {
        EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
        BN_CTX*   ctx   = BN_CTX_new();

        const uint8_t* sigData = derSig.data();
        ECDSA_SIG* sig = d2i_ECDSA_SIG(nullptr, &sigData, (long)derSig.size());
        if (!sig) {
            BN_CTX_free(ctx); EC_GROUP_free(group);
            return "";
        }

        const BIGNUM* r; const BIGNUM* s;
        ECDSA_SIG_get0(sig, &r, &s);

        // Get curve order n and prime p
        BIGNUM* n = BN_new(); EC_GROUP_get_order(group, n, ctx);
        BIGNUM* p = BN_new();
        BIGNUM* a = BN_new(); BIGNUM* b = BN_new();
        EC_GROUP_get_curve(group, p, a, b, ctx);

        // x = r + j*n for j in {0,1}
        BIGNUM* x = BN_dup(r);
        if (recoveryId & 2) BN_add(x, x, n);
        if (BN_cmp(x, p) >= 0) {
            BN_free(n); BN_free(p); BN_free(a); BN_free(b); BN_free(x);
            ECDSA_SIG_free(sig); BN_CTX_free(ctx); EC_GROUP_free(group);
            return "";
        }

        // Recover R from x coordinate
        EC_POINT* R = EC_POINT_new(group);
        // y^2 = x^3 + 7 (mod p)
        BIGNUM* y2 = BN_new(); BIGNUM* tmp = BN_new();
        BN_mod_sqr(tmp, x, p, ctx);
        BN_mod_mul(y2, tmp, x, p, ctx);
        BIGNUM* b7 = BN_new(); BN_set_word(b7, 7);
        BN_mod_add(y2, y2, b7, p, ctx);
        BIGNUM* y = BN_new();
        BIGNUM* exp = BN_new(); BIGNUM* p1 = BN_dup(p);
        BN_add_word(p1, 1); BN_rshift(exp, p1, 2);
        BN_mod_exp(y, y2, exp, p, ctx);
        if ((BN_is_odd(y) ? 1 : 0) != (recoveryId & 1)) BN_sub(y, p, y);
        EC_POINT_set_affine_coordinates(group, R, x, y, ctx);

        // Q = r^-1 * (s*R - e*G)
        BIGNUM* rinv = BN_mod_inverse(nullptr, r, n, ctx);
        BIGNUM* e    = BN_bin2bn(msgHash.data(), (int)msgHash.size(), nullptr);
        BIGNUM* sr   = BN_new(); BN_mod_mul(sr, s, rinv, n, ctx);
        BIGNUM* er   = BN_new(); BN_mod_mul(er, e, rinv, n, ctx);
        BN_mod_sub(er, n, er, n, ctx);

        EC_POINT* Q = EC_POINT_new(group);
        EC_POINT_mul(group, Q, er, R, sr, ctx);

        // Serialize uncompressed public key
        std::string result;
        size_t len = EC_POINT_point2oct(group, Q, POINT_CONVERSION_UNCOMPRESSED,
                                         nullptr, 0, ctx);
        if (len > 0) {
            Bytes pubkey(len);
            EC_POINT_point2oct(group, Q, POINT_CONVERSION_UNCOMPRESSED,
                               pubkey.data(), len, ctx);
            result = toHex(pubkey);
        }

        BN_free(n); BN_free(p); BN_free(a); BN_free(b); BN_free(x);
        BN_free(y2); BN_free(y); BN_free(tmp); BN_free(b7);
        BN_free(exp); BN_free(p1); BN_free(rinv); BN_free(e);
        BN_free(sr); BN_free(er);
        EC_POINT_free(R); EC_POINT_free(Q);
        ECDSA_SIG_free(sig);
        BN_CTX_free(ctx); EC_GROUP_free(group);
        return result;
    }

    // ── Verify ECDSA (DER sig, uncompressed/compressed pubkey, hash) ───────────
    inline bool verifySig(const std::string& pubkeyHex, const std::string& msgHashHex,
                          const std::string& sigHex) {
        Bytes pubBytes = fromHex(pubkeyHex);
        Bytes sigBytes = fromHex(sigHex);
        Bytes hashBytes= fromHex(msgHashHex);

        EC_GROUP* group = EC_GROUP_new_by_curve_name(NID_secp256k1);
        BN_CTX*   ctx   = BN_CTX_new();

        EC_POINT* pubpoint = EC_POINT_new(group);
        if (!EC_POINT_oct2point(group, pubpoint, pubBytes.data(), pubBytes.size(), ctx)) {
            EC_POINT_free(pubpoint); BN_CTX_free(ctx); EC_GROUP_free(group);
            return false;
        }

        EC_KEY* key = EC_KEY_new();
        EC_KEY_set_group(key, group);
        EC_KEY_set_public_key(key, pubpoint);

        const uint8_t* sd = sigBytes.data();
        ECDSA_SIG* sig = d2i_ECDSA_SIG(nullptr, &sd, (long)sigBytes.size());
        bool valid = false;
        if (sig) {
            valid = ECDSA_do_verify(hashBytes.data(), (int)hashBytes.size(), sig, key) == 1;
            ECDSA_SIG_free(sig);
        }

        EC_KEY_free(key);
        EC_POINT_free(pubpoint);
        BN_CTX_free(ctx);
        EC_GROUP_free(group);
        return valid;
    }

} // namespace ECRecovery

#endif
