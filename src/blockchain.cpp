#include "../include/blockchain.h"
#include "../include/storage.h"
#include "../include/crypto.h"
#include <iostream>
#include <cmath>
#include <iomanip>
#include <sstream>
#include <map>
#include <ctime>
#include <algorithm> // Para std::max e std::min

// Constantes da Economia MazeChain
const double MITS_PER_COIN = 100000000.0; 
const double MIN_FEE_RELAY = 0.00001; // Taxa mínima para evitar spam

Blockchain::Blockchain() {
    difficulty = 4;
    totalSupply = 0;
    
    // Tenta carregar o estado atual do UTXO de forma segura
    try {
        utxoSet.loadFromFile("data/utxo.dat");
    } catch (...) {
        std::cerr << "[ERRO] Falha ao carregar UTXO.dat. Iniciando novo conjunto." << std::endl;
    }

    if (chain.empty()) {
        std::vector<Transaction> genesisTxs;
        // O Bloco Gênesis não possui transações inicialmente
        Block genesis(0, "0", genesisTxs);
        genesis.hash = genesis.calculateHash(); 
        chain.push_back(genesis);
        std::cout << "[SISTEMA] Bloco Gênesis estabelecido com sucesso." << std::endl;
    }
}

void Blockchain::clearChain() {
    chain.clear();
    totalSupply = 0;
    utxoSet.utxos.clear();
}

bool Blockchain::isChainValid() {
    for (size_t i = 1; i < chain.size(); i++) {
        const Block& currentBlock = chain[i];
        const Block& prevBlock = chain[i-1];

        // Validação 1: Integridade do Hash do bloco atual
        if (currentBlock.hash != currentBlock.calculateHash()) {
            return false;
        }

        // Validação 2: Continuidade da corrente (Linkagem)
        if (currentBlock.prevHash != prevBlock.hash) {
            return false;
        }

        // Validação 3: Prova de Trabalho (Dificuldade)
        std::string target(difficulty, '0');
        if (currentBlock.hash.substr(0, difficulty) != target) {
            return false;
        }
    }
    return true;
}

void Blockchain::printStats() {
    std::cout << "\n==========================================" << std::endl;
    std::cout << "          ESTATÍSTICAS DA MAZECHAIN        " << std::endl;
    std::cout << "==========================================" << std::endl;
    std::cout << " Altura da Rede     : " << chain.size() << " blocos" << std::endl;
    std::cout << " Dificuldade Atual  : " << difficulty << std::endl;
    std::cout << " Circulante (Supply): " << std::fixed << std::setprecision(8) << totalSupply << " MZ" << std::endl;
    std::cout << " Supply Máximo      : " << std::fixed << std::setprecision(8) << getMaxSupply() << " MZ" << std::endl;
    std::cout << " Progresso de Emissão: " << (totalSupply / getMaxSupply() * 100.0) << "%" << std::endl;
    std::cout << "==========================================\n" << std::endl;
}

double Blockchain::getBlockReward(int height) {
    // Se já atingimos o limite, emissão zero
    if (totalSupply >= getMaxSupply()) return 0.0;

    double reward = 0.0;
    
    // Lógica de Subsídio Escalonado
    if (height > 0 && height <= 1000) {
        reward = 2000.0; // Era do Incentivo Turbo
    } else if (height > 1000 && height <= 2000) {
        reward = 1000.0; // Redução para 1000 (Halving 1)
    } else if (height > 2000 && height <= 4000) {
        reward = 500.0;  // Redução para 500 (Halving 2)
    } else if (height > 4000) {
        // Redução exponencial baseada na distância do bloco 4000
        int halvings = ((height - 4000) / 2000) + 3; 
        reward = 4000.0 / std::pow(2, halvings);
    }

    // Proteção de Overflow de Supply (Hard Cap)
    if (totalSupply + reward > getMaxSupply()) {
        reward = std::max(0.0, getMaxSupply() - totalSupply);
    }

    // Precisão mínima: 1 Mit (0.00000001 MZ)
    return (reward < 0.00000001) ? 0.0 : reward;
}

void Blockchain::mineBlock(std::string minerAddress) {
    // Validação rigorosa do endereço MZ (Proteção contra perda de fundos)
    if (minerAddress.length() < 30 || minerAddress.substr(0, 2) != "MZ") {
        std::cout << "❌ ERRO: Endereço de minerador inválido: " << minerAddress << std::endl;
        return;
    }

    adjustDifficulty();

    std::vector<Transaction> pending = Storage::loadMempool("data/mempool.dat");
    std::vector<Transaction> validTransactions;
    double totalFees = 0;
    std::map<std::string, double> spendingInThisBlock;

    for (const auto& tx : pending) {
        if (!verifyTransaction(tx)) continue;
        
        std::string sender = "";
        double amountWithFee = 0;
        
        // Localiza a saída negativa (o gasto/débito)
        for (const auto& out : tx.vout) {
            if (out.amount < 0) { 
                sender = out.address; 
                amountWithFee = std::abs(out.amount); 
                break;
            }
        }

        if (sender.empty()) continue;

        double currentBalance = getBalance(sender);
        // Verificação de saldo incluindo gastos pendentes no mesmo bloco (Prevenção de Double Spend)
        if (currentBalance - spendingInThisBlock[sender] >= (amountWithFee - 0.000000005)) {
            validTransactions.push_back(tx);
            spendingInThisBlock[sender] += amountWithFee;
            
            // Cálculo de taxa (Incentivo ao minerador)
            // A taxa é a diferença entre o que saiu e o que foi enviado
            double fee = amountWithFee - (amountWithFee / 1.01);
            totalFees += std::max(0.0, fee);
        }
    }

    double subsidy = getBlockReward(static_cast<int>(chain.size()));
    double totalReward = subsidy + totalFees;

    // Construção da Transação Coinbase (Geração de novas moedas)
    Transaction coinbase;
    coinbase.id = "coinbase_h" + std::to_string(chain.size()) + "_" + std::to_string(std::time(nullptr));
    coinbase.vout.push_back({minerAddress, totalReward});
    coinbase.signature = "coinbase";
    coinbase.publicKey = "MAZE_EMISSION_SYSTEM_2026";

    std::vector<Transaction> blockTxs;
    blockTxs.push_back(coinbase);
    // Adiciona as transações validadas da mempool
    blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

    Block newBlock(static_cast<int>(chain.size()), chain.back().hash, blockTxs);
    std::cout << "[MINER] Iniciando Prova de Trabalho no bloco #" << newBlock.index << "..." << std::endl;
    std::cout << "[MINER] Subsídio: " << subsidy << " MZ | Taxas: " << totalFees << " MZ" << std::endl;

    newBlock.mine(difficulty);
    
    // Adição formal à corrente
    addBlock(newBlock);
    
    // Persistência de dados (Atomicidade)
    Storage::saveChain(*this, "data/blockchain.dat");
    utxoSet.saveToFile("data/utxo.dat"); 
    Storage::clearMempool("data/mempool.dat");

    std::cout << "🎯 Bloco #" << newBlock.index << " aceito pela rede! Hash: " << newBlock.hash.substr(0,10) << "..." << std::endl;
}

void Blockchain::addBlock(const Block& block) {
    chain.push_back(block);
    
    // Atualização do Supply (Apenas para blocos minerados, ignora Gênesis se já processado)
    if (block.index > 0) {
        totalSupply += getBlockReward(block.index);
    }
    
    // Atualização do UTXO Set (Gasto e Recebimento)
    for(const auto& tx : block.transactions) {
        utxoSet.update(tx);
    }
}

bool Blockchain::verifyTransaction(const Transaction& tx) {
    if (tx.signature == "coinbase") return true;
    
    std::string senderAddress = "";
    for (const auto& out : tx.vout) {
        if (out.amount < 0) {
            senderAddress = out.address;
            break;
        }
    }

    if (senderAddress.empty()) return false;

    // Validação da Prova de Chave (Protocolo SALT_MAZE_2026_PRODUCTION)
    // O endereço deve obrigatoriamente derivar da chave pública enviada
    std::string h1 = Crypto::sha256_util(tx.publicKey);
    std::string expectedAddress = "MZ" + Crypto::sha256_util(h1 + "SALT_MAZE_2026_PRODUCTION").substr(0, 32);
    
    return (senderAddress == expectedAddress);
}

void Blockchain::adjustDifficulty() {
    // Reajuste a cada 10 blocos (Retargeting)
    if (chain.size() < 11) return;

    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[chain.size() - 10];
    
    long timeTarget = 600; // Alvo de 10 minutos para 10 blocos (1 min por bloco)
    long timeTaken = static_cast<long>(lastBlock.timestamp - relayBlock.timestamp);

    // Algoritmo de ajuste suave para evitar oscilações bruscas de Hashrate
    if (timeTaken < (timeTarget / 2)) {
        difficulty++;
        std::cout << "[REDE] Dificuldade aumentada para: " << difficulty << std::endl;
    } else if (timeTaken > (timeTarget * 2)) {
        if (difficulty > 1) {
            difficulty--;
            std::cout << "[REDE] Dificuldade diminuída para: " << difficulty << std::endl;
        }
    }
}

double Blockchain::getBalance(std::string address) { 
    return utxoSet.getBalance(address); 
}

void Blockchain::send(std::string from, std::string to, double amount, std::string seed) {
    // Taxa de rede fixa em 1% para sustentabilidade
    double totalNeeded = amount * 1.01;
    
    if (amount <= 0) throw std::runtime_error("Valor de envio deve ser positivo.");
    
    if (getBalance(from) < totalNeeded) {
        throw std::runtime_error("Saldo insuficiente (Lembre-se da taxa de 1%).");
    }

    Transaction tx;
    // O ID da transação inclui um salt temporal para unicidade de hash
    tx.id = Crypto::sha256_util(from + to + std::to_string(amount) + std::to_string(std::time(nullptr)));
    
    tx.vout.push_back({to, amount});              // Saída de recebimento
    tx.vout.push_back({from, totalNeeded * -1});   // Saída de débito (incluindo taxa)
    
    tx.publicKey = seed; // No MazeChain v2, a seed/chave é necessária para assinar o envio
    tx.signature = "SIG_" + Crypto::sha256_util(seed + tx.id).substr(0, 24);
    
    Storage::saveMempool(tx, "data/mempool.dat");
    std::cout << "✅ Transação de " << amount << " MZ enviada para a mempool!" << std::endl;
}

// Métodos de Acesso
std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
double Blockchain::getTotalSupply() const { return totalSupply; }
double Blockchain::getMaxSupply() const { return 20000000.0; } // Hard Cap: 20 Milhões
