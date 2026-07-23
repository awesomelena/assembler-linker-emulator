// ============================================================================
// asembler.cpp
//
// Ulazna tacka (main) asemblera. Zaduzen je za obradu argumenata komandne
// linije, ucitavanje ulazne datoteke u listu linija i pokretanje asembliranja.
// Sama logika asembliranja je u klasi Asembler (parser.cpp).
//
// Nacin pokretanja:
//   ./asembler [opcije] <ulazna_datoteka>
// Opcije:
//   -o <izlazna_datoteka>   naziv izlaznog predmetnog programa
// ============================================================================

#include "asembler.hpp"
#include "izlaz.hpp"
#include <iostream>
#include <fstream>
#include <string>
#include <vector>

// Ucitava ceo tekstualni fajl u vektor linija (bez zavrsnih znakova novog reda).
// Vraca true ako je uspesno, false ako fajl ne moze da se otvori.
static bool ucitajLinije(const std::string& putanja, std::vector<std::string>& linije) {
    std::ifstream ulaz(putanja);
    if (!ulaz.is_open()) {
        return false;
    }
    std::string linija;
    while (std::getline(ulaz, linija)) {
        linije.push_back(linija);
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::string ulaznaDatoteka;
    std::string izlaznaDatoteka = "izlaz.o";

    // Obrada argumenata komandne linije.
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Greska: opcija -o zahteva naziv izlazne datoteke.\n";
                return 1;
            }
            izlaznaDatoteka = argv[++i];
        } else {
            ulaznaDatoteka = arg;
        }
    }

    if (ulaznaDatoteka.empty()) {
        std::cerr << "Greska: nije navedena ulazna datoteka.\n";
        std::cerr << "Upotreba: asembler [-o izlaz.o] <ulaz.s>\n";
        return 1;
    }

    // Ucitavanje izvornog koda.
    std::vector<std::string> linije;
    if (!ucitajLinije(ulaznaDatoteka, linije)) {
        std::cerr << "Greska: ne mogu da otvorim ulaznu datoteku '"
                  << ulaznaDatoteka << "'.\n";
        return 1;
    }

    // Asembliranje.
    Asembler asembler;
    try {
        asembler.asembliraj(linije);
    } catch (const AsemblerskaGreska& g) {
        std::cerr << "Greska pri asembliranju: " << g.poruka << "\n";
        return 1;
    }

    // Ispis predmetnog programa u izlaznu datoteku.
    if (!ispisiPredmetniProgram(izlaznaDatoteka, asembler.tabela(),
                                asembler.sveSekcije())) {
        std::cerr << "Greska: ne mogu da otvorim izlaznu datoteku '"
                  << izlaznaDatoteka << "' za pisanje.\n";
        return 1;
    }

    return 0;
}