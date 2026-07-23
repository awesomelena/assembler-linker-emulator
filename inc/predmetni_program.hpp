// ============================================================================
// predmetni_program.hpp
//
// Struktura koja drzi jedan ucitani predmetni program (izlaz asemblera) u
// memoriji linkera: tabelu simbola, sekcije sa sadrzajem i relokacije.
//
// Svaki ucitani fajl je zaseban objekat jer se redni brojevi simbola
// preklapaju medju fajlovima (svaki fajl numerise svoje simbole od nule).
// ============================================================================

#ifndef PREDMETNI_PROGRAM_HPP
#define PREDMETNI_PROGRAM_HPP

#include "tipovi.hpp"
#include "sekcija.hpp"
#include "tabela_simbola.hpp"
#include <string>
#include <vector>

// Jedan ucitani predmetni program.
struct PredmetniProgram {
    std::string imeDatoteke;          // radi jasnih poruka o greskama
    std::vector<Simbol> simboli;      // tabela simbola ovog fajla
    std::vector<Sekcija> sekcije;     // sekcije sa sadrzajem i relokacijama

    // Pomocna: nadji simbol po rednom broju (indeksu u tabeli ovog fajla).
    const Simbol* simbolPoIndeksu(u32 indeks) const {
        if (indeks < simboli.size()) {
            return &simboli[indeks];
        }
        return nullptr;
    }
};

// Ucitava predmetni program iz tekstualne datoteke koju je generisao asembler.
// Vraca true ako je uspesno; u suprotnom false i postavlja poruku o gresci.
bool ucitajPredmetniProgram(const std::string& putanja,
                            PredmetniProgram& program,
                            std::string& greska);

#endif // PREDMETNI_PROGRAM_HPP