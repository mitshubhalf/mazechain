#ifndef MERKLE_PROOF_H
#define MERKLE_PROOF_H

// MazeChain — Merkle Proofs completas (Bitcoin-style SHA256d)
// Suporta: Merkle root calculation, Merkle proof generation & verification
// SPV support: verify tx inclusion without full chain

#include <vector>
#include <string>
#include <stdexcept>
#include <openssl/sha.h>
#include <sstream>
#include <iomanip>

namespace Merkle {

    using Hash = std::vector<uint8_t>;

    inline Hash sha256(const Hash& data) {
        Hash out(32);
        SHA256(data.data(), data.size(), out.data());
        return out;
    }

    inline Hash hash256(const Hash& data) {
        return sha256(sha256(data));
    }

    inline Hash fromHex(const std::string& hex) {
        Hash out;
        for (size_t i = 0; i + 1 < hex.size(); i += 2)
            out.push_back((uint8_t)std::stoi(hex.substr(i,2), nullptr, 16));
        return out;
    }

    inline std::string toHex(const Hash& h) {
        std::ostringstream ss;
        for (uint8_t b : h) ss << std::hex << std::setw(2) << std::setfill('0') << (int)b;
        return ss.str();
    }

    inline Hash combine(const Hash& l, const Hash& r) {
        Hash combined;
        combined.insert(combined.end(), l.begin(), l.end());
        combined.insert(combined.end(), r.begin(), r.end());
        return hash256(combined);
    }

    // Calculate Merkle root from list of tx hashes (Bitcoin style: SHA256d)
    inline std::string calcMerkleRoot(const std::vector<std::string>& txids) {
        if (txids.empty()) return std::string(64, '0');

        std::vector<Hash> layer;
        for (const auto& txid : txids) layer.push_back(fromHex(txid));

        while (layer.size() > 1) {
            if (layer.size() % 2 != 0) layer.push_back(layer.back()); // duplicate last
            std::vector<Hash> next;
            for (size_t i = 0; i < layer.size(); i += 2)
                next.push_back(combine(layer[i], layer[i+1]));
            layer = next;
        }
        return toHex(layer[0]);
    }

    // Merkle proof node: hash + side (false=left, true=right)
    struct ProofNode {
        std::string hash;
        bool        isRight; // true if this node is on the right side
    };

    struct MerkleProof {
        std::string txid;
        std::string merkleRoot;
        int         txIndex;
        std::vector<ProofNode> path;

        bool isValid() const {
            Hash current = fromHex(txid);
            for (const auto& node : path) {
                Hash sibling = fromHex(node.hash);
                current = node.isRight
                    ? combine(current, sibling)
                    : combine(sibling, current);
            }
            return toHex(current) == merkleRoot;
        }

        std::string serialize() const {
            std::ostringstream ss;
            ss << txid << "|" << merkleRoot << "|" << txIndex;
            for (const auto& n : path)
                ss << "|" << n.hash << ":" << (n.isRight ? "R" : "L");
            return ss.str();
        }
    };

    // Generate Merkle proof for a specific tx at index
    inline MerkleProof generateProof(const std::vector<std::string>& txids, int txIdx) {
        MerkleProof proof;
        if (txIdx < 0 || txIdx >= (int)txids.size())
            throw std::out_of_range("txIdx out of range");

        proof.txid       = txids[txIdx];
        proof.txIndex    = txIdx;
        proof.merkleRoot = calcMerkleRoot(txids);

        std::vector<Hash> layer;
        for (const auto& txid : txids) layer.push_back(fromHex(txid));
        int idx = txIdx;

        while (layer.size() > 1) {
            if (layer.size() % 2 != 0) layer.push_back(layer.back());
            int siblingIdx = (idx % 2 == 0) ? idx + 1 : idx - 1;
            proof.path.push_back({toHex(layer[siblingIdx]), (idx % 2 == 0)});
            std::vector<Hash> next;
            for (size_t i = 0; i < layer.size(); i += 2)
                next.push_back(combine(layer[i], layer[i+1]));
            layer = next;
            idx /= 2;
        }
        return proof;
    }

    // SPV verification: verify tx is in block given only merkle proof
    inline bool spvVerify(const std::string& txid, const std::string& merkleRoot,
                          const std::vector<ProofNode>& path) {
        MerkleProof p;
        p.txid = txid; p.merkleRoot = merkleRoot; p.path = path;
        return p.isValid();
    }

} // namespace Merkle

#endif
