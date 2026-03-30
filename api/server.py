from fastapi import FastAPI
import subprocess
import json

app = FastAPI()

# Função pra rodar seu C++
def run_cmd(cmd):
    result = subprocess.run(cmd, shell=True, capture_output=True, text=True)
    return result.stdout

# 📜 GET BLOCKCHAIN
@app.get("/chain")
def get_chain():
    output = run_cmd("./mazechain chain")

    try:
        return json.loads(output)
    except:
        return {"raw": output}

# ⛏️ MINERAR BLOCO
@app.post("/mine")
def mine():
    output = run_cmd("./mazechain mine")
    return {"result": output}

# 💸 ENVIAR TRANSAÇÃO
@app.post("/send")
def send(to: str, amount: int):
    cmd = f"./mazechain send {to} {amount}"
    output = run_cmd(cmd)
    return {"result": output}
