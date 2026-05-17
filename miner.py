#!/usr/bin/env python3
"""
MazeChain External Miner
Mine blocks from any machine with Python 3 and the requests library.

Usage:
    python3 miner.py <NODE_URL> <MINER_ADDRESS>

Example:
    python3 miner.py https://your-node.replit.app MZ1a2b3c4d5e6f...

Requirements:
    pip install requests
"""

import hashlib
import requests
import time
import random
import sys
import json


def sha256d(s: str) -> str:
    b = s.encode("utf-8")
    h1 = hashlib.sha256(b).digest()
    h2 = hashlib.sha256(h1).digest()
    return h2.hex()


def merkle_root(tx_ids: list) -> str:
    if not tx_ids:
        return sha256d("empty_block")
    tree = list(tx_ids)
    while len(tree) > 1:
        if len(tree) % 2 != 0:
            tree.append(tree[-1])
        next_level = []
        for i in range(0, len(tree), 2):
            next_level.append(sha256d(tree[i] + tree[i + 1]))
        tree = next_level
    return tree[0]


def build_hash_string(index, timestamp, prev_hash, merkle, miner_address, extra_nonce, nonce):
    return f"{index}{timestamp}{prev_hash}{merkle}{miner_address}{extra_nonce}{nonce}"


def get_template(node_url: str, miner_addr: str) -> dict:
    resp = requests.get(f"{node_url}/getblocktemplate/{miner_addr}", timeout=10)
    resp.raise_for_status()
    return resp.json()


def submit_block(node_url: str, block: dict) -> dict:
    resp = requests.post(f"{node_url}/block", json=block, timeout=10)
    return resp.json()


def mine(node_url: str, miner_addr: str):
    print("=" * 60)
    print("  MazeChain External Miner")
    print(f"  Node   : {node_url}")
    print(f"  Address: {miner_addr}")
    print("=" * 60)

    blocks_found = 0

    while True:
        try:
            tmpl = get_template(node_url, miner_addr)

            if "error" in tmpl:
                print(f"[ERROR] Node returned: {tmpl['error']}")
                time.sleep(5)
                continue

            index      = tmpl["index"]
            prev_hash  = tmpl["prevHash"]
            difficulty = tmpl["difficulty"]
            target     = "0" * difficulty
            reward     = tmpl["reward"]
            coinbase   = tmpl["coinbase"]

            tx_ids     = [coinbase["id"]]
            extra_nonce = random.randint(1_000_000, 9_999_999)
            timestamp   = int(time.time())

            merkle = merkle_root(tx_ids)

            print(f"\n[MINER] Block #{index} | Diff: {difficulty} | "
                  f"Reward: {reward:.2f} MZ | Target: {target}")

            nonce   = 0
            start   = time.time()
            found   = False
            hash_val = ""

            while True:
                data     = build_hash_string(index, timestamp, prev_hash,
                                              merkle, miner_addr, extra_nonce, nonce)
                hash_val = sha256d(data)

                if hash_val.startswith(target):
                    found = True
                    break

                nonce += 1

                if nonce % 1_000_000 == 0:
                    elapsed = time.time() - start
                    mhs = nonce / elapsed / 1_000_000
                    print(f"  Nonce: {nonce:>12,} | {hash_val[:14]}... | {mhs:.2f} MH/s")

                    if elapsed >= 15:
                        print("  [REFRESH] Fetching new template...")
                        break

                if nonce > 0xFFFFFFFF:
                    extra_nonce += 1
                    nonce = 0
                    timestamp = int(time.time())
                    merkle = merkle_root(tx_ids)

            if not found:
                continue

            elapsed = time.time() - start
            print(f"\n  BLOCK FOUND! Hash: {hash_val}")
            print(f"  Nonce: {nonce} | Time: {elapsed:.1f}s")

            block_data = {
                "index":      index,
                "hash":       hash_val,
                "prevHash":   prev_hash,
                "timestamp":  timestamp,
                "nonce":      nonce,
                "extraNonce": extra_nonce,
                "miner":      miner_addr,
                "txs":        [coinbase],
            }

            result = submit_block(node_url, block_data)
            status = result.get("status", "unknown")

            if status == "accepted":
                blocks_found += 1
                print(f"\n  ACCEPTED! Block #{index} | Reward: {reward:.2f} MZ | "
                      f"Total found: {blocks_found}")
            elif status == "already_have":
                print(f"  Block #{index} already mined by someone else.")
            else:
                print(f"  Rejected: {result}")

        except requests.exceptions.ConnectionError:
            print(f"[ERROR] Cannot connect to {node_url}. Retrying in 5s...")
            time.sleep(5)
        except requests.exceptions.Timeout:
            print("[ERROR] Request timed out. Retrying...")
            time.sleep(3)
        except KeyboardInterrupt:
            print(f"\n\nMiner stopped. Blocks found this session: {blocks_found}")
            sys.exit(0)
        except Exception as e:
            print(f"[ERROR] {e}. Retrying in 3s...")
            time.sleep(3)


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    node_url   = sys.argv[1].rstrip("/")
    miner_addr = sys.argv[2]

    if not miner_addr.startswith("MZ") or len(miner_addr) < 30:
        print(f"[ERROR] Invalid MazeChain address: {miner_addr}")
        print("Address must start with 'MZ' and be at least 30 characters.")
        sys.exit(1)

    mine(node_url, miner_addr)
