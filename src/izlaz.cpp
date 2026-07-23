// ============================================================================
// izlaz.cpp
//
// Implementacija ispisa predmetnog programa u tekstualnu datoteku.
// ============================================================================

#include "izlaz.hpp"
#include <fstream>
#include <iomanip>
#include <sstream>

// Pomocna: formatira jedan bajt kao dvocifreni heksadecimalni zapis.
static std::string hex2(u8 v) {
    std::ostringstream os;
    os << std::hex << std::uppercase << std::setw(2) << std::setfill('0')
       << static_cast<u32>(v);
    return os.str();
}

// Naziv tipa relokacije za ispis.
static std::string nazivTipaRelokacije(TipRelokacije t) {
    switch (t) {
        case TipRelokacije::APSOLUTNA_32: return "APS32";
    }
    return "?";
}

// Interna funkcija koja radi ceo posao nad vektorom simbola i sekcija.
static bool ispisiInterno(const std::string& putanja,
                          const std::vector<Simbol>& simboli,
                          const std::vector<Sekcija>& sekcije) {
    std::ofstream izlaz(putanja);
    if (!izlaz.is_open()) {
        return false;
    }

    // --- Tabela simbola ----------------------------------------------------
    izlaz << "#tabela_simbola\n";
    izlaz << "#redni ime sekcija vrednost def glob ext vrsta\n";
    for (const Simbol& s : simboli) {
        // Sekcija se ispisuje kao redni broj sekcije, ili -1 ako je nema.
        long sekcija = (s.indeksSekcije == NEDEFINISANA_SEKCIJA)
                           ? -1
                           : static_cast<long>(s.indeksSekcije);
        izlaz << s.redniBroj << " "
              << s.ime << " "
              << sekcija << " "
              << s.vrednost << " "
              << (s.jeDefinisan ? 1 : 0) << " "
              << (s.jeGlobalan ? 1 : 0) << " "
              << (s.jeEksteran ? 1 : 0) << " "
              << (s.vrsta == VrstaSimbola::SEKCIJA ? "SEK" : "SIM") << "\n";
    }
    izlaz << "\n";

    // --- Sekcije (sadrzaj) i njihove relokacije ----------------------------
    for (const Sekcija& sek : sekcije) {
        izlaz << "#sekcija " << sek.ime << "\n";
        izlaz << "#velicina " << sek.sadrzaj.size() << "\n";

        // Sadrzaj: ispisujemo bajtove u hex, po 16 u redu radi citljivosti.
        for (size_t k = 0; k < sek.sadrzaj.size(); k++) {
            izlaz << hex2(sek.sadrzaj[k]);
            if ((k + 1) % 16 == 0) {
                izlaz << "\n";
            } else {
                izlaz << " ";
            }
        }
        // Zavrsi red ako poslednji red nije bio pun.
        if (sek.sadrzaj.size() % 16 != 0 || sek.sadrzaj.empty()) {
            izlaz << "\n";
        }

        // Relokacije ove sekcije (ako ih ima).
        if (!sek.relokacije.empty()) {
            izlaz << "#relokacije " << sek.ime << "\n";
            izlaz << "#offset tip simbol addend\n";
            for (const Relokacija& r : sek.relokacije) {
                izlaz << r.offset << " "
                      << nazivTipaRelokacije(r.tip) << " "
                      << r.indeksSimbola << " "
                      << r.addend << "\n";
            }
        }
        izlaz << "\n";
    }

    izlaz << "#kraj\n";

    return true;
}

// Javni omotac: prima TabelaSimbola (koristi asembler).
bool ispisiPredmetniProgram(const std::string& putanja,
                            const TabelaSimbola& tabela,
                            const std::vector<Sekcija>& sekcije) {
    return ispisiInterno(putanja, tabela.sviSimboli(), sekcije);
}

// Javni omotac: prima vektor simbola direktno (koristi linker za -relocatable).
bool ispisiPredmetniProgram(const std::string& putanja,
                            const std::vector<Simbol>& simboli,
                            const std::vector<Sekcija>& sekcije) {
    return ispisiInterno(putanja, simboli, sekcije);
}