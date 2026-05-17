#!/usr/bin/env python3
"""
MazeChain Client — Terminal Externo
Minere, crie carteiras e consulte saldos de qualquer máquina.

USO:
  python3 mazechain_client.py <NODE_URL> <COMANDO> [argumentos]

COMANDOS:
  wallet new                    — Cria uma nova carteira (endereço + seed)
  balance <address>             — Consulta o saldo de um endereço
  mine <address>                — Minera UM bloco e para
  mine looping <address>        — Minera em loop contínuo (Ctrl+C para parar)
  sync                          — Mostra status atual da rede e últimos blocos
  checkpoints                   — Lista todos os pontos seguros da chain

EXEMPLOS:
  python3 mazechain_client.py https://meu-no.replit.app wallet new
  python3 mazechain_client.py https://meu-no.replit.app mine MZabc123...
  python3 mazechain_client.py https://meu-no.replit.app mine looping MZabc123...
  python3 mazechain_client.py https://meu-no.replit.app balance MZabc123...
  python3 mazechain_client.py https://meu-no.replit.app sync

REQUISITOS:
  pip install requests
"""

import hashlib
import requests
import time
import random
import sys
import json

BANNER = """
╔══════════════════════════════════════════════╗
║          MAZECHAIN EXTERNAL CLIENT           ║
║     Bitcoin-style SHA256d Proof of Work      ║
╚══════════════════════════════════════════════╝
"""


# ── HASHING (igual ao Bitcoin: SHA256d) ─────────────────────────────────────

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


# ── HTTP ────────────────────────────────────────────────────────────────────

def get(node_url: str, path: str) -> dict:
    r = requests.get(f"{node_url}{path}", timeout=15)
    r.raise_for_status()
    return r.json()


def post(node_url: str, path: str, data: dict) -> dict:
    r = requests.post(f"{node_url}{path}", json=data, timeout=15)
    return r.json()


# ── COMANDOS ────────────────────────────────────────────────────────────────

def cmd_wallet_new(node_url: str):
    print("\n⏳ Gerando nova carteira no nó...")
    try:
        d = get(node_url, "/wallet/new")
        print("\n" + "═" * 50)
        print("  NOVA CARTEIRA MAZECHAIN GERADA")
        print("═" * 50)
        print(f"  Endereço : {d['address']}")
        print(f"  Seed     : {d['seed']}")
        print("═" * 50)
        print("  ⚠️  Guarde sua Seed em lugar seguro!")
        print("     Nunca a compartilhe com ninguém.\n")
    except Exception as e:
        print(f"[ERRO] {e}")


def cmd_balance(node_url: str, address: str):
    if not address.startswith("MZ"):
        print("[ERRO] Endereço inválido. Deve começar com MZ.")
        return
    try:
        d = get(node_url, f"/balance/{address}")
        print(f"\n  Endereço : {address}")
        print(f"  Saldo    : {d['balance_mz']:.8f} MZ")
        print(f"           = {int(d['balance_mz'] * 1e8)} Mits\n")
    except Exception as e:
        print(f"[ERRO] {e}")


def cmd_sync(node_url: str):
    try:
        d = get(node_url, "/status")
        chain = d.get("chain", {})
        proto = d.get("protocol", {})
        net   = d.get("network", {})
        print(f"\n  ⛓  Altura      : {chain.get('height', '?')}")
        print(f"  🔒 Último Hash : {chain.get('last_hash', '?')[:40]}...")
        print(f"  ⚡ Dificuldade : {proto.get('difficulty', '?')}")
        print(f"  💰 Supply      : {proto.get('total_supply', 0):.2f} MZ")
        print(f"  🌐 Peers       : {net.get('peers_count', 0)}")
        print()

        print("  Últimos blocos:")
        blocks = get(node_url, "/chain").get("blocks", [])[:10]
        for b in blocks:
            ts = time.strftime('%d/%m %H:%M:%S', time.localtime(b['timestamp']))
            miner = b.get('miner', '?')
            miner_short = miner[:20] + "..." if len(miner) > 20 else miner
            print(f"  #{b['index']:>6}  {b['hash'][:18]}...  {miner_short}  [{ts}]")
        print()
    except Exception as e:
        print(f"[ERRO] {e}")


def cmd_checkpoints(node_url: str):
    try:
        d = get(node_url, "/status")
        cps = d.get("checkpoints", [])
        if not cps:
            print("\n  Nenhum checkpoint além do gênesis registrado ainda.\n")
            return
        print("\n  CHECKPOINTS MAZECHAIN:")
        for cp in cps:
            print(f"  Bloco #{cp['height']:>6} → {cp['hash'][:40]}...")
        print()
    except Exception as e:
        print(f"[ERRO ao buscar checkpoints: {e}]")


# ── MINERAÇÃO ────────────────────────────────────────────────────────────────

def get_template(node_url: str, miner_addr: str) -> dict:
    return get(node_url, f"/getblocktemplate/{miner_addr}")


def submit_block(node_url: str, block: dict) -> dict:
    return post(node_url, "/block", block)


def mine_one(node_url: str, miner_addr: str) -> bool:
    """Minera um único bloco. Retorna True se aceito."""
    try:
        tmpl = get_template(node_url, miner_addr)
        if "error" in tmpl:
            print(f"[ERRO] {tmpl['error']}")
            return False

        index      = tmpl["index"]
        prev_hash  = tmpl["prevHash"]
        difficulty = tmpl["difficulty"]
        target     = "0" * difficulty
        reward     = tmpl["reward"]
        coinbase   = tmpl["coinbase"]

        extra_nonce = random.randint(1_000_000, 9_999_999)
        timestamp   = int(time.time())
        tx_ids      = [coinbase["id"]]
        merkle      = merkle_root(tx_ids)

        print(f"\n  ⛏  Bloco #{index} | Dificuldade: {difficulty} | Recompensa: {reward:.2f} MZ")
        print(f"  Target: {target}...")

        nonce   = 0
        start   = time.time()
        hash_val = ""

        while True:
            data     = build_hash_string(index, timestamp, prev_hash, merkle, miner_addr, extra_nonce, nonce)
            hash_val = sha256d(data)

            if hash_val.startswith(target):
                break

            nonce += 1

            if nonce % 500_000 == 0:
                elapsed = time.time() - start
                mhs = nonce / elapsed / 1_000_000 if elapsed > 0 else 0
                print(f"  Nonce: {nonce:>12,} | {hash_val[:16]}... | {mhs:.2f} MH/s", end="\r")

                # Atualiza template a cada 30s para pegar novo prev_hash se alguém minerou
                if elapsed >= 30:
                    print("\n  [REFRESH] Buscando novo template...")
                    return mine_one(node_url, miner_addr)

            if nonce > 0xFFFFFFFF:
                extra_nonce += 1
                nonce = 0
                timestamp = int(time.time())

        elapsed = time.time() - start
        print(f"\n\n  ✅ BLOCO ENCONTRADO!")
        print(f"  Hash   : {hash_val}")
        print(f"  Nonce  : {nonce:,}")
        print(f"  Tempo  : {elapsed:.1f}s")

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
            print(f"  🎯 ACEITO! +{reward:.2f} MZ para {miner_addr[:20]}...")
            return True
        elif status == "already_have":
            print(f"  ⚠  Bloco #{index} já foi minerado por outro nó.")
            return False
        else:
            print(f"  ❌ Rejeitado: {result}")
            return False

    except requests.exceptions.ConnectionError:
        print(f"[ERRO] Não foi possível conectar em {node_url}")
        return False
    except Exception as e:
        print(f"[ERRO] {e}")
        return False


def cmd_mine_single(node_url: str, miner_addr: str):
    print(f"\n  Minerando um bloco para: {miner_addr}\n")
    mine_one(node_url, miner_addr)
    print()


def cmd_mine_looping(node_url: str, miner_addr: str):
    print(f"\n  Mineração em loop para: {miner_addr}")
    print("  Pressione Ctrl+C para parar.\n")
    blocks_found = 0
    try:
        while True:
            success = mine_one(node_url, miner_addr)
            if success:
                blocks_found += 1
                print(f"  Total minerado nesta sessão: {blocks_found} bloco(s)\n")
            time.sleep(0.1)
    except KeyboardInterrupt:
        print(f"\n\n  Mineração encerrada. Blocos encontrados: {blocks_found}\n")


# ── MAIN ─────────────────────────────────────────────────────────────────────

def main():
    if len(sys.argv) < 3:
        print(__doc__)
        sys.exit(1)

    print(BANNER)

    node_url = sys.argv[1].rstrip("/")
    cmd      = sys.argv[2].lower()
    args     = sys.argv[3:]

    print(f"  Nó: {node_url}\n")

    if cmd == "wallet" and args and args[0] == "new":
        cmd_wallet_new(node_url)

    elif cmd == "balance":
        if not args:
            print("[ERRO] Uso: balance <address>")
            sys.exit(1)
        cmd_balance(node_url, args[0])

    elif cmd == "sync":
        cmd_sync(node_url)

    elif cmd == "checkpoints":
        cmd_checkpoints(node_url)

    elif cmd == "mine":
        if not args:
            print("[ERRO] Uso: mine <address>  ou  mine looping <address>")
            sys.exit(1)

        if args[0] == "looping":
            if len(args) < 2:
                print("[ERRO] Uso: mine looping <address>")
                sys.exit(1)
            addr = args[1]
            if not addr.startswith("MZ") or len(addr) < 30:
                print(f"[ERRO] Endereço inválido: {addr}")
                sys.exit(1)
            cmd_mine_looping(node_url, addr)
        else:
            addr = args[0]
            if not addr.startswith("MZ") or len(addr) < 30:
                print(f"[ERRO] Endereço inválido: {addr}")
                sys.exit(1)
            cmd_mine_single(node_url, addr)

    else:
        print(f"[ERRO] Comando desconhecido: '{cmd}'")
        print(__doc__)
        sys.exit(1)


if __name__ == "__main__":
    main()
