#ifndef MAZECHAIN_ARITH_UINT256_H
#define MAZECHAIN_ARITH_UINT256_H

#include <cstdint>
#include <cstring>
#include <limits>
#include <stdexcept>
#include <string>
#include "uint256.h"

class uint_error : public std::runtime_error {
public:
    explicit uint_error(const std::string& str) : std::runtime_error(str) {}
};

template <unsigned int BITS>
class base_uint
{
protected:
    static constexpr int WIDTH = BITS / 32;
    uint32_t pn[WIDTH];

public:
    // Construtor padrão inicializando o array (resolve o erro de constexpr)
    constexpr base_uint() : pn() {
        for (int i = 0; i < WIDTH; i++) pn[i] = 0;
    }

    base_uint(const base_uint& b) {
        std::memcpy(pn, b.pn, sizeof(pn));
    }

    base_uint& operator=(const base_uint& b) {
        std::memcpy(pn, b.pn, sizeof(pn));
        return *this;
    }

    // Construtor para uint64_t (necessário para o arith_uint256 compilar)
    base_uint(uint64_t b) : pn() {
        pn[0] = (uint32_t)b;
        pn[1] = (uint32_t)(b >> 32);
        for (int i = 2; i < WIDTH; i++) pn[i] = 0;
    }

    // Declarações necessárias para o arith_uint256.cpp encontrar
    base_uint& operator<<=(unsigned int shift);
    base_uint& operator>>=(unsigned int shift);
    base_uint& operator*=(uint32_t b32);
    base_uint& operator*=(const base_uint& b);
    base_uint& operator/=(const base_uint& b);

    base_uint& operator+=(const base_uint& b) {
        uint64_t carry = 0;
        for (int i = 0; i < WIDTH; i++) {
            uint64_t n = carry + pn[i] + b.pn[i];
            pn[i] = n & 0xffffffff;
            carry = n >> 32;
        }
        return *this;
    }

    base_uint& operator-=(const base_uint& b) {
        base_uint inv = ~b;
        inv += 1;
        *this += inv;
        return *this;
    }

    base_uint operator~() const {
        base_uint ret;
        for (int i = 0; i < WIDTH; i++) ret.pn[i] = ~pn[i];
        return ret;
    }

    uint64_t GetLow64() const {
        return pn[0] | (uint64_t)pn[1] << 32;
    }

    unsigned int bits() const;
    int CompareTo(const base_uint& b) const;
    bool EqualTo(uint64_t b) const;

    // Operadores de comparação (C++17)
    friend bool operator==(const base_uint& a, const base_uint& b) { return std::memcmp(a.pn, b.pn, sizeof(a.pn)) == 0; }
    friend bool operator!=(const base_uint& a, const base_uint& b) { return !(a == b); }
    friend bool operator<(const base_uint& a, const base_uint& b)  { return a.CompareTo(b) < 0; }
    friend bool operator<=(const base_uint& a, const base_uint& b) { return a.CompareTo(b) <= 0; }
    friend bool operator>(const base_uint& a, const base_uint& b)  { return a.CompareTo(b) > 0; }
    friend bool operator>=(const base_uint& a, const base_uint& b) { return a.CompareTo(b) >= 0; }

    // Operadores aritméticos binários
    friend base_uint operator+(base_uint a, const base_uint& b) { return a += b; }
    friend base_uint operator-(base_uint a, const base_uint& b) { return a -= b; }
    friend base_uint operator*(base_uint a, const base_uint& b) { return a *= b; }
    friend base_uint operator/(base_uint a, const base_uint& b) { return a /= b; }
    friend base_uint operator<<(base_uint a, unsigned int shift) { return a <<= shift; }
    friend base_uint operator>>(base_uint a, unsigned int shift) { return a >>= shift; }
};

class arith_uint256 : public base_uint<256>
{
public:
    constexpr arith_uint256() : base_uint<256>() {}
    arith_uint256(const base_uint<256>& b) : base_uint<256>(b) {}
    arith_uint256(uint64_t b) : base_uint<256>(b) {}

    arith_uint256& SetCompact(uint32_t nCompact, bool *pfNegative = nullptr, bool *pfOverflow = nullptr);
    uint32_t GetCompact(bool fNegative = false) const;

    friend uint256 ArithToUint256(const arith_uint256 &);
    friend arith_uint256 UintToArith256(const uint256 &);
};

uint256 ArithToUint256(const arith_uint256 &);
arith_uint256 UintToArith256(const uint256 &);

#endif