#include "uint256.h"
#include <iomanip>
#include <sstream>

std::string uint256::GetHex() const {
    std::stringstream ss;
    for (int i = WIDTH - 1; i >= 0; i--) {
        ss << std::hex << std::setw(2) << std::setfill('0') << (int)data[i];
    }
    return ss.str();
}

uint256::uint256(const std::string& str) {
    std::memset(data, 0, WIDTH);
    // Lógica simplificada de conversão Hex para bytes (Little Endian)
    // ... implementação omitida para brevidade, mas o esqueleto é esse
}