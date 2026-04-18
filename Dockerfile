FROM ubuntu:22.04

# Evita prompts interativos durante a instalação
ENV DEBIAN_FRONTEND=noninteractive

# 1. Instalação de dependências essenciais
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    cmake \
    wget \
    libssl-dev \
    libboost-system-dev \
    libboost-thread-dev \
    libcurl4-openssl-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app

# 2. Copia os arquivos do projeto
COPY . .

# 3. Configura diretórios e dependências externas (Crow)
RUN mkdir -p include data
RUN wget https://github.com/CrowCpp/Crow/releases/download/v1.0+5/crow_all.h -O include/crow_all.h

# 4. Compilação
# Note que mantive o nome 'mazechain_api' para o executável
RUN g++ -O3 -std=c++17 \
        src/blockchain.cpp \
        src/block.cpp \
        src/transaction.cpp \
        src/utxo.cpp \
        src/storage.cpp \
        src/wallet.cpp \
        src/crypto.cpp \
        api/main.cpp \
    -Iinclude \
    -I. \
    -lssl -lcrypto -lpthread -lboost_system -lboost_thread -lcurl \
    -o mazechain_api

# 5. Permissões de execução
RUN chmod +x mazechain_api

# 6. Ajuste para a porta 10000
# Isso informa ao Docker/Render qual porta o container escuta
EXPOSE 10000

# 7. Comando de inicialização
# Se o seu main.cpp usar a variável de ambiente PORT, o Crow lerá 10000
CMD ["./mazechain_api"]
