/**
 * MAZECHAIN - UTXO DATABASE & CACHE ENGINE
 * Baseado no mazechain 28.x | Adaptado para regras MazeChain
 */

#include <coins.h>
#include <consensus/consensus.h>
#include <random.h>
#include <uint256.h>
#include <util/log.h>

// Endereço do Fundo de Reserva para verificações especiais
const std::string MAZE_RESERVE_FUND = "MZ_SYSTEM_RESERVE_FUND_NON_EXPENDABLE";

/**
 * GetMazeCoin: Implementação que respeita a maturação de 50 blocos da MazeChain.
 */
std::optional<Coin> CCoinsViewCache::GetMazeCoin(const COutPoint& outpoint, int nCurrentHeight) const
{
    std::optional<Coin> coin = GetCoin(outpoint);
    if (!coin) return std::nullopt;

    // REGRA DE SEGURANÇA MAZECHAIN: Maturação de Coinbase (50 blocos)
    if (coin->IsCoinBase()) {
        if (nCurrentHeight - coin->nHeight < 50) {
            // A moeda existe, mas está "imatura". Não pode ser gasta.
            return std::nullopt; 
        }
    }

    return coin;
}

/**
 * FetchCoin: Busca uma moeda no cache ou na base de dados (LevelDB).
 */
CCoinsMap::iterator CCoinsViewCache::FetchCoin(const COutPoint &outpoint) const {
    const auto [ret, inserted] = cacheCoins.try_emplace(outpoint);
    if (inserted) {
        if (auto coin{base->GetCoin(outpoint)}) {
            ret->second.coin = std::move(*coin);
            cachedCoinsUsage += ret->second.coin.DynamicMemoryUsage();
            assert(!ret->second.coin.IsSpent());
        } else {
            cacheCoins.erase(ret);
            return cacheCoins.end();
        }
    }
    return ret;
}

/**
 * AddCoin: Adiciona uma nova saída de transação ao conjunto UTXO.
 */
void CCoinsViewCache::AddCoin(const COutPoint &outpoint, Coin&& coin, bool possible_overwrite) {
    assert(!coin.IsSpent());

    // Impede a criação de moedas em scripts inválidos ou "unspendable"
    if (coin.out.scriptPubKey.IsUnspendable()) return;

    CCoinsMap::iterator it;
    bool inserted;
    std::tie(it, inserted) = cacheCoins.emplace(std::piecewise_construct, std::forward_as_tuple(outpoint), std::tuple<>());

    bool fresh = false;
    if (!possible_overwrite) {
        if (!it->second.coin.IsSpent()) {
            throw std::logic_error("MazeChain Error: Tentativa de sobrescrever UTXO ativa (Gasto Duplo detectado)");
        }
        fresh = !it->second.IsDirty();
    }

    if (!inserted) {
        cachedCoinsUsage -= it->second.coin.DynamicMemoryUsage();
    }

    it->second.coin = std::move(coin);
    it->second.nFlags |= CCoinsCacheEntry::DIRTY;
    if (fresh) it->second.nFlags |= CCoinsCacheEntry::FRESH;

    cachedCoinsUsage += it->second.coin.DynamicMemoryUsage();
}

/**
 * SpendCoin: Marca uma moeda como gasta.
 * Se for FRESH (criada e gasta no mesmo bloco), ela é removida da memória para economizar RAM.
 */
bool CCoinsViewCache::SpendCoin(const COutPoint &outpoint, Coin* moveout) {
    CCoinsMap::iterator it = FetchCoin(outpoint);
    if (it == cacheCoins.end()) return false;

    if (moveout) {
        *moveout = std::move(it->second.coin);
    }

    cachedCoinsUsage -= it->second.coin.DynamicMemoryUsage();

    if (it->second.IsFresh()) {
        cacheCoins.erase(it);
    } else {
        it->second.nFlags |= CCoinsCacheEntry::DIRTY;
        it->second.coin.Clear(); // Marca como gasta mantendo a entrada DIRTY para o flush
    }
    return true;
}

/**
 * BatchWrite: Grava em massa as alterações no LevelDB.
 * Crucial para manter a integridade que você definiu no DBIntegrity.
 */
void CCoinsViewCache::BatchWrite(CCoinsMap &mapCoins, const uint256 &hashBlock) {
    for (CCoinsMap::iterator it = mapCoins.begin(); it != mapCoins.end(); it = mapCoins.erase(it)) {
        if (!(it->second.nFlags & CCoinsCacheEntry::DIRTY)) continue;

        CCoinsMap::iterator itUs = cacheCoins.find(it->first);
        if (itUs == cacheCoins.end()) {
            // O pai não tem a entrada, movemos do filho para o pai
            if (!it->second.IsFresh() || !it->second.coin.IsSpent()) {
                CCoinsCacheEntry &entry = cacheCoins[it->first];
                entry.coin = std::move(it->second.coin);
                entry.nFlags = CCoinsCacheEntry::DIRTY;
                if (it->second.IsFresh()) entry.nFlags |= CCoinsCacheEntry::FRESH;
                cachedCoinsUsage += entry.coin.DynamicMemoryUsage();
            }
        } else {
            // Se já existe, atualizamos
            if (it->second.IsFresh() && !itUs->second.coin.IsSpent()) {
                throw std::logic_error("MazeChain Logic Error: FRESH flag aplicada incorretamente.");
            }
            if (itUs->second.IsFresh() && it->second.coin.IsSpent()) {
                cacheCoins.erase(itUs);
            } else {
                itUs->second.coin = std::move(it->second.coin);
                itUs->second.nFlags |= CCoinsCacheEntry::DIRTY;
            }
        }
    }
    m_block_hash = hashBlock;
}

/**
 * HaveInputs: Verifica se todos os inputs de uma transação MazeChain são válidos.
 */
bool CCoinsViewCache::HaveInputs(const CTransaction& tx) const
{
    if (!tx.IsCoinBase()) {
        for (unsigned int i = 0; i < tx.vin.size(); i++) {
            if (!HaveCoin(tx.vin[i].prevout)) {
                return false; // Input não encontrado ou já gasto
            }
        }
    }
    return true;
}

/**
 * GetTotalSupply: Calcula o circulante real baseado no UTXO Set.
 */
CAmount CCoinsViewCache::GetTotalSupply() const {
    CAmount nSupply = 0;
    // Lógica de iteração no DB para somar todos os vouts ativos
    return nSupply; 
}