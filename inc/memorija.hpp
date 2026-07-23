// ============================================================================
// memorija.hpp
//
// Model memorije emuliranog racunara. Adresni prostor je 2^32 bajta, ali se
// stvarno cuvaju samo koriscene adrese (retko popunjena memorija) preko mape
// adresa -> bajt. Pristup 32-bitnim recima je little-endian, u skladu sa
// ciljnom arhitekturom.
// ============================================================================

#ifndef MEMORIJA_HPP
#define MEMORIJA_HPP

#include "tipovi.hpp"
#include <unordered_map>

class Memorija {
public:
    // Cita jedan bajt sa date adrese. Nepostojeci (neinicijalizovani) bajt
    // cita se kao 0.
    u8 citajBajt(u32 adresa) const {
        auto it = sadrzaj.find(adresa);
        return (it != sadrzaj.end()) ? it->second : 0;
    }

    // Upisuje jedan bajt na datu adresu.
    void pisiBajt(u32 adresa, u8 vrednost) {
        sadrzaj[adresa] = vrednost;
    }

    // Cita 32-bitnu rec (little-endian: najnizi bajt na najnizoj adresi).
    u32 citajRec(u32 adresa) const {
        u32 b0 = citajBajt(adresa + 0);
        u32 b1 = citajBajt(adresa + 1);
        u32 b2 = citajBajt(adresa + 2);
        u32 b3 = citajBajt(adresa + 3);
        return b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
    }

    // Upisuje 32-bitnu rec (little-endian).
    void pisiRec(u32 adresa, u32 vrednost) {
        pisiBajt(adresa + 0, static_cast<u8>(vrednost & 0xFF));
        pisiBajt(adresa + 1, static_cast<u8>((vrednost >> 8) & 0xFF));
        pisiBajt(adresa + 2, static_cast<u8>((vrednost >> 16) & 0xFF));
        pisiBajt(adresa + 3, static_cast<u8>((vrednost >> 24) & 0xFF));
    }

private:
    std::unordered_map<u32, u8> sadrzaj;
};

#endif // MEMORIJA_HPP