#ifndef LOCKTIME_H
#define LOCKTIME_H

// MazeChain — Locktime, Sequence, CSV, CLTV, RBF
// nLockTime: absolute locktime (block height or timestamp)
// nSequence: relative locktime / RBF signaling
// CSV  (BIP 68/112): OP_CHECKSEQUENCEVERIFY
// CLTV (BIP 65):     OP_CHECKLOCKTIMEVERIFY

#include <cstdint>
#include <ctime>
#include <string>

namespace Locktime {

    // Threshold: values >= this are UNIX timestamps; below = block heights
    constexpr uint32_t LOCKTIME_THRESHOLD = 500000000;
    // Sequence flag for CSV
    constexpr uint32_t SEQUENCE_LOCKTIME_DISABLE_FLAG = 1u << 31;
    constexpr uint32_t SEQUENCE_LOCKTIME_TYPE_FLAG    = 1u << 22;
    constexpr uint32_t SEQUENCE_LOCKTIME_MASK         = 0x0000ffff;
    // RBF: sequence < 0xfffffffe signals RBF opt-in
    constexpr uint32_t SEQUENCE_FINAL    = 0xffffffff;
    constexpr uint32_t SEQUENCE_RBF_MAX  = 0xfffffffd;

    // Returns true if tx is final given current block height and time
    inline bool isFinal(uint32_t nLockTime, uint32_t nSequence,
                        int blockHeight, int64_t blockTime) {
        if (nLockTime == 0) return true;
        if (nSequence == SEQUENCE_FINAL) return true;

        int64_t limit = (nLockTime < LOCKTIME_THRESHOLD)
                        ? (int64_t)blockHeight
                        : blockTime;
        return (int64_t)nLockTime < limit;
    }

    // Returns true if this transaction signals RBF (Replace-by-Fee)
    inline bool signalsRBF(uint32_t nSequence) {
        return nSequence <= SEQUENCE_RBF_MAX;
    }

    // Evaluate CSV (relative locktime via nSequence)
    // Returns true if the input's relative locktime is satisfied
    inline bool checkCSV(uint32_t nSequence, int txDepth,
                         int64_t txTime, int currentHeight, int64_t currentTime) {
        if (nSequence & SEQUENCE_LOCKTIME_DISABLE_FLAG) return true;
        if (nSequence & SEQUENCE_LOCKTIME_TYPE_FLAG) {
            // Time-based: in 512-second units
            int64_t required = (nSequence & SEQUENCE_LOCKTIME_MASK) * 512;
            return (currentTime - txTime) >= required;
        } else {
            // Block-height based
            int required = (int)(nSequence & SEQUENCE_LOCKTIME_MASK);
            return (currentHeight - txDepth) >= required;
        }
    }

    // Evaluate CLTV (absolute locktime on script stack)
    inline bool checkCLTV(int64_t stackLocktime, uint32_t nLockTime,
                          int blockHeight, int64_t blockTime) {
        if (stackLocktime < 0) return false;
        // Both must be same type (height or time)
        bool stackIsTime = (stackLocktime >= LOCKTIME_THRESHOLD);
        bool txIsTime    = (nLockTime    >= LOCKTIME_THRESHOLD);
        if (stackIsTime != txIsTime) return false;
        if (stackIsTime) return blockTime >= stackLocktime;
        return blockHeight >= (int)stackLocktime;
    }

    // Coinbase maturity: coinbase outputs spendable after N blocks
    constexpr int COINBASE_MATURITY = 100;

    inline bool isCoinbaseMature(int coinbaseHeight, int currentHeight) {
        return (currentHeight - coinbaseHeight) >= COINBASE_MATURITY;
    }

    // Dust threshold: outputs below this amount are considered dust
    // Based on 3 * minRelayFee for a typical P2PKH input
    constexpr double DUST_RELAY_FEE_RATE = 0.00003; // MZ per byte
    constexpr int    P2PKH_INPUT_SIZE    = 148;       // bytes

    inline bool isDust(double amount, double feeRatePerByte = DUST_RELAY_FEE_RATE) {
        return amount < (feeRatePerByte * P2PKH_INPUT_SIZE);
    }

    // Sequence helpers
    inline uint32_t makeCSVBlocks(int blocks) {
        return (uint32_t)(blocks & SEQUENCE_LOCKTIME_MASK);
    }
    inline uint32_t makeCSVTime(int seconds) {
        return SEQUENCE_LOCKTIME_TYPE_FLAG | (uint32_t)((seconds / 512) & SEQUENCE_LOCKTIME_MASK);
    }
    inline uint32_t makeCLTVHeight(int height) { return (uint32_t)height; }
    inline uint32_t makeCLTVTime(int64_t ts)   { return (uint32_t)ts; }

    // Transaction version for RBF
    constexpr int TX_VERSION_RBF = 2;

} // namespace Locktime

#endif
