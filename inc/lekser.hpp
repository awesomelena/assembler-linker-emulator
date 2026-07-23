// ============================================================================
// lekser.hpp
//
// Leksicki analizator (lekser) pretvara jednu liniju izvornog asemblerskog
// koda u niz tokena - najmanjih smislenih jedinica (imena, brojeva, registara,
// interpunkcije). Time se odvaja "citanje karaktera" od "razumevanja znacenja"
// koje kasnije radi parser.
// ============================================================================

#ifndef LEKSER_HPP
#define LEKSER_HPP

#include "tipovi.hpp"
#include <string>
#include <vector>

// ----------------------------------------------------------------------------
// Vrste tokena koje lekser prepoznaje.
// ----------------------------------------------------------------------------
enum class VrstaTokena {
    IDENT,      // identifikator: ime instrukcije, direktive, labele ili simbola
    BROJ,       // celobrojni literal (decimalni ili heksadecimalni)
    REGISTAR,   // registar naveden sa % (npr. %r1, %sp, %status)
    DOLAR,      // znak $ (prefiks za neposrednu vrednost)
    ZAPETA,     // ,
    DVOTACKA,   // :
    UGL_OTV,    // [
    UGL_ZATV,   // ]
    PLUS,       // +
    MINUS,      // - (operator oduzimanja u izrazima, npr. u .equ)
    STRING,     // "..." (za .ascii direktivu)
    KRAJ        // marker kraja linije (nema vise tokena)
};

// ----------------------------------------------------------------------------
// Jedan token: njegova vrsta i tekstualni sadrzaj.
//   - za IDENT: samo ime (npr. "ld", "start")
//   - za BROJ: vrednost kao broj u polju broj, a tekst originala u tekst
//   - za REGISTAR: ime registra bez % (npr. "r1", "sp")
//   - za STRING: sadrzaj izmedju navodnika (bez samih navodnika)
//   - za interpunkciju: tekst je sam znak
// ----------------------------------------------------------------------------
struct Token {
    VrstaTokena vrsta;
    std::string tekst;   // tekstualni sadrzaj tokena
    i32 broj = 0;        // brojcana vrednost (popunjeno samo za vrstu BROJ)
};

// ----------------------------------------------------------------------------
// Izuzetak koji lekser baca kada naidje na neispravan sadrzaj (npr. nepoznat
// karakter ili nezatvoren string). Nosi opis greske radi jasne poruke.
// ----------------------------------------------------------------------------
struct LeksickaGreska {
    std::string poruka;
};

// Razbija jednu liniju izvornog koda na tokene.
// Na kraj svake linije uvek dodaje token vrste KRAJ.
// U slucaju neispravnog sadrzaja baca LeksickaGreska.
std::vector<Token> tokenizuj(const std::string& linija);

#endif // LEKSER_HPP