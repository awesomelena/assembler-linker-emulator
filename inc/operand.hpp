// ============================================================================
// operand.hpp
//
// Opis jednog operanda asemblerske naredbe. Lekser daje tokene; ovaj tip
// hvata SEMANTIKU operanda (koji je nacin adresiranja u pitanju) kako bi
// generisanje koda bilo pregledno.
//
// Nacini adresiranja za naredbe za rad sa podacima (iz postavke):
//   $<literal>            neposredna vrednost (literal)
//   $<simbol>             neposredna vrednost (adresa simbola)
//   <literal>             vrednost iz memorije na adresi <literal>
//   <simbol>              vrednost iz memorije na adresi <simbol>
//   %<reg>                vrednost u registru
//   [%<reg>]              vrednost iz memorije na adresi u registru
//   [%<reg> + <literal>]  vrednost iz memorije na adresi reg + literal
//   [%<reg> + <simbol>]   vrednost iz memorije na adresi reg + simbol
//
// Nacini adresiranja za naredbe skoka (iz postavke):
//   <literal>            odredisna adresa = literal
//   <simbol>             odredisna adresa = simbol
// ============================================================================

#ifndef OPERAND_HPP
#define OPERAND_HPP

#include "tipovi.hpp"
#include <string>

// Vrsta operanda (nacin adresiranja).
enum class VrstaOperanda {
    IMED_LITERAL,     // $<literal>           neposredni literal
    IMED_SIMBOL,      // $<simbol>            neposredna vrednost simbola
    MEM_LITERAL,      // <literal>            iz memorije na adresi literal
    MEM_SIMBOL,       // <simbol>             iz memorije na adresi simbol
    REG,              // %<reg>               vrednost u registru
    MEM_REG,          // [%<reg>]             iz memorije na adresi reg
    MEM_REG_LITERAL,  // [%<reg> + <literal>] iz memorije na adresi reg+literal
    MEM_REG_SIMBOL    // [%<reg> + <simbol>]  iz memorije na adresi reg+simbol
};

struct Operand {
    VrstaOperanda vrsta;
    u8 reg = 0;              // indeks registra (za REG i MEM_REG* oblike)
    i32 literal = 0;         // vrednost literala (za *_LITERAL oblike)
    std::string simbol;      // ime simbola (za *_SIMBOL oblike)
};

#endif // OPERAND_HPP