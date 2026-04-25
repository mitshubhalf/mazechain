#!/bin/bash
echo "------------------------------------------"
echo "🔨 COMPILANDO MAZECHAIN CORE v3.0 P2P..."
echo "------------------------------------------"

# Remove o binário antigo para garantir uma compilação nova
rm -f mazechain_api

# Compilação completa com todas as bibliotecas necessárias
g++ -O3 -x c++ src/main.cpp.old \
src/blockchain.cpp \
src/block.cpp \
src/transaction.cpp \
src/crypto.cpp \
src/utxo.cpp \
src/storage.cpp \
src/node_manager.cpp \
src/p2p.cpp \
src/wallet.cpp \
-o mazechain_api -lssl -lcrypto -lpthread -lcurl

# Verifica se deu certo
if [ $? -eq 0 ]; then
    echo "✅ COMPILAÇÃO CONCLUÍDA COM SUCESSO!"
    # Dá permissão de execução ao binário gerado
    chmod +x mazechain_api
else
    echo "❌ ERRO NA COMPILAÇÃO! Verifique os logs acima."
    exit 1
fi