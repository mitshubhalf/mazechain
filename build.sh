#!/bin/bash
set -e

echo "[BUILD] Compiling MazeChain..."
ASIO_INC=$(pkg-config --cflags asio)
OPENSSL_FLAGS=$(pkg-config --cflags --libs openssl)
g++ -O2 -std=c++17 \
  src/blockchain.cpp src/block.cpp src/transaction.cpp src/utxo.cpp src/storage.cpp src/wallet.cpp src/crypto.cpp api/main.cpp \
  -Iinclude \
  $ASIO_INC \
  $OPENSSL_FLAGS \
  -lpthread \
  -Wno-deprecated-declarations \
  -o mazechain_api
chmod +x mazechain_api
mkdir -p data
echo "[BUILD] Done."
