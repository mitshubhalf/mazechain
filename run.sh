#!/bin/bash
set -e

# Configurações de Portas
export BACKEND_PORT=10000
export FRONTEND_PORT=5000

# 1. LIMPEZA TOTAL
echo "[CLEANUP] Liberando portas $BACKEND_PORT e $FRONTEND_PORT..."
fuser -k $BACKEND_PORT/tcp 2>/dev/null || true
fuser -k $FRONTEND_PORT/tcp 2>/dev/null || true
sleep 1

# 2. PREPARAÇÃO
mkdir -p data

# 3. COMPILAÇÃO DO BACKEND (C++)
echo "[BUILD] Compilando Backend MazeChain..."
# Detecta caminhos das bibliotecas no ambiente Nix/Replit
ASIO_INC=$(pkg-config --cflags asio 2>/dev/null || echo "-I/usr/include")
OPENSSL_FLAGS=$(pkg-config --cflags --libs openssl 2>/dev/null || echo "-lssl -lcrypto")

g++ -O2 -std=c++17 \
  src/blockchain.cpp \
  src/block.cpp \
  src/transaction.cpp \
  src/utxo.cpp \
  src/storage.cpp \
  src/wallet.cpp \
  src/crypto.cpp \
  src/p2p.cpp \
  src/node_manager.cpp \
  api/main.cpp \
  -Iinclude \
  $ASIO_INC \
  $OPENSSL_FLAGS \
  -lpthread \
  -lcurl \
  -Wno-deprecated-declarations \
  -o mazechain_api

echo "[BUILD] Compilação finalizada."

# 4. INICIALIZAÇÃO DO FRONTEND (Porta 5000)
# Serve os arquivos da pasta atual (onde deve estar seu index.html)
echo "[START] Iniciando Interface Web em http://localhost:$FRONTEND_PORT"
python3 -m http.server $FRONTEND_PORT > /dev/null 2>&1 &

# 5. INICIALIZAÇÃO DO BACKEND (Porta 10000)
echo "[START] Iniciando Nó MazeChain em http://localhost:$BACKEND_PORT"
# O backend assume o controle do terminal para você ver os logs em tempo real
export PORT=$BACKEND_PORT
./mazechain_api