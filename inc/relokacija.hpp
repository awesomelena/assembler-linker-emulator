// ============================================================================
// relokacija.hpp
//
// Relokacioni zapis je "odlozeni zadatak sabiranja" koji asembler ostavlja
// linkeru: na datom mestu (offset) u sekciji treba da stoji vrednost izracunata
// po formuli  vrednost = vrednost(simbol) + addend.
//
// Za globalni/eksterni simbol: relokacija pokazuje na sam simbol, addend = 0.
// Za lokalni simbol: relokacija pokazuje na SEKCIJU kojoj simbol pripada, a
// addend je offset simbola unutar te sekcije.
// ============================================================================

#ifndef RELOKACIJA_HPP
#define RELOKACIJA_HPP

#include "tipovi.hpp"
#include <string>

// Tip relokacije definise formulu po kojoj linker racuna vrednost.
// Za ovu arhitekturu dovoljna nam je apsolutna 32-bitna vrednost: linker upise
// punu 32-bitnu adresu simbola (uvecanu za addend) na dati offset.
enum class TipRelokacije {
    APSOLUTNA_32   // upisi 32-bitnu vrednost: vrednost(simbol) + addend
};

// Jedan relokacioni zapis. Vezan je za jednu sekciju (u kojoj se nalazi offset).
struct Relokacija {
    u32 offset;                 // pozicija u sekciji koju treba popraviti (u bajtovima)
    TipRelokacije tip;          // formula izracunavanja
    u32 indeksSimbola;          // redni broj simbola ILI sekcije u tabeli simbola
    i32 addend;                 // dodatak na vrednost simbola

    // Privremeno polje koje asembler koristi dok ne finalizuje relokaciju:
    // ime simbola na koji se relokacija odnosi. Odluka o tome da li relokacija
    // pokazuje na simbol ili na sekciju donosi se tek na kraju asembliranja,
    // kada su svi simboli u fajlu definisani. Za ucitane (linkerske) relokacije
    // ovo polje je prazno jer je odluka vec doneta.
    std::string imeSimbola;
};

#endif // RELOKACIJA_HPP