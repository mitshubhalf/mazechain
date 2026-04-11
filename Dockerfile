FROM ubuntu:22.04

# Instala o compilador, OpenSSL e a biblioteca Boost (necessária para o Crow)
RUN apt-get update && apt-get install -y \
    g++ \
    make \
    wget \
    libssl-dev \
    libboost-all-dev \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /app
COPY . .

# Garante que a pasta include existe e baixa o Crow
RUN mkdir -p include
RUN wget https://github.com/CrowCpp/Crow/releases/download/v1.0+5/crow_all.h -O include/crow_all.h

# Compila a sua API
RUN g++ src/blockchain.cpp src/storage.cpp src/wallet.cpp api/main.cpp -Iinclude -lssl -lcrypto -lpthread -o mazechain_api

# Define a porta do Render
ENV PORT=10000
EXPOSE 10000

# Comando para rodar a API
CMD ["./mazechain_api"]
