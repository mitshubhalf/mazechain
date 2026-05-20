#!/usr/bin/env bash
# MazeChain — Single node launcher
# Usage: bash scripts/run_node.sh <port> <miner_addr> [peer_url]
# Example: bash scripts/run_node.sh 10001 MZabc123 http://localhost:10000

PORT="${1:-10000}"
MINER_ADDR="${2:-}"
PEER_URL="${3:-}"
DATA_DIR="data/node_${PORT}"
BINARY="./mazechain_api"

mkdir -p "${DATA_DIR}"

# Write per-node config
cat > "${DATA_DIR}/mazechain.conf" <<EOF
[network]
port=${PORT}
maxpeers=32
p2pport=$((PORT + 1000))

[chain]
testnet=false
segwit=true
taproot=true

[storage]
datadir=${DATA_DIR}
txindex=true

[logging]
loglevel=INFO
logfile=${DATA_DIR}/mazechain.log

[wallet]
walletfile=${DATA_DIR}/wallet.dat
EOF

# Seed peer file if a peer was given
if [ -n "${PEER_URL}" ]; then
  echo "${PEER_URL}" > "${DATA_DIR}/peers.dat"
  echo "🌐 [NODE:${PORT}] Peer configurado: ${PEER_URL}"
fi

echo "🚀 [NODE:${PORT}] Iniciando nó MazeChain na porta ${PORT}"
echo "   Data dir : ${DATA_DIR}"
echo "   Minerador: ${MINER_ADDR:-<automático>}"

PORT="${PORT}" \
MAZE_DATADIR="${DATA_DIR}" \
MAZE_PEERS_FILE="${DATA_DIR}/peers.dat" \
MAZE_WALLET_FILE="${DATA_DIR}/wallet.dat" \
MAZE_MINEADDR="${MINER_ADDR}" \
  "${BINARY}"
