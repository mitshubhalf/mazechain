#ifndef MAZECHAIN_UINT256_H
#define MAZECHAIN_UINT256_H

#include <assert.h>
#include <cstring>
#include <string>
#include <vector>
#include <stdint.h>

/** Classe base para blobs de dados de 256 bits (hashes) */
class uint256 {
protected:
    static constexpr int WIDTH = 32; // 32 bytes = 256 bits
    uint8_t data[WIDTH];

public:
    uint256() { std::memset(data, 0, WIDTH); }

    // Construtor para converter de hex string (usado em testes)
    explicit uint256(const std::string& str);

    uint8_t* begin() { return &data[0]; }
    const uint8_t* begin() const { return &data[0]; }
    uint8_t* end() { return &data[WIDTH]; }
    const uint8_t* end() const { return &data[WIDTH]; }

    bool IsNull() const {
        for (int i = 0; i < WIDTH; i++) if (data[i] != 0) return false;
        return true;
    }

    std::string GetHex() const;
    std::string ToString() const { return GetHex(); }

    inline int Compare(const uint256& other) const { return std::memcmp(data, other.data, WIDTH); }
    friend bool operator==(const uint256& a, const uint256& b) { return a.Compare(b) == 0; }
    friend bool operator!=(const uint256& a, const uint256& b) { return a.Compare(b) != 0; }
    friend bool operator<(const uint256& a, const uint256& b) { return a.Compare(b) < 0; }
};

#endif // MAZECHAIN_UINT256_H