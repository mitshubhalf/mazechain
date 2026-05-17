#ifndef MAZECHAIN_COMPRESSOR_H
#define MAZECHAIN_COMPRESSOR_H

// Ajustando os caminhos para a estrutura da sua MazeChain
#include "serialize.h"
#include "uint256.h"
#include "prevector.h" // Se você não tiver este arquivo, mude para std::vector
#include <vector>
#include <span.h>

/**
 * O MazeChain usa scripts comprimidos para economizar espaço no banco de dados (UTXO set).
 * scripts de P2PKH e P2SH são reduzidos de 23-25 bytes para 21 bytes.
 */
using CompressedScript = std::vector<unsigned char>;

// Declarações das funções que serão implementadas no .cpp
bool CompressScript(const std::vector<unsigned char>& script, CompressedScript& out);
unsigned int GetSpecialScriptSize(unsigned int nSize);
bool DecompressScript(std::vector<unsigned char>& script, unsigned int nSize, const CompressedScript& in);

/**
 * Comprime valores (CAmount). 
 * Valores pequenos ou com muitos zeros no final ocupam menos bytes via VARINT.
 */
uint64_t CompressAmount(uint64_t nAmount);
uint64_t DecompressAmount(uint64_t nAmount);

/** * Serializador compacto para Scripts.
 * Detecta casos comuns: P2PKH, P2SH e chaves públicas comprimidas.
 */
struct ScriptCompression
{
    static const unsigned int nSpecialScripts = 6;

    template<typename Stream, typename T>
    void Ser(Stream &s, const T& script) {
        CompressedScript compr;
        if (CompressScript(script, compr)) {
            s << std::span{compr};
            return;
        }
        unsigned int nSize = script.size() + nSpecialScripts;
        s << VARINT(nSize);
        s << std::span{script};
    }

    template<typename Stream, typename T>
    void Unser(Stream &s, T& script) {
        unsigned int nSize = 0;
        s >> VARINT(nSize);
        if (nSize < nSpecialScripts) {
            CompressedScript vch(GetSpecialScriptSize(nSize), 0x00);
            s >> std::span{vch};
            DecompressScript(script, nSize, vch);
            return;
        }
        nSize -= nSpecialScripts;
        script.resize(nSize);
        s >> std::span{script};
    }
};

/** Serializador compacto para Quantidades (Moedas) */
struct AmountCompression
{
    template<typename Stream, typename I> 
    void Ser(Stream& s, I val)
    {
        s << VARINT(CompressAmount(val));
    }
    template<typename Stream, typename I> 
    void Unser(Stream& s, I& val)
    {
        uint64_t v;
        s >> VARINT(v);
        val = DecompressAmount(v);
    }
};

#endif // MAZECHAIN_COMPRESSOR_H