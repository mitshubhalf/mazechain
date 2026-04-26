// Copyright (c) 2026 mits
// Copyright (c) 2026-present The mazecoin Core developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#ifndef MAZECHAIN_COINS_H
#define MAZECHAIN_COINS_H

#include <compressor.h>
#include <core_memusage.h>
#include <memusage.h>
#include <primitives/transaction.h>
#include <serialize.h>
#include <uint256.h>
#include <util/check.h>
#include <util/hasher.h>

#include <cassert>
#include <cstdint>
#include <unordered_map>

/**
 * Classe Coin - Representa uma saída de transação (UTXO) na MazeChain.
 * Adaptada para suportar a lógica de Maturação de 50 blocos.
 */
class Coin
{
public:
    //! Saída da transação (script e valor em MZ)
    CTxOut out;

    //! Identifica se a moeda veio de uma recompensa de mineração (Coinbase)
    bool fCoinBase : 1;

    //! Altura do bloco onde a moeda foi gerada
    uint32_t nHeight : 31;

    Coin(CTxOut&& outIn, int nHeightIn, bool fCoinBaseIn) : out(std::move(outIn)), fCoinBase(fCoinBaseIn), nHeight(nHeightIn) {}
    Coin(const CTxOut& outIn, int nHeightIn, bool fCoinBaseIn) : out(outIn), fCoinBase(fCoinBaseIn), nHeight(nHeightIn) {}
    Coin() : fCoinBase(false), nHeight(0) {}

    void Clear() {
        out.SetNull();
        fCoinBase = false;
        nHeight = 0;
    }

    bool IsCoinBase() const { return fCoinBase; }

    /** * MazeChain Maturity Check:
     * Verifica se a moeda já pode ser gasta. 
     * Se for Coinbase, exige 50 blocos de profundidade.
     */
    bool IsSpendable(int nSpendHeight) const {
        if (!fCoinBase) return true;
        return (nSpendHeight - (int)nHeight >= 50);
    }

    bool IsSpent() const { return out.IsNull(); }

    size_t DynamicMemoryUsage() const {
        return memusage::DynamicUsage(out.scriptPubKey);
    }

    // Serialização customizada para performance industrial
    template<typename Stream>
    void Serialize(Stream &s) const {
        assert(!IsSpent());
        uint32_t code{(uint32_t{nHeight} << 1) | uint32_t{fCoinBase}};
        ::Serialize(s, VARINT(code));
        ::Serialize(s, Using<TxOutCompression>(out));
    }

    template<typename Stream>
    void Unserialize(Stream &s) {
        uint32_t code = 0;
        ::Unserialize(s, VARINT(code));
        nHeight = code >> 1;
        fCoinBase = code & 1;
        ::Unserialize(s, Using<TxOutCompression>(out));
    }
};

/** * Estrutura de entrada no Cache de Moedas.
 * Gerencia os estados DIRTY (modificado) e FRESH (novo no bloco).
 */
struct CCoinsCacheEntry
{
    Coin coin;
    unsigned char flags;

    enum Flags {
        DIRTY = (1 << 0), // Precisa ser gravado no disco
        FRESH = (1 << 1), // Não existe no disco ainda (economiza IO)
    };

    CCoinsCacheEntry() : flags(0) {}
    explicit CCoinsCacheEntry(Coin&& coin_) : coin(std::move(coin_)), flags(0) {}
};

typedef std::unordered_map<COutPoint, CCoinsCacheEntry, SaltedOutpointHasher> CCoinsMap;

/** * CCoinsView: Interface abstrata para o conjunto de UTXOs.
 */
class CCoinsView
{
public:
    virtual std::optional<Coin> GetCoin(const COutPoint &outpoint) const = 0;
    virtual bool HaveCoin(const COutPoint &outpoint) const = 0;
    virtual uint256 GetBestBlock() const = 0;
    virtual void BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) = 0;
    virtual ~CCoinsView() {}
};

/** * CCoinsViewCache: O cérebro da memória RAM da MazeChain.
 * Mantém as moedas em cache para validação ultra-rápida.
 */
class CCoinsViewCache : public CCoinsView
{
protected:
    CCoinsView *base;
    mutable uint256 hashBlock;
    mutable CCoinsMap cacheCoins;
    mutable size_t cachedCoinsUsage;

public:
    CCoinsViewCache(CCoinsView *baseIn);

    // Métodos de consulta (Respeitam as leis da MazeChain)
    std::optional<Coin> GetCoin(const COutPoint &outpoint) const override;
    bool HaveCoin(const COutPoint &outpoint) const override;

    /** Retorna a moeda apenas se ela estiver madura (>50 blocos se Coinbase) */
    std::optional<Coin> GetSpendableCoin(const COutPoint &outpoint, int nHeight) const;

    // Métodos de modificação
    void AddCoin(const COutPoint &outpoint, Coin&& coin, bool possible_overwrite);
    bool SpendCoin(const COutPoint &outpoint, Coin* moveout = nullptr);

    /** Envia as moedas do cache para o banco de dados LevelDB */
    void Flush();

    /** Verifica se a transação tem todos os inputs disponíveis e maduros */
    bool HaveInputs(const CTransaction& tx, int nHeight) const;

    uint256 GetBestBlock() const override;
    void SetBestBlock(const uint256 &hashBlockIn);

    void BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) override;

    size_t DynamicMemoryUsage() const;
};

#endif // MAZECHAIN_COINS_H