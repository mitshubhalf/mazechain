#!/bin/bash
echo "------------------------------------------"
echo "🔨 COMPILANDO MAZECHAIN CORE v3.0 P2P..."
echo "------------------------------------------"

# Remove o binário antigo
rm -f mazechain_api

# Compilação completa incluindo os novos módulos (sha.cpp, checkpoints.cpp, etc)
# Adicionamos -I para os headers e os arquivos que causavam erro de Linker
g++ -O3 -I./include -I./src -I./api \
api/main.cpp \
src/blockchain.cpp \
src/block.cpp \
src/transaction.cpp \
src/crypto.cpp \
src/utxo.cpp \
src/storage.cpp \
src/node_manager.cpp \
src/p2p.cpp \
src/wallet.cpp \
src/checkpoints.cpp \
src/db_integrity.cpp \
src/mempool_limit.cpp \
src/sha.cpp \
-o mazechain_api -lssl -lcrypto -lpthread -lcurl -Wno-deprecated-declarations

# Verifica se deu certo
if [ $? -eq 0 ]; then
    echo "✅ COMPILAÇÃO CONCLUÍDA COM SUCESSO!"
    chmod +x mazechain_api
else
    echo "❌ ERRO NA COMPILAÇÃO! Verifique os logs acima."
    exit 1
fi