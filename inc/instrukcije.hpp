// ============================================================================
// instrukcije.hpp
//
// Definicije vezane za format masinskih instrukcija ciljne arhitekture i
// pomocne funkcije za sklapanje (kodiranje) instrukcija u 4 bajta.
//
// Format instrukcije (4 bajta), prema prilogu:
//
//    Bajt 0     Bajt 1     Bajt 2      Bajt 3
//   [OC][MOD]  [RegA][RegB] [RegC][Disp] [Disp][Disp]
//    4   4      4    4        4    4        8       (bita)
//
//   OC   - operacioni kod (koja instrukcija)
//   MOD  - modifikator (varijanta instrukcije)
//   RegA, RegB, RegC - indeksi registara (0..15)
//   Disp - oznaceni 12-bitni pomeraj
// ============================================================================

#ifndef INSTRUKCIJE_HPP
#define INSTRUKCIJE_HPP

#include "tipovi.hpp"

// ----------------------------------------------------------------------------
// Operacioni kodovi (gornja 4 bita prvog bajta), prema prilogu.
// ----------------------------------------------------------------------------
enum Opkod : u8 {
    OC_HALT   = 0b0000,  // zaustavljanje procesora
    OC_INT    = 0b0001,  // softverski prekid
    OC_CALL   = 0b0010,  // poziv potprograma
    OC_JMP    = 0b0011,  // skokovi (bezuslovni i uslovni), MOD bira
    OC_XCHG   = 0b0100,  // atomicna zamena registara
    OC_ARIT   = 0b0101,  // aritmetika (add/sub/mul/div), MOD bira
    OC_LOGIKA = 0b0110,  // logika (not/and/or/xor), MOD bira
    OC_POMER  = 0b0111,  // pomeranje (shl/shr), MOD bira
    OC_ST     = 0b1000,  // smestanje podatka u memoriju, MOD bira
    OC_LD     = 0b1001   // ucitavanje podatka u registar, MOD bira
};

// Indeksi kontrolnih i statusnih registara, prema prilogu.
enum CsrIndeks : u8 {
    CSR_STATUS  = 0,
    CSR_HANDLER = 1,
    CSR_CAUSE   = 2
};

// Indeksi opstenamenskih registara sa posebnom ulogom.
constexpr u8 REG_SP = 14;  // stack pointer
constexpr u8 REG_PC = 15;  // program counter

// Bitovi statusnog registra (maskiranje prekida), prema prilogu.
//   Tr (bit 0) - maskiranje tajmera   (0 omogucen, 1 maskiran)
//   Tl (bit 1) - maskiranje terminala (0 omogucen, 1 maskiran)
//   I  (bit 2) - globalno maskiranje  (0 omoguceni, 1 maskirani)
constexpr u32 STATUS_TR = 0x1;  // tajmer
constexpr u32 STATUS_TL = 0x2;  // terminal
constexpr u32 STATUS_I  = 0x4;  // globalno

// Vrednosti cause registra (uzrok ulaska u prekidnu rutinu), prema prilogu.
constexpr u32 CAUSE_GRESKA   = 1;  // nekorektna instrukcija
constexpr u32 CAUSE_TAJMER   = 2;  // prekid od tajmera
constexpr u32 CAUSE_TERMINAL = 3;  // prekid od terminala
constexpr u32 CAUSE_SOFTVER  = 4;  // softverski prekid (int)

// ----------------------------------------------------------------------------
// Sklapa instrukciju u 32-bitnu vrednost iz pojedinacnih polja.
// Polja koja se ne koriste za datu instrukciju prosledjuju se kao 0.
// Disp je 12-bitni oznaceni pomeraj; ovde ga prihvatamo kao i32 i uzimamo
// donjih 12 bita (pozivalac je duzan da proveri da vrednost staje u 12 bita).
// ----------------------------------------------------------------------------
inline u32 sklopiInstrukciju(u8 oc, u8 mod, u8 regA, u8 regB, u8 regC, i32 disp) {
    u32 rezultat = 0;
    rezultat |= (static_cast<u32>(oc   & 0xF) << 28);
    rezultat |= (static_cast<u32>(mod  & 0xF) << 24);
    rezultat |= (static_cast<u32>(regA & 0xF) << 20);
    rezultat |= (static_cast<u32>(regB & 0xF) << 16);
    rezultat |= (static_cast<u32>(regC & 0xF) << 12);
    rezultat |= (static_cast<u32>(disp) & 0xFFF);  // donjih 12 bita pomeraja
    return rezultat;
}

// Provera da li oznaceni broj staje u 12-bitni opseg pomeraja (-2048..2047).
inline bool staje12bita(i32 vrednost) {
    return vrednost >= -2048 && vrednost <= 2047;
}

// ----------------------------------------------------------------------------
// Raspakovana (dekodirana) instrukcija - obrnuto od sklopiInstrukciju.
// ----------------------------------------------------------------------------
struct DekodiranaInstrukcija {
    u8 oc;    // operacioni kod
    u8 mod;   // modifikator
    u8 regA;  // indeks registra A
    u8 regB;  // indeks registra B
    u8 regC;  // indeks registra C
    i32 disp; // 12-bitni oznaceni pomeraj (prosiren na 32 bita sa znakom)
};

// Rastavlja 32-bitnu rec na polja instrukcije. Pomeraj se prosiruje sa znakom
// iz 12 na 32 bita (jer je Disp oznaceni broj u opsegu -2048..2047).
inline DekodiranaInstrukcija dekodirajInstrukciju(u32 rec) {
    DekodiranaInstrukcija d;
    d.oc   = static_cast<u8>((rec >> 28) & 0xF);
    d.mod  = static_cast<u8>((rec >> 24) & 0xF);
    d.regA = static_cast<u8>((rec >> 20) & 0xF);
    d.regB = static_cast<u8>((rec >> 16) & 0xF);
    d.regC = static_cast<u8>((rec >> 12) & 0xF);

    // Donjih 12 bita je pomeraj. Prosirenje znaka: ako je bit 11 postavljen,
    // vrednost je negativna, pa popunjavamo gornje bitove jedinicama.
    u32 disp12 = rec & 0xFFF;
    if (disp12 & 0x800) {
        d.disp = static_cast<i32>(disp12 | 0xFFFFF000u);  // negativna
    } else {
        d.disp = static_cast<i32>(disp12);                // pozitivna
    }
    return d;
}

#endif // INSTRUKCIJE_HPP