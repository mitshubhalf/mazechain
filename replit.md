# MazeChain

A lightweight blockchain implementation in C++ featuring a full ecosystem with wallets, mining (Proof of Work), and a REST API.

## Overview

MazeChain is a functional blockchain node with:
- **Wallets**: ECDSA-based address creation with BIP-39 style seeds
- **Transactions**: Signature verification and balance checks with 1% fee
- **Mining**: Proof of Work with automatic halving and 20M max supply
- **Storage**: Persistence to disk (blockchain.dat, mempool.dat)
- **REST API**: Crow-based HTTP server
- **Web UI**: `index.html` served at the root

## Tech Stack

- **Language**: C++17
- **Web Framework**: [Crow](https://crowcpp.org/) v1.1.0 (header-only)
- **Cryptography**: OpenSSL (ECDSA, SHA-256)
- **Async I/O**: Standalone Asio 1.24.0
- **Build**: g++ with pkg-config

## Project Structure

```
api/main.cpp          - REST API routes and server entry point
include/              - Headers (blockchain.h, wallet.h, crypto.h, etc.)
  crow_all.h          - Crow framework (downloaded at setup)
src/                  - Implementation files
  blockchain.cpp
  block.cpp
  transaction.cpp
  utxo.cpp
  storage.cpp
  wallet.cpp
  crypto.cpp
data/                 - Runtime data (blockchain.dat, mempool.dat)
index.html            - Web dashboard/explorer frontend
run.sh                - Development run script (skips recompile if binary exists)
build.sh              - Build-only script (for deployment)
mazechain_api         - Compiled binary
```

## API Endpoints

- `GET /` - Serves index.html dashboard
- `GET /status` - Node status (version, difficulty, supply, blocks)
- `GET /chain` - Block explorer
- `GET /balance/<address>` - Balance in MZ and Mits
- `GET /wallet/new` - Create a new wallet
- `POST /send` - Send a transaction (from, to, amount, seed)
- `GET /minerar_agora/<address>` - Mine a new block

## Running

The app compiles and runs via `run.sh`. The workflow is configured to use port **5000**.

```bash
bash run.sh
```

## System Dependencies

Installed via Nix:
- `openssl` - Cryptography
- `boost` - Header libraries (Crow dependency)
- `asio` - Standalone async I/O
- `pkg-config` - Package config
- `gcc` - C++ compiler

## Deployment

Build command: `bash build.sh`
Run command: `mkdir -p data && PORT=5000 ./mazechain_api`
Target: autoscale
