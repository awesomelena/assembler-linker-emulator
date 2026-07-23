// ============================================================================
// linker.cpp
//
// Ulazna tacka (main) linkera. Povezuje jedan ili vise predmetnih programa u
// celinu: ucitava ih, spaja istoimene sekcije, raspoređuje na adrese i (u
// narednim fazama) razresava relokacije i generise izlaz.
//
// Nacin pokretanja:
//   ./linker [opcije] <ulazna_datoteka>...
// Opcije:
//   -o <izlaz>              naziv izlazne datoteke
//   -place=<sekcija>@<adr>  eksplicitna adresa sekcije
//   -hex                    generisi zapis za inicijalizaciju memorije
//   -relocatable            generisi predmetni program (opet relokatibilan)
// ============================================================================

#include "linker.hpp"
#include "predmetni_program.hpp"
#include "hex_izlaz.hpp"
#include "izlaz.hpp"
#include <iostream>
#include <string>
#include <vector>

// Parsira -place=<ime>@<adresa> u ime sekcije i adresu.
// Vraca true ako je format ispravan.
static bool parsirajPlace(const std::string& arg, std::string& ime, u32& adresa) {
    // Ocekivani oblik posle "-place=" : ime@adresa
    std::string telo = arg.substr(std::string("-place=").size());
    size_t at = telo.find('@');
    if (at == std::string::npos) {
        return false;
    }
    ime = telo.substr(0, at);
    std::string adrStr = telo.substr(at + 1);
    if (ime.empty() || adrStr.empty()) {
        return false;
    }
    try {
        // Osnova 0 dozvoljava i heksadecimalni (0x...) i decimalni zapis.
        adresa = static_cast<u32>(std::stoul(adrStr, nullptr, 0));
    } catch (const std::exception&) {
        return false;
    }
    return true;
}

int main(int argc, char* argv[]) {
    std::string izlaznaDatoteka = "izlaz.hex";
    std::vector<std::string> ulazneDatoteke;
    std::vector<std::pair<std::string, u32>> placeOpcije;

    bool hexRezim = false;
    bool relocatableRezim = false;

    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "-o") {
            if (i + 1 >= argc) {
                std::cerr << "Greska: opcija -o zahteva naziv izlazne datoteke.\n";
                return 1;
            }
            izlaznaDatoteka = argv[++i];
        } else if (arg == "-hex") {
            hexRezim = true;
        } else if (arg == "-relocatable") {
            relocatableRezim = true;
        } else if (arg.rfind("-place=", 0) == 0) {
            std::string ime;
            u32 adresa;
            if (!parsirajPlace(arg, ime, adresa)) {
                std::cerr << "Greska: neispravna -place opcija: " << arg << "\n";
                return 1;
            }
            placeOpcije.push_back({ime, adresa});
        } else {
            ulazneDatoteke.push_back(arg);
        }
    }

    // Tacno jedan od dva rezima mora biti naveden.
    if (hexRezim == relocatableRezim) {
        std::cerr << "Greska: navesti tacno jednu od opcija -hex ili -relocatable.\n";
        return 1;
    }
    if (ulazneDatoteke.empty()) {
        std::cerr << "Greska: nije navedena nijedna ulazna datoteka.\n";
        return 1;
    }

    // Ucitaj sve predmetne programe.
    Linker linker;
    for (const std::string& put : ulazneDatoteke) {
        PredmetniProgram prog;
        std::string greska;
        if (!ucitajPredmetniProgram(put, prog, greska)) {
            std::cerr << "Greska pri ucitavanju: " << greska << "\n";
            return 1;
        }
        linker.dodajProgram(prog);
    }

    // Dodaj -place direktive.
    for (const auto& p : placeOpcije) {
        linker.dodajPlace(p.first, p.second);
    }

    // Spajanje, raspoređivanje i (za hex) razresavanje relokacija.
    try {
        linker.spoji();
        if (hexRezim) {
            linker.rasporedi();
            linker.razresiRelokacije();
        }
    } catch (const LinkerskaGreska& g) {
        std::cerr << "Greska pri linkovanju: " << g.poruka << "\n";
        return 1;
    }

    // Generisanje izlaza.
    if (hexRezim) {
        if (!ispisiHex(izlaznaDatoteka, linker.spojeneSekcije())) {
            std::cerr << "Greska: ne mogu da otvorim izlaznu datoteku '"
                      << izlaznaDatoteka << "' za pisanje.\n";
            return 1;
        }
    } else {
        // -relocatable: generisi spojen predmetni program (od nulte adrese).
        std::vector<Simbol> izlazniSimboli;
        std::vector<Sekcija> izlazneSekcije;
        linker.generisiRelocatable(izlazniSimboli, izlazneSekcije);
        if (!ispisiPredmetniProgram(izlaznaDatoteka, izlazniSimboli, izlazneSekcije)) {
            std::cerr << "Greska: ne mogu da otvorim izlaznu datoteku '"
                      << izlaznaDatoteka << "' za pisanje.\n";
            return 1;
        }
    }

    return 0;
}