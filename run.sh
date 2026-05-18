#!/bin/bash
set -e

# Configurações de Portas
export BACKEND_PORT=10000
export FRONTEND_PORT=5000

# 1. LIMPEZA TOTAL
echo "[CLEANUP] Encerrando processos anteriores..."
pkill -f mazechain_api 2>/dev/null || true
fuser -k $BACKEND_PORT/tcp 2>/dev/null || true
sleep 2

# 2. PREPARAÇÃO
mkdir -p data

# 3. COMPILAÇÃO DO BACKEND (C++)
echo "[BUILD] Compilando Backend MazeChain..."
# Detecta caminhos das bibliotecas no ambiente Nix/Replit
ASIO_INC=$(pkg-config --cflags asio 2>/dev/null || echo "-I/usr/include")
OPENSSL_FLAGS=$(pkg-config --cflags --libs openssl 2>/dev/null || echo "-lssl -lcrypto")

CURL_FLAGS=$(pkg-config --libs libcurl 2>/dev/null || echo "-lcurl")

g++ -O2 -std=c++17 \
  src/blockchain.cpp \
  src/block.cpp \
  src/transaction.cpp \
  src/utxo.cpp \
  src/storage.cpp \
  src/wallet.cpp \
  src/crypto.cpp \
  src/sha.cpp \
  src/checkpoints.cpp \
  src/db_integrity.cpp \
  src/p2p.cpp \
  src/node_manager.cpp \
  api/main.cpp \
  -Iinclude -Isrc \
  $ASIO_INC \
  $OPENSSL_FLAGS \
  $CURL_FLAGS \
  -lpthread \
  -Wno-deprecated-declarations \
  -o mazechain_api
  # Módulos: mempool_expiry (header-only), hd_wallet (header-only),
  #          multisig (header-only), testnet (header-only)

echo "[BUILD] Compilação finalizada."

# 4. SCRIPTS DE ACESSO — mazechain (CLI) e mazechain_node (nó completo)
chmod +x mazechain mazechain_node 2>/dev/null || true

# 5. INICIALIZAÇÃO DO NÓ MAZECHAIN (Porta 10000 - serve API + index.html)
echo "[START] Iniciando Nó MazeChain em http://localhost:$BACKEND_PORT"
export PORT=$BACKEND_PORT
./mazechain_api