#ifndef FEE_ESTIMATION_H
#define FEE_ESTIMATION_H

// MazeChain — Fee Estimation, Auto Selection, Fee Market, CPFP, RBF
// Fee estimation based on recent block history and mempool depth

#include <vector>
#include <map>
#include <string>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <mutex>

namespace FeeEstimation {

    // Fee rate in MZ per byte
    struct FeeRate {
        double fast     = 0.0001;   // confirms in ~1 block
        double normal   = 0.00005;  // confirms in ~3 blocks
        double economy  = 0.00001;  // confirms in ~6 blocks
        double minimum  = 0.000003; // min relay fee
    };

    // Transaction size estimates (bytes)
    constexpr int P2PKH_INPUT_SIZE   = 148;
    constexpr int P2PKH_OUTPUT_SIZE  = 34;
    constexpr int P2WPKH_INPUT_SIZE  = 68;
    constexpr int P2WPKH_OUTPUT_SIZE = 31;
    constexpr int TX_OVERHEAD        = 10;
    constexpr int SEGWIT_OVERHEAD    = 2;

    // Block weight constants (SegWit)
    constexpr int MAX_BLOCK_WEIGHT   = 4000000;
    constexpr int MAX_BLOCK_SIZE     = 1000000; // legacy

    // Block history entry for fee estimation
    struct BlockFeeRecord {
        int    height;
        double minFeeRate;
        double medianFeeRate;
        double maxFeeRate;
        int    txCount;
    };

    static std::vector<BlockFeeRecord> g_feeHistory;
    static std::mutex g_feeMutex;

    inline void recordBlock(int height, const std::vector<double>& fees) {
        std::lock_guard<std::mutex> lock(g_feeMutex);
        if (fees.empty()) return;
        auto sorted = fees;
        std::sort(sorted.begin(), sorted.end());
        BlockFeeRecord rec;
        rec.height     = height;
        rec.minFeeRate = sorted.front();
        rec.maxFeeRate = sorted.back();
        rec.medianFeeRate = sorted[sorted.size()/2];
        rec.txCount    = (int)fees.size();
        g_feeHistory.push_back(rec);
        if (g_feeHistory.size() > 1000) g_feeHistory.erase(g_feeHistory.begin());
    }

    inline FeeRate estimate(int targetBlocks = 3) {
        std::lock_guard<std::mutex> lock(g_feeMutex);
        FeeRate rate;
        if (g_feeHistory.empty()) return rate;

        int lookback = std::min((int)g_feeHistory.size(), targetBlocks * 2 + 1);
        double sumMedian = 0;
        for (int i = (int)g_feeHistory.size()-lookback; i < (int)g_feeHistory.size(); i++) {
            sumMedian += g_feeHistory[i].medianFeeRate;
        }
        double median = sumMedian / lookback;
        rate.fast    = median * 2.0;
        rate.normal  = median;
        rate.economy = median * 0.5;
        rate.minimum = 0.000003;
        return rate;
    }

    // Estimate tx size given input/output counts (virtual bytes for SegWit)
    inline int estimateTxSize(int numInputs, int numOutputs, bool segwit = false) {
        if (segwit) {
            int base   = TX_OVERHEAD + SEGWIT_OVERHEAD +
                         numInputs * P2WPKH_INPUT_SIZE +
                         numOutputs * P2WPKH_OUTPUT_SIZE;
            int weight = base * 4; // vbytes calculation
            return (weight + 3) / 4;
        }
        return TX_OVERHEAD + numInputs * P2PKH_INPUT_SIZE + numOutputs * P2PKH_OUTPUT_SIZE;
    }

    // Calculate fee for a given tx size and rate
    inline double calcFee(int txSizeBytes, double feeRatePerByte) {
        return txSizeBytes * feeRatePerByte;
    }

    // Dynamic block weight calculation
    inline int estimateBlockWeight(int numTxs, int avgTxSize, int numWitnessTxs = 0) {
        int baseSize   = numTxs * avgTxSize;
        int witnessSize= numWitnessTxs * 100; // approx witness overhead
        return (baseSize - witnessSize) * 3 + (baseSize + witnessSize);
    }

    // Priority transaction: fee rate well above median
    inline bool isPriority(double feeRate, double medianFeeRate) {
        return feeRate >= medianFeeRate * 1.5;
    }

    // RBF: new fee must be >= old fee + min relay fee * new tx size
    inline bool isValidRBF(double oldFee, double newFee, int newTxSize,
                           double minRelayFeeRate = 0.000003) {
        double minNewFee = oldFee + minRelayFeeRate * newTxSize;
        return newFee >= minNewFee;
    }

    // CPFP: child pays for parent — effective fee rate calculation
    inline double cpfpEffectiveFeeRate(double parentFee, int parentSize,
                                       double childFee,  int childSize) {
        double totalFee  = parentFee + childFee;
        double totalSize = parentSize + childSize;
        return totalFee / totalSize;
    }

    // Coin selection: Branch-and-Bound algorithm (simplified)
    struct UTXO { std::string txid; int vout; double amount; };

    inline std::vector<UTXO> selectCoins(const std::vector<UTXO>& available,
                                          double target, double feePerInput = 0.0001) {
        // Sort by value descending
        auto utxos = available;
        std::sort(utxos.begin(), utxos.end(),
                  [](const UTXO& a, const UTXO& b){ return a.amount > b.amount; });

        // Greedy selection
        std::vector<UTXO> selected;
        double total = 0;
        for (const auto& u : utxos) {
            selected.push_back(u);
            total += u.amount - feePerInput;
            if (total >= target) break;
        }
        if (total < target) return {}; // insufficient funds
        return selected;
    }

    // Dust output detection
    inline bool isDustOutput(double amount, double feeRatePerByte = 0.00003) {
        return amount < (feeRatePerByte * P2PKH_INPUT_SIZE);
    }

    // Auto-select fee based on urgency
    inline double autoFee(int urgencyBlocks, int txSizeBytes) {
        FeeRate rates = estimate(urgencyBlocks);
        double rate = (urgencyBlocks <= 1) ? rates.fast :
                      (urgencyBlocks <= 3) ? rates.normal : rates.economy;
        return std::max(calcFee(txSizeBytes, rate), rates.minimum * txSizeBytes);
    }

} // namespace FeeEstimation

#endif
