#ifndef WALLET_FEATURES_H
#define WALLET_FEATURES_H

// MazeChain — Advanced Wallet Features
// Change addresses, Address gap limit, Address labels,
// Watch-only wallets, PSBT, Cold wallet, Multi-account,
// Address gap limit (BIP44: scan 20 unused before stopping)

#include <string>
#include <vector>
#include <map>
#include <set>
#include <fstream>
#include <sstream>
#include <mutex>
#include <ctime>

namespace WalletFeatures {

    // ── Address Label Store ────────────────────────────────────────────────────
    struct AddressLabel {
        std::string address;
        std::string label;
        bool        watchOnly = false;
        int64_t     createdAt = 0;
    };

    static std::map<std::string, AddressLabel> g_labels;
    static std::mutex g_labelMutex;

    inline void setLabel(const std::string& address, const std::string& label,
                         bool watchOnly = false) {
        std::lock_guard<std::mutex> lock(g_labelMutex);
        g_labels[address] = {address, label, watchOnly, (int64_t)std::time(nullptr)};
    }

    inline std::string getLabel(const std::string& address) {
        std::lock_guard<std::mutex> lock(g_labelMutex);
        auto it = g_labels.find(address);
        return (it != g_labels.end()) ? it->second.label : "";
    }

    inline bool isWatchOnly(const std::string& address) {
        std::lock_guard<std::mutex> lock(g_labelMutex);
        auto it = g_labels.find(address);
        return (it != g_labels.end()) ? it->second.watchOnly : false;
    }

    inline std::vector<AddressLabel> getAllLabels() {
        std::lock_guard<std::mutex> lock(g_labelMutex);
        std::vector<AddressLabel> out;
        for (const auto& kv : g_labels) out.push_back(kv.second);
        return out;
    }

    inline void saveLabels(const std::string& path) {
        std::lock_guard<std::mutex> lock(g_labelMutex);
        std::ofstream f(path);
        for (const auto& kv : g_labels) {
            f << kv.first << "\t" << kv.second.label << "\t"
              << kv.second.watchOnly << "\t" << kv.second.createdAt << "\n";
        }
    }

    inline void loadLabels(const std::string& path) {
        std::lock_guard<std::mutex> lock(g_labelMutex);
        std::ifstream f(path);
        std::string line;
        while (std::getline(f, line)) {
            std::istringstream ss(line);
            AddressLabel al;
            int wo;
            ss >> al.address >> al.label >> wo >> al.createdAt;
            al.watchOnly = (wo != 0);
            g_labels[al.address] = al;
        }
    }

    // ── Change Address Manager ─────────────────────────────────────────────────
    struct ChangeAddressManager {
        std::vector<std::string> pool;       // pre-derived change addresses
        int                      nextIndex = 0;
        std::set<std::string>    used;

        std::string getNext() {
            while (nextIndex < (int)pool.size() && used.count(pool[nextIndex]))
                nextIndex++;
            if (nextIndex >= (int)pool.size()) return "";
            return pool[nextIndex++];
        }

        void markUsed(const std::string& addr) { used.insert(addr); }
        bool isChange(const std::string& addr) const {
            for (const auto& a : pool) if (a == addr) return true;
            return false;
        }
        int available() const { return (int)pool.size() - nextIndex; }
    };

    // ── Address Gap Limit (BIP44) ──────────────────────────────────────────────
    constexpr int GAP_LIMIT = 20;

    // Returns the number of consecutive unused addresses scanned
    // Stop deriving when gap >= GAP_LIMIT
    inline int checkGapLimit(const std::vector<std::string>& derived,
                              const std::set<std::string>& used) {
        int gap = 0, maxGap = 0;
        for (const auto& addr : derived) {
            if (!used.count(addr)) { gap++; maxGap = std::max(maxGap, gap); }
            else gap = 0;
        }
        return maxGap;
    }

    // ── PSBT — Partially Signed Bitcoin Transaction ────────────────────────────
    struct PSBTInput {
        std::string txid;
        int         vout;
        std::string scriptSig;
        std::string witnessScript;
        std::map<std::string, std::string> partialSigs; // pubkey -> sig
        uint32_t    nSequence = 0xffffffff;
        std::string redeemScript;
        std::string bip32Derivation;
    };

    struct PSBTOutput {
        std::string address;
        double      amount;
        std::string redeemScript;
        std::string bip32Derivation;
    };

    struct PSBT {
        std::string version   = "2.0";
        std::string txVersion = "1";
        std::vector<PSBTInput>  inputs;
        std::vector<PSBTOutput> outputs;
        std::string globalXpub;
        bool        finalized = false;

        // Add a partial signature to an input
        bool addSignature(int inputIdx, const std::string& pubkey,
                          const std::string& sig) {
            if (inputIdx < 0 || inputIdx >= (int)inputs.size()) return false;
            inputs[inputIdx].partialSigs[pubkey] = sig;
            return true;
        }

        // Check if all inputs have required signatures
        bool isComplete() const {
            for (const auto& inp : inputs) {
                if (inp.partialSigs.empty()) return false;
            }
            return true;
        }

        // Serialize to base64-like string (simplified)
        std::string serialize() const {
            std::ostringstream ss;
            ss << "psbt:" << version << ":" << inputs.size() << ":" << outputs.size();
            for (const auto& i : inputs) {
                ss << "|" << i.txid << ":" << i.vout;
                for (const auto& sig : i.partialSigs)
                    ss << "~" << sig.first.substr(0,8) << "=" << sig.second.substr(0,8);
            }
            for (const auto& o : outputs) ss << "|" << o.address << ":" << o.amount;
            ss << "|finalized=" << finalized;
            return ss.str();
        }
    };

    // ── Multi-Account Wallet ───────────────────────────────────────────────────
    struct Account {
        int         index;
        std::string name;
        std::string xpub;
        std::string rootAddress;
        bool        watchOnly = false;
        int64_t     createdAt = 0;
    };

    static std::map<int, Account> g_accounts;
    static std::mutex g_accountMutex;
    static int g_activeAccount = 0;

    inline int addAccount(const std::string& name, const std::string& xpub,
                          const std::string& rootAddr, bool watchOnly = false) {
        std::lock_guard<std::mutex> lock(g_accountMutex);
        int idx = (int)g_accounts.size();
        g_accounts[idx] = {idx, name, xpub, rootAddr, watchOnly, (int64_t)std::time(nullptr)};
        return idx;
    }

    inline std::vector<Account> getAccounts() {
        std::lock_guard<std::mutex> lock(g_accountMutex);
        std::vector<Account> out;
        for (const auto& kv : g_accounts) out.push_back(kv.second);
        return out;
    }

    inline void setActiveAccount(int idx) {
        std::lock_guard<std::mutex> lock(g_accountMutex);
        g_activeAccount = idx;
    }

    inline int getActiveAccount() { return g_activeAccount; }

    // ── Transaction History Index ──────────────────────────────────────────────
    struct TxRecord {
        std::string txid;
        std::string from;
        std::string to;
        double      amount;
        double      fee;
        int         blockHeight;
        int64_t     timestamp;
        bool        incoming;
        std::string label;
    };

    static std::map<std::string, std::vector<TxRecord>> g_txHistory; // addr -> txs
    static std::mutex g_txHistMutex;

    inline void recordTx(const std::string& address, const TxRecord& tx) {
        std::lock_guard<std::mutex> lock(g_txHistMutex);
        g_txHistory[address].push_back(tx);
    }

    inline std::vector<TxRecord> getTxHistory(const std::string& address) {
        std::lock_guard<std::mutex> lock(g_txHistMutex);
        auto it = g_txHistory.find(address);
        return (it != g_txHistory.end()) ? it->second : std::vector<TxRecord>{};
    }

} // namespace WalletFeatures

#endif
