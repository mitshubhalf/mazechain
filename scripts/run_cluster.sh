#!/usr/bin/env bash
# MazeChain — Launch a local cluster of N mining nodes
# Each node gets its own port, wallet, data dir
# They all peer with node 0 (seed node) — mesh propagates from there
#
# Usage: bash scripts/run_cluster.sh [num_nodes]
# Default: 3 nodes on ports 10000, 10100, 10200

NUM_NODES="${1:-3}"
BASE_PORT=10000
STEP=100
BINARY="./mazechain_api"
LOG_DIR="data/cluster_logs"

mkdir -p "${LOG_DIR}"

# Build if needed
if [ ! -f "${BINARY}" ]; then
  echo "⚙️  Compilando MazeChain..."
  bash build.sh
fi

PIDS=()

cleanup() {
  echo ""
  echo "🛑 Encerrando todos os nós do cluster..."
  for pid in "${PIDS[@]}"; do
    kill "${pid}" 2>/dev/null
  done
  echo "✅ Cluster encerrado."
  exit 0
}
trap cleanup SIGINT SIGTERM

echo "╔══════════════════════════════════════════════════════╗"
echo "║          MAZECHAIN CLUSTER — ${NUM_NODES} NÓS                    ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

SEED_PORT="${BASE_PORT}"
SEED_URL="http://localhost:${SEED_PORT}"

for i in $(seq 0 $((NUM_NODES - 1))); do
  PORT=$((BASE_PORT + i * STEP))
  DATA_DIR="data/node_${PORT}"
  mkdir -p "${DATA_DIR}"

  # Generate a unique wallet for this node
  WALLET_FILE="${DATA_DIR}/wallet.dat"

  # Peers file: all non-seed nodes point at seed node
  PEERS_FILE="${DATA_DIR}/peers.dat"
  if [ "${i}" -gt 0 ]; then
    echo "${SEED_URL}" > "${PEERS_FILE}"
    # Also peer with previous node for mesh
    PREV_PORT=$((PORT - STEP))
    echo "http://localhost:${PREV_PORT}" >> "${PEERS_FILE}"
  else
    # Seed node: knows about all others
    > "${PEERS_FILE}"
    for j in $(seq 1 $((NUM_NODES - 1))); do
      echo "http://localhost:$((BASE_PORT + j * STEP))" >> "${PEERS_FILE}"
    done
  fi

  LOG_FILE="${LOG_DIR}/node_${PORT}.log"

  echo "▶ Nó #${i} porta ${PORT} | data: ${DATA_DIR}"

  PORT="${PORT}" \
  MAZE_DATADIR="${DATA_DIR}" \
  MAZE_PEERS_FILE="${PEERS_FILE}" \
  MAZE_WALLET_FILE="${WALLET_FILE}" \
    "${BINARY}" > "${LOG_FILE}" 2>&1 &

  PIDS+=($!)
  echo "  PID: ${PIDS[$i]} | log: ${LOG_FILE}"
  sleep 0.5   # stagger starts so seed is ready first
done

echo ""
echo "╔══════════════════════════════════════════════════════╗"
echo "║  Cluster iniciado! Nós rodando:                     ║"
for i in $(seq 0 $((NUM_NODES - 1))); do
  PORT=$((BASE_PORT + i * STEP))
  echo "║    Nó #${i}  →  http://localhost:${PORT}              ║"
done
echo "║                                                      ║"
echo "║  Pressione Ctrl+C para encerrar todos os nós        ║"
echo "╚══════════════════════════════════════════════════════╝"
echo ""

# Tail all logs merged
tail -f "${LOG_DIR}"/node_*.log &
TAIL_PID=$!

# Wait for any node to die
wait "${PIDS[@]}"
kill "${TAIL_PID}" 2>/dev/null
