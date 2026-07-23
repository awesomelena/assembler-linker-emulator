// ============================================================================
// ucitavanje.cpp
//
// Implementacija ucitavanja -hex zapisa u memoriju emulatora.
// ============================================================================

#include "ucitavanje.hpp"
#include <fstream>
#include <sstream>
#include <string>

bool ucitajHexUMemoriju(const std::string& putanja,
                        Memorija& memorija,
                        std::string& greska) {
    std::ifstream ulaz(putanja);
    if (!ulaz.is_open()) {
        greska = "ne mogu da otvorim datoteku '" + putanja + "'";
        return false;
    }

    std::string linija;
    size_t brLinije = 0;
    while (std::getline(ulaz, linija)) {
        brLinije++;
        // Preskoci prazne linije.
        if (linija.empty()) {
            continue;
        }

        // Format reda: "ADRESA: BB BB BB ...".
        // Prvo izdvojimo adresu (do dvotacke).
        size_t dvotacka = linija.find(':');
        if (dvotacka == std::string::npos) {
            greska = "linija " + std::to_string(brLinije) +
                     ": nedostaje ':' u hex zapisu";
            return false;
        }

        std::string adrStr = linija.substr(0, dvotacka);
        u32 adresa;
        try {
            adresa = static_cast<u32>(std::stoul(adrStr, nullptr, 16));
        } catch (const std::exception&) {
            greska = "linija " + std::to_string(brLinije) +
                     ": neispravna adresa '" + adrStr + "'";
            return false;
        }

        // Ostatak linije su bajtovi razdvojeni razmakom.
        std::istringstream bajtovi(linija.substr(dvotacka + 1));
        std::string bajt;
        u32 tekucaAdresa = adresa;
        while (bajtovi >> bajt) {
            try {
                u8 vrednost = static_cast<u8>(std::stoul(bajt, nullptr, 16));
                memorija.pisiBajt(tekucaAdresa, vrednost);
                tekucaAdresa++;
            } catch (const std::exception&) {
                greska = "linija " + std::to_string(brLinije) +
                         ": neispravan bajt '" + bajt + "'";
                return false;
            }
        }
    }

    return true;
}