// Copyright (c) 2026 mits
// Copyright (c) 2009-present mazechain Core developers
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
 * Versão Sincronizada: Lógica de Dificuldade 5 (Fixa até 10k) e Dinâmica Progressiva
 * Recompensa de Bloco Atualizada para 150 MZ.
 */

class CMainParams : public CChainParams {
public:
    CMainParams() {
        strNetworkID = "main";

        // 1. REGRAS DE CONSENSO
        consensus.nSubsidyHalvingInterval = 10000; // Halving a cada 10k blocos
        consensus.BIP34Height = 1; 
        consensus.BIP65Height = 1; 
        consensus.BIP66Height = 1;
        consensus.CSVHeight = 1;
        consensus.SegwitHeight = 1;

        // 2. PARÂMETROS DE DIFICULDADE E TEMPO (POW)
        // Definido para suportar o patamar inicial de Dificuldade 5
        consensus.powLimit = uint256S("00000fffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        /**
         * AJUSTE DE TEMPO ALVO:
         * TargetSpacing: 1 minuto (60 segundos).
         * TargetTimespan: 10 minutos (curto para reagir rápido a mudanças de Hashrate).
         */
        consensus.nPowTargetTimespan = 10 * 60; 
        consensus.nPowTargetSpacing = 1 * 60;   

        consensus.fPowAllowMinDifficultyBlocks = false;
        consensus.fPowNoRetargeting = false;

        // 3. ENDEREÇO DO FUNDO DE RESERVA
        consensus.m_maze_reserve_fund_address = "MZ_SYSTEM_RESERVE_FUND_NON_EXPENDABLE";

        /**
         * 4. CONFIGURAÇÃO DAS JANELAS DE DIFICULDADE (Sincronizado com Blockchain.cpp)
         * Definimos as alturas onde o "piso" da dificuldade sobe.
         */
        consensus.nHeightLevel5 = 1;       // Dificuldade 5 começa no bloco 1
        consensus.nHeightDynamic5 = 10001; // Ativação do Retargeting Dinâmico (Base 5)
        consensus.nHeightLevel6 = 20001;   // Salto para Dificuldade 6 + Dinâmica

        /**
         * O Bloco Gênesis da MazeChain
         */
        const char* pszTimestamp = "MazeChain 2026: The evolution of decentralized reserve systems";

        // Script de saída do Genesis
        const CScript genesisOutputScript = CScript() << ParseHex("04678afdb0fe5548271967f1a67130b7105cd6a828e03909a67962e0ea1f61deb649f6bc3f4cef38c4f35504e51ec112de5c384df7ba0b8d578a4c702b6bf11d5f") << OP_CHECKSIG;

        /**
         * Parâmetros do Gênesis:
         * nTime: 1714131539 (Sincronizado com o seu Blockchain.cpp)
         * nNonce: 12345 (Ajustado para o novo valor de recompensa)
         * nBits: 0x1f00ffff (Representa o limite powLimit)
         * Recompensa: 150 * COIN (Atualizado de 400 para 150)
         */
        genesis = CreateGenesisBlock(1714131539, 12345, 0x1f00ffff, 1, 150 * COIN);
        consensus.hashGenesisBlock = genesis.GetHash();

        // 5. REDE P2P E PORTAS
        nDefaultPort = 9333;
        nPruneAfterHeight = 1000;
        m_assumed_blockchain_size = 2;
        m_assumed_chain_state_size = 1;

        // Seed nodes
        vSeeds.emplace_back("seed.mazechain.org"); 

        // Prefixos de Endereço MazeChain
        base58Prefixes[PUBKEY_ADDRESS] = std::vector<unsigned char>(1, 50); // Endereços começam com 'M'
        base58Prefixes[SCRIPT_ADDRESS] = std::vector<unsigned char>(1, 58);
        base58Prefixes[SECRET_KEY] =     std::vector<unsigned char>(1, 128);

        fDefaultCheckMemPool = false;
        fAllowMultipleAddressesFromGroup = false;

        /**
         * CHECKPOINTS DE SEGURANÇA
         * Limpo para evitar o travamento no bloco 5000.
         * O hash deve coincidir com o novo Genesis de 150 MZ.
         */
        checkpointData = {
            {
                {0, consensus.hashGenesisBlock}, 
            }
        };

        chainTxData = ChainTxData{
            1714131539, 
            0,          
            0           
        };
    }
};