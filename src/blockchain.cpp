                                          // 1. DEFINIÇÕES DE SISTEMA (Devem vir antes de tudo para o ASIO e Crow funcionarem no Nix/Linux)
                                          #ifndef _POSIX_C_SOURCE
                                          #define _POSIX_C_SOURCE 200809L
                                          #endif

                                          #include <sys/uio.h>   // Resolve o erro do 'struct iovec'
                                          #include <exception>   // Resolve o erro 'terminate is not a member of std'
                                          #include <cstring>     // Resolve o erro 'memcpy/strlen was not declared'

                                          // 2. CABEÇALHOS DE TERCEIROS (ASIO e Chrono)
                                          #include <chrono>

                                          // 3. CABEÇALHOS DA BIBLIOTECA PADRÃO C++
                                          #include <iostream>
                                          #include <cmath>
                                          #include <iomanip>
                                          #include <sstream>
                                          #include <map>
                                          #include <algorithm>
                                          #include <cstdint>
                                          #include <vector>
                                          #include <string>

                                          // 4. CABEÇALHOS C
                                          #include <ctime>

                                          // 5. SEUS CABEÇALHOS LOCAIS
                                          #include "../include/blockchain.h"
                                          #include "../include/storage.h"
                                          #include "../include/crypto.h"
                                          #include "../include/mining_utils.h"

                                          // Caminhos de Dados
                                          const std::string DB_PATH = "/home/runner/workspace/data/blockchain.dat";
                                          const std::string UTXO_PATH = "/home/runner/workspace/data/utxo.dat";
                                          const std::string MEMPOOL_PATH = "/home/runner/workspace/data/mempool.dat";

                                          const double MITS_PER_COIN = 100000000.0; 
                                          const double MIN_FEE_RELAY = 0.00001; 

                                          Blockchain::Blockchain() {
                                              difficulty = 4;
                                              totalSupply = 0;

                                              Storage::loadChain(*this, DB_PATH);

                                              if (this->chain.empty()) {
                                                  std::vector<Transaction> genesisTxs;

                                                  Transaction coinbase;
                                                  coinbase.id = "coinbase_genesis_" + std::to_string(std::time(nullptr));
                                                  // DEFINITIVO: Recompensa inicial do Gênesis em 400 MZ
                                                  coinbase.vout.push_back({"MZ_GENESIS_ADDRESS", 400.0}); 
                                                  coinbase.signature = "coinbase";
                                                  coinbase.publicKey = "MAZE_GENESIS_MESSAGE: The Times 21/Apr/2026 MazeChain reborn.";
                                                  genesisTxs.push_back(coinbase);

                                                  std::string zeroHash(64, '0');

                                                  Block genesis(0, zeroHash, genesisTxs, "GENESIS_BLOCK", 0);
                                                  genesis.hash = genesis.calculateHash(); 
                                                  chain.push_back(genesis);

                                                  std::cout << "[SISTEMA] Novo Bloco Gênesis estabelecido (400 MZ)." << std::endl;
                                              } else {
                                                  std::cout << "✅ Blockchain carregada do disco. Altura: " << chain.size() << std::endl;
                                              }

                                              // O rebuild vai garantir que o totalSupply seja recalculado com a regra correta
                                              rebuildUTXO();
                                          }

                                          // SISTEMA DE RECOMPENSA: 400 MZ + TRANSIÇÕES DINÂMICAS (50%, 20%, 10%, 2%)
                                          double Blockchain::getBlockReward(int height) {
                                              int interval = 10000; 
                                              int halving_count = height / interval;

                                              if (halving_count >= 64) return 0.00000001;

                                              double reward = 400.0; // Base inicial fixa em 400 MZ

                                              if (halving_count < 4) {
                                                  for (int i = 0; i < halving_count; i++) reward *= 0.5;
                                              } 
                                              else if (halving_count < 20) {
                                                  reward = 40.0; 
                                                  for (int i = 4; i < halving_count; i++) reward *= 0.80;
                                              }
                                              else if (halving_count < 50) {
                                                  reward = 1.407; 
                                                  for (int i = 20; i <= halving_count; i++) reward *= 0.90;
                                              }
                                              else {
                                                  reward = 0.060; 
                                                  for (int i = 50; i < halving_count; i++) reward *= 0.98;
                                              }

                                              return (reward < 0.00000001) ? 0.00000001 : reward;
                                          }

                                          // SISTEMA DE DIFICULDADE: Bloco 1-5k Nível 4 | 5001+ Dinâmica (4 a 7)
                                          void Blockchain::adjustDifficulty() {
                                              int currentHeight = static_cast<int>(chain.size());

                                              if (currentHeight <= 5000) {
                                                  difficulty = 4;
                                                  return;
                                              }

                                              int baseDifficulty = 4;
                                              int maxDifficulty = 7; 

                                              if (currentHeight >= 20000) baseDifficulty = 5; 
                                              else if (currentHeight >= 10001) baseDifficulty = 4;

                                              if (difficulty < baseDifficulty) difficulty = baseDifficulty;

                                              if (currentHeight < 11) return;

                                              const Block& lastBlock = chain.back();
                                              const Block& relayBlock = chain[chain.size() - 10];

                                              long timeTaken = static_cast<long>(lastBlock.timestamp - relayBlock.timestamp);

                                              if (timeTaken < 100) { 
                                                  if (difficulty < maxDifficulty) {
                                                      difficulty += 1;
                                                      std::cout << "⚠️ [REDE] Ajuste: Dificuldade subiu para " << difficulty << std::endl;
                                                  }
                                              } 
                                              else if (timeTaken > 1200) { 
                                                  if (difficulty > baseDifficulty) {
                                                      difficulty--;
                                                      std::cout << "📉 [REDE] Ajuste: Dificuldade baixou para " << difficulty << std::endl;
                                                  }
                                              }

                                              if (difficulty > maxDifficulty) difficulty = maxDifficulty;
                                          }

                                          // CORREÇÃO DE SALDO: Zera e recalcula estritamente baseado na Chain
                                          void Blockchain::rebuildUTXO() {
                                              std::cout << "🔄 Sincronizando saldos com a blockchain..." << std::endl;

                                              utxoSet.utxoMap.clear(); 
                                              utxoSet.addressBalances.clear();
                                              totalSupply = 0; 

                                              for (const auto& block : chain) {
                                                  totalSupply += getBlockReward(block.index);
                                                  for (const auto& tx : block.transactions) {
                                                      utxoSet.update(tx);
                                                  }
                                              }

                                              utxoSet.saveToFile(UTXO_PATH);
                                              std::cout << "✅ Saldos sincronizados. Supply total: " << std::fixed << std::setprecision(8) << totalSupply << " MZ" << std::endl;
                                          }

                                          void Blockchain::mineBlock(std::string minerAddress) {
                                              if (minerAddress.length() < 30 || minerAddress.substr(0, 2) != "MZ") {
                                                  std::cout << "❌ ERRO: Endereço de minerador inválido!" << std::endl;
                                                  return;
                                              }

                                              adjustDifficulty();

                                              std::vector<Transaction> pending = Storage::loadMempool(MEMPOOL_PATH);
                                              std::vector<Transaction> validTransactions;
                                              double totalFees = 0;
                                              std::map<std::string, double> spendingInThisBlock;

                                              for (const auto& tx : pending) {
                                                  if (tx.publicKey.empty()) continue; 
                                                  if (!verifyTransaction(tx)) {
                                                      std::cout << "⚠️ [MEMPOOL] Transação " << tx.id << " inválida (Assinatura falhou)!" << std::endl;
                                                      continue;
                                                  }

                                                  std::string sender = "";
                                                  double amountWithFee = 0;
                                                  double netValue = 0;

                                                  for (const auto& out : tx.vout) {
                                                      if (out.amount < 0) { 
                                                          sender = out.address; 
                                                          amountWithFee = std::abs(out.amount); 
                                                      } else {
                                                          netValue += out.amount;
                                                      }
                                                  }

                                                  if (sender.empty()) continue;
                                                  if (getBalance(sender) - spendingInThisBlock[sender] >= amountWithFee) {
                                                      validTransactions.push_back(tx);
                                                      spendingInThisBlock[sender] += amountWithFee;
                                                      totalFees += (amountWithFee - netValue);
                                                  }
                                              }

                                              int currentHeight = static_cast<int>(chain.size());
                                              MinerIdentity id = prepare_miner_identity(minerAddress.c_str(), currentHeight);

                                              double subsidy = getBlockReward(currentHeight); 
                                              double totalReward = subsidy + totalFees;

                                              Transaction coinbase;
                                              coinbase.id = "coinbase_h" + std::to_string(currentHeight) + "_" + std::to_string(std::time(nullptr));
                                              coinbase.vout.push_back({minerAddress, totalReward});
                                              coinbase.signature = "coinbase";
                                              coinbase.publicKey = "MAZE_EMISSION_SYSTEM_V3_DYNAMIC";

                                              std::vector<Transaction> blockTxs;
                                              blockTxs.push_back(coinbase);
                                              blockTxs.insert(blockTxs.end(), validTransactions.begin(), validTransactions.end());

                                              Block newBlock(currentHeight, chain.back().hash, blockTxs, id.miner_address, id.extra_nonce);

                                              std::cout << "⛏️ Buscando Hash (Dificuldade: " << difficulty << " | Recompensa: " << subsidy << " MZ)..." << std::endl;
                                              newBlock.mine(difficulty);

                                              addBlock(newBlock);

                                              Storage::saveChain(*this, DB_PATH);
                                              utxoSet.saveToFile(UTXO_PATH); 
                                              Storage::clearMempool(MEMPOOL_PATH);

                                              std::cout << "🎯 Bloco #" << newBlock.index << " OK! Recompensa: " << subsidy << " MZ" << std::endl;
                                          }

                                          void Blockchain::addBlock(const Block& block) {
                                              if (block.index >= (int)chain.size()) {
                                                  chain.push_back(block);
                                                  totalSupply += getBlockReward(block.index);
                                                  for(const auto& tx : block.transactions) {
                                                      utxoSet.update(tx);
                                                  }
                                              }
                                          }

                                          bool Blockchain::isChainValid() {
                                              if (chain.size() <= 1) return true;
                                              for (size_t i = 1; i < chain.size(); i++) {
                                                  if (chain[i].prevHash != chain[i-1].hash) return false;
                                                  if (chain[i].hash != chain[i].calculateHash()) return false;
                                              }
                                              return true;
                                          }

                                          void Blockchain::printStats() {
                                              std::cout << "\n==========================================" << std::endl;
                                              std::cout << "           ESTATÍSTICAS DA MAZECHAIN        " << std::endl;
                                              std::cout << "==========================================" << std::endl;
                                              std::cout << " Altura da Rede     : " << chain.size() << " blocos" << std::endl;
                                              std::cout << " Dificuldade Atual  : " << difficulty << std::endl;
                                              std::cout << " Circulante (Supply): " << std::fixed << std::setprecision(8) << totalSupply << " MZ" << std::endl;
                                              std::cout << " Supply Máximo      : " << getMaxSupply() << " MZ" << std::endl;
                                              std::cout << "==========================================\n" << std::endl;
                                          }

                                          double Blockchain::getBalance(std::string address) { 
                                              return utxoSet.getBalance(address); 
                                          }

                                          // IMPLEMENTAÇÃO DE ASSINATURA DIGITAL (ECDSA)
                                          bool Blockchain::verifyTransaction(const Transaction& tx) {
                                              if (tx.signature == "coinbase") return true;

                                              // Os dados que foram assinados: quem envia + quem recebe + valor
                                              // Importante: No seu sistema vout[0] é o destino e vout[1] é o remetente (valor negativo)
                                              if (tx.vout.size() < 2) return false;

                                              std::string data_to_verify = tx.vout[0].address + std::to_string(tx.vout[0].amount);

                                              // Chama a função criptográfica real do namespace Crypto
                                              return Crypto::verify_signature(data_to_verify, tx.signature, tx.publicKey);
                                          }

                                          void Blockchain::send(std::string from, std::string to, double amount, std::string seed) {
                                              double totalNeeded = amount + (amount * 0.01);

                                              if (getBalance(from) < totalNeeded) {
                                                  throw std::runtime_error("Saldo insuficiente para cobrir o envio e a taxa de 1%.");
                                              }

                                              // 1. Gerar o par de chaves ECDSA reais a partir da Seed
                                              Crypto::KeyPair keys = Crypto::generate_keys_from_seed(seed);

                                              // 2. Preparar os dados para assinatura
                                              std::string data_to_sign = to + std::to_string(amount);

                                              Transaction tx;
                                              tx.id = Crypto::sha256d(from + to + std::to_string(amount) + std::to_string(std::time(nullptr)));
                                              tx.vout.push_back({to, amount});
                                              tx.vout.push_back({from, totalNeeded * -1}); 

                                              // 3. Vincular a Chave Pública Real e Gerar Assinatura Digital ECDSA
                                              tx.publicKey = keys.public_key;
                                              tx.signature = Crypto::sign_data(data_to_sign, keys.private_key);

                                              Storage::saveMempool(tx, MEMPOOL_PATH);
                                              std::cout << "✅ Transação assinada digitalmente e enviada ao Mempool!" << std::endl;
                                          }

                                          void Blockchain::clearChain() { 
                                              chain.clear(); 
                                              totalSupply = 0; 
                                              utxoSet.utxoMap.clear(); 
                                              utxoSet.addressBalances.clear();
                                          }

                                          std::vector<Block> Blockchain::getChain() const { return chain; }
                                          int Blockchain::getDifficulty() const { return difficulty; }
                                          double Blockchain::getTotalSupply() const { return totalSupply; }
                                          double Blockchain::getMaxSupply() const { return 20000000.0; }