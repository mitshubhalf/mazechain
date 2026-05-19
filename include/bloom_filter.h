#ifndef BLOOM_FILTER_H
#define BLOOM_FILTER_H

// MazeChain — Bloom Filter (BIP37) for SPV wallets
// Compact probabilistic filter for transaction/address matching
// Used by light clients to filter relevant transactions from full nodes

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <functional>

namespace BloomFilter {

    constexpr uint32_t MAX_BLOOM_FILTER_SIZE = 36000; // bytes
    constexpr uint32_t MAX_HASH_FUNCS        = 50;
    constexpr double   LN2_SQUARED           = 0.4804530139;
    constexpr double   LN2                   = 0.6931471805;

    // MurmurHash3 (32-bit) — used for Bloom filter
    inline uint32_t murmurHash3(const std::vector<uint8_t>& data, uint32_t seed) {
        uint32_t h1 = seed;
        const uint32_t c1 = 0xcc9e2d51, c2 = 0x1b873593;
        size_t i = 0;
        while (i + 4 <= data.size()) {
            uint32_t k1;
            memcpy(&k1, data.data()+i, 4);
            k1 *= c1; k1 = (k1<<15)|(k1>>17); k1 *= c2; h1 ^= k1;
            h1 = ((h1<<13)|(h1>>19)); h1 = h1*5 + 0xe6546b64;
            i += 4;
        }
        uint32_t k1 = 0;
        switch (data.size() & 3) {
            case 3: k1 ^= data[i+2] << 16; [[fallthrough]];
            case 2: k1 ^= data[i+1] << 8;  [[fallthrough]];
            case 1: k1 ^= data[i]; k1 *= c1; k1 = (k1<<15)|(k1>>17); k1 *= c2; h1 ^= k1;
        }
        h1 ^= (uint32_t)data.size();
        h1 ^= h1 >> 16; h1 *= 0x85ebca6b; h1 ^= h1 >> 13;
        h1 *= 0xc2b2ae35; h1 ^= h1 >> 16;
        return h1;
    }

    class Filter {
    public:
        std::vector<uint8_t> data;
        uint32_t nHashFuncs;
        uint32_t nTweak;

        // Create filter for nElements items with false-positive rate fpRate
        Filter(uint32_t nElements, double fpRate, uint32_t tweak = 0) : nTweak(tweak) {
            size_t nBits = (size_t)(-1.0 / LN2_SQUARED * nElements * std::log(fpRate));
            nBits = std::min(nBits, (size_t)MAX_BLOOM_FILTER_SIZE * 8);
            data.assign((nBits + 7) / 8, 0);
            nHashFuncs = (uint32_t)(data.size() * 8.0 / nElements * LN2);
            nHashFuncs = std::min(nHashFuncs, MAX_HASH_FUNCS);
        }

        Filter() : nHashFuncs(0), nTweak(0) { data.assign(1, 0); }

        void insert(const std::vector<uint8_t>& item) {
            for (uint32_t i = 0; i < nHashFuncs; i++) {
                uint32_t h = murmurHash3(item, i * 0xfba4c795 + nTweak);
                uint32_t bit = h % (uint32_t)(data.size() * 8);
                data[bit / 8] |= (1 << (bit % 8));
            }
        }

        void insert(const std::string& s) {
            insert(std::vector<uint8_t>(s.begin(), s.end()));
        }

        bool contains(const std::vector<uint8_t>& item) const {
            for (uint32_t i = 0; i < nHashFuncs; i++) {
                uint32_t h = murmurHash3(item, i * 0xfba4c795 + nTweak);
                uint32_t bit = h % (uint32_t)(data.size() * 8);
                if (!(data[bit / 8] & (1 << (bit % 8)))) return false;
            }
            return true;
        }

        bool contains(const std::string& s) const {
            return contains(std::vector<uint8_t>(s.begin(), s.end()));
        }

        void clear() { std::fill(data.begin(), data.end(), 0); }
        size_t sizeBytes() const { return data.size(); }
        double falsePositiveRate() const {
            // Estimate current FP rate
            int setBits = 0;
            for (uint8_t b : data) for (int i=0;i<8;i++) if (b & (1<<i)) setBits++;
            double k = nHashFuncs;
            double m = data.size() * 8.0;
            return std::pow(setBits / m, k);
        }

        std::string toHex() const {
            std::string out;
            for (uint8_t b : data) {
                char buf[3];
                snprintf(buf, sizeof(buf), "%02x", b);
                out += buf;
            }
            return out;
        }
    };

    // Standard service flags for BIP37 (BLOOM)
    constexpr uint64_t NODE_BLOOM = (1 << 2);

} // namespace BloomFilter

#endif
