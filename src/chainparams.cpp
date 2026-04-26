// Copyright (c) 2026
// Copyright (c) 2026-present The MAZECHAIN developers
// Copyright (c) 2026-present The MazeChain developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <kernel/chainparams.h>
#include <consensus/amount.h>
#include <consensus/params.h>
#include <util/chaintype.h>
#include <util/strencodings.h>

/**
 * MAZECHAIN - CONFIGURAÇÃO DE PARÂMETROS DA REDE
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";

        // 1. REGRAS DE CONSENSO (A LEI DA MAZECHAIN)
        consensus.nSubsidyHalvingInterval = 10000; // Halving a cada 10k blocos
        consensus.BIP34Height = 1; 
        consensus.BIP65Height = 1; 
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 1;

        // 2. PARÂMETROS DE DIFICULDADE E TEMPO
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");
        consensus.nPowTargetTimespan = 10 * 60; // Ajuste contínuo (alvo de 10 min)
        consensus.nPowTargetSpacing = 10 * 60;  // 10 minutos entre blocos
        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;

        // 3. ENDEREÇO DO FUNDO DE RESERVA (Taxas de 1% a 7%)
        // TODO: Substituir pela chave pública real da sua carteira de reserva
        consensus.m_maze_reserve_fund_address = "maze_reserve_fund_address_placeholder";

        // 4. CONFIGURAÇÃO DAS JANELAS DE DIFICULDADE (Sua Regra Específica)
        // Essas alturas serão consultadas no pow.cpp adaptado
        consensus.nHeightLevel4 = 5000;
        consensus.nHeightLevel6 = 10000;

        /**
         * O Bloco Gênesis da MazeChain
         */
        const char* pszTimestamp = "MazeChain 2026: The evolution of decentralized reserve systems";
        const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;

        // nTime, nNonce, nBits, nVersion, genesisReward
        genesis = CreateGenesisBlock(1714131539, 12345, 0x1e0ffff0, 1, 400 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

        // 5. REDE P2P E PORTAS
        // Porta padrão da MazeChain: 9333 (exemplo)
        nDefaultPort = 9333;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 2;
        m_assumed_chain_state_size = 1;

        // Seed Nodes (DNS Seeds para os nós se encontrarem)
        vSeeds.emplace_back("seed.mazechain.org"); 

        // Prefixos de Endereço (Ex: Endereços Maze começam com 'M')
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 50); // 'M'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 58);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1, 128);

        fDefaultCheckMemPool = false;
        fAllowMultipleAddressesFromGroup = false;
        fHasGenerateIndex = false;

        checkpointData = {
            {
                {0, consensus.hashGenesisBlock},
            }
        };

        chainTxData = ChainTxData{
            0, // timestamp
            0, // total de transações
            0  // tx por segundo
        };
    }
};