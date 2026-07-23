// ============================================================================
// hex_izlaz.cpp
//
// Implementacija -hex ispisa: svaka spojena sekcija se ispisuje kao niz redova
// oblika "adresa: 8 bajtova", pocev od bazne adrese sekcije.
// ============================================================================

#include "hex_izlaz.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

// Formatira 32-bitnu adresu kao osmocifreni hex (npr. "40000000").
static std::string hexAdresa(u32 v) {
    std::ostringstream os;
    os << std::hex << std::uppercase << std::setw(8) << std::setfill('0') << v;
    return os.str();
}

// Formatira jedan bajt kao dvocifreni hex (npr. "1F").
static std::string hexBajt(u8 v) {
    std::ostringstream os;
    os << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
       << static_cast<u32>(v);
    return os.str();
}

bool ispisiHex(const std::string& putanja,
               const std::vector<SpojenaSekcija>& sekcije) {
    std::ofstream izlaz(putanja);
    if (!izlaz.is_open()) {
        return false;
    }

    // Za svaku sekciju ispisujemo njen sadrzaj u redovima po 8 bajtova,
    // pocev od njene bazne adrese.
    for (const SpojenaSekcija& sek : sekcije) {
        for (size_t k = 0; k < sek.sadrzaj.size(); k += 8) {
            // Adresa prvog bajta u ovom redu.
            u32 adresaReda = sek.bazna_adresa + static_cast<u32>(k);
            izlaz << hexAdresa(adresaReda) << ":";

            // Do 8 bajtova u redu (poslednji red moze biti kraci).
            for (size_t b = 0; b < 8 && (k + b) < sek.sadrzaj.size(); b++) {
                izlaz << " " << hexBajt(sek.sadrzaj[k + b]);
            }
            izlaz << "\n";
        }
    }

    return true;
}