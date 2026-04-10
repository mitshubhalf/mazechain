#include "../include/blockchain.h"
#include "../include/storage.h"
#include <iostream>
#include <chrono>

Blockchain::Blockchain() {
    difficulty = 5;
    totalSupply = 0;
}

Block Blockchain::getLastBlock() {
    if (chain.empty()) return Block(-1, "0", {});
    return chain.back();
}

double Blockchain::getBlockReward(int height) {
    if (totalSupply >= 20000000) return 0;
    double reward = 250.0;
    int halvings = height / 1000;
    for (int i = 0; i < halvings; i++) reward /= 2.0;
    return reward;
}

void Blockchain::adjustDifficulty() {
    if (chain.size() < DIFFICULTY_ADJUSTMENT_INTERVAL) return;
    const Block& lastBlock = chain.back();
    const Block& relayBlock = chain[chain.size() - DIFFICULTY_ADJUSTMENT_INTERVAL];
    long timeExpected = TARGET_BLOCK_TIME * DIFFICULTY_ADJUSTMENT_INTERVAL;
    long timeTaken = lastBlock.timestamp - relayBlock.timestamp;
    if (timeTaken < 1) timeTaken = 1;

    std::cout << "\n📊 --- AJUSTE DE DIFICULDADE ---" << std::endl;
    if (timeTaken < timeExpected / 2) {
        difficulty++;
        std::cout << "🔥 Rede rápida! Nova Dificuldade: " << difficulty << std::endl;
    } else if (timeTaken > timeExpected * 2) {
        if (difficulty > 1) difficulty--;
        std::cout << "🧊 Rede lenta! Nova Dificuldade: " << difficulty << std::endl;
    } else {
        std::cout << "✅ Estável. Mantendo Diff: " << difficulty << std::endl;
    }
    std::cout << "--------------------------------\n" << std::endl;
}

void Blockchain::mineBlock(std::string minerAddress) {
    if (chain.empty()) {
        Block genesis(0, "0", {});
        genesis.mine(difficulty);
        chain.push_back(genesis);
    }
    if (chain.size() % DIFFICULTY_ADJUSTMENT_INTERVAL == 0) adjustDifficulty();

    double reward = getBlockReward(chain.size());
    Transaction coinbase({}, { {minerAddress, reward} });
    Block newBlock(chain.size(), getLastBlock().hash, {coinbase});

    std::cout << "⛏️ Minando bloco " << newBlock.index << " (Diff: " << difficulty << ")..." << std::endl;
    auto start = std::chrono::high_resolution_clock::now();
    newBlock.mine(difficulty);
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double> diff = end - start;
    std::cout << "🚀 Velocidade: " << (newBlock.nonce / diff.count()) / 1000 << " KH/s" << std::endl;

    chain.push_back(newBlock);
    totalSupply += reward;
    Storage::saveChain(*this, "data/blockchain.dat");
}

// --- FUNÇÃO CORRIGIDA: SALDO REAL (ENTRADAS - SAÍDAS) ---
double Blockchain::getBalance(std::string address) {
    double balance = 0;
    for (const auto &block : chain) {
        for (const auto &tx : block.transactions) {
            // Verifica quem recebeu moedas nesta transação
            for (const auto &out : tx.vout) {
                if (out.address == address) {
                    balance += out.amount;
                }
            }
            // Verifica se o endereço enviou moedas (analisando os inputs das transações que não são coinbase)
            // No nosso sistema simplificado, o endereço de envio fica registrado no 'troco' ou podemos identificar pelo processo.
            // Para ser preciso agora: se a transação tem mais de um output e o segundo output diminuiu o saldo,
            // a lógica de 'send' abaixo já cuida de criar um output de valor reduzido para o remetente.
        }
    }
    return balance;
}

void Blockchain::send(std::string from, std::string to, double amount) {
    double fee = amount * 0.01;
    double currentBalance = getBalance(from);

    if (currentBalance < (amount + fee)) {
        std::cout << "❌ Saldo insuficiente. Saldo atual: " << currentBalance << " | Necessário: " << (amount + fee) << "\n";
        return;
    }

    // A mágica do saldo corrigido:
    // Criamos a transação com o valor para o destino E o valor que SOBROU para o 'from'
    // Mas para o saldo não duplicar, precisamos que o sistema entenda que o valor antigo "sumiu".
    // Como não temos UTXO real ainda, vamos simular o gasto limpando o saldo anterior no cálculo:
    
    // IMPORTANTE: Para este MVP, a lógica de saldo baseia-se em varredura. 
    // Vamos criar uma transação que debita o valor total do 'from' enviando o que sobrou de volta.
    Transaction tx({}, { {to, amount}, {from, (amount + fee) * -1} }); // O valor negativo sinaliza a saída

    double rewardWithFee = getBlockReward(chain.size()) + fee;
    Transaction coinbase({}, { {"miner1", rewardWithFee} });

    Block newBlock(chain.size(), getLastBlock().hash, {tx, coinbase});
    newBlock.mine(difficulty);
    chain.push_back(newBlock);
    
    Storage::saveChain(*this, "data/blockchain.dat");
    std::cout << "✅ Transferência de " << amount << " confirmada! Taxa: " << fee << "\n";
}

// Getters e Setters continuam iguais...
std::vector<Block> Blockchain::getChain() const { return chain; }
int Blockchain::getDifficulty() const { return difficulty; }
void Blockchain::setDifficulty(int d) { difficulty = d; }
void Blockchain::clearChain() { chain.clear(); totalSupply = 0; }
void Blockchain::addBlock(const Block& block) {
    chain.push_back(block);
    for(const auto& tx : block.transactions)
        for(const auto& out : tx.vout) totalSupply += out.amount;
}
