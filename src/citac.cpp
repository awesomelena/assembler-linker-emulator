// ============================================================================
// citac.cpp
//
// Ucitavanje predmetnog programa iz tekstualne datoteke koju generise asembler.
// Format je "ogledalo" onoga sto pise izlaz.cpp: citamo iste blokove istim
// redom (#tabela_simbola, #sekcija, #relokacije).
// ============================================================================

#include "predmetni_program.hpp"
#include <fstream>
#include <sstream>
#include <string>

// Pretvara dvocifreni hex string (npr. "3A") u bajt.
static u8 hexUBajt(const std::string& s) {
    return static_cast<u8>(std::stoul(s, nullptr, 16));
}

bool ucitajPredmetniProgram(const std::string& putanja,
                            PredmetniProgram& program,
                            std::string& greska) {
    std::ifstream ulaz(putanja);
    if (!ulaz.is_open()) {
        greska = "ne mogu da otvorim datoteku '" + putanja + "'";
        return false;
    }

    program.imeDatoteke = putanja;

    std::string linija;
    // Trenutno stanje citaca: u kom bloku se nalazimo.
    enum class Stanje { NISTA, SIMBOLI, SEKCIJA_SADRZAJ, RELOKACIJE };
    Stanje stanje = Stanje::NISTA;

    // Indeks sekcije na koju se odnosi tekuci #sekcija ili #relokacije blok.
    int trenutnaSekcija = -1;

    while (std::getline(ulaz, linija)) {
        // Preskoci potpuno prazne linije.
        if (linija.empty()) {
            continue;
        }

        // Linije koje pocinju markerom (#) menjaju stanje ili nose zaglavlja.
        if (linija[0] == '#') {
            std::istringstream ls(linija);
            std::string marker;
            ls >> marker;  // npr. "#tabela_simbola", "#sekcija", "#velicina"...

            if (marker == "#tabela_simbola") {
                stanje = Stanje::SIMBOLI;
            } else if (marker == "#sekcija") {
                // Iza markera sledi ime sekcije. Pravimo novu sekciju.
                std::string imeSek;
                ls >> imeSek;
                Sekcija s;
                s.ime = imeSek;
                program.sekcije.push_back(s);
                trenutnaSekcija = static_cast<int>(program.sekcije.size()) - 1;
                stanje = Stanje::SEKCIJA_SADRZAJ;
            } else if (marker == "#velicina") {
                // Informativno; sadrzaj citamo dok ne naidjemo na sledeci marker.
                // Ne moramo nista da radimo jer velicinu odredjuje sam sadrzaj.
            } else if (marker == "#relokacije") {
                // Iza markera je ime sekcije; ono je vec poznato (tekuca sekcija).
                stanje = Stanje::RELOKACIJE;
            } else if (marker == "#offset") {
                // Zaglavlje kolona relokacija - preskoci.
            } else if (marker == "#redni") {
                // Zaglavlje kolona tabele simbola - preskoci.
            } else if (marker == "#kraj") {
                break;
            }
            // Ostali #-markeri (zaglavlja) se ignorisu.
            continue;
        }

        // Linije bez markera obradjujemo prema trenutnom stanju.
        std::istringstream ls(linija);

        if (stanje == Stanje::SIMBOLI) {
            // Format: redni ime sekcija vrednost def glob ext vrsta
            Simbol s;
            long sekcija;
            int def, glob, ext;
            std::string vrsta;
            ls >> s.redniBroj >> s.ime >> sekcija >> s.vrednost
               >> def >> glob >> ext >> vrsta;

            s.indeksSekcije = (sekcija < 0)
                                  ? NEDEFINISANA_SEKCIJA
                                  : static_cast<u32>(sekcija);
            s.jeDefinisan = (def != 0);
            s.jeGlobalan = (glob != 0);
            s.jeEksteran = (ext != 0);
            s.vrsta = (vrsta == "SEK") ? VrstaSimbola::SEKCIJA : VrstaSimbola::SIMBOL;

            program.simboli.push_back(s);
        }
        else if (stanje == Stanje::SEKCIJA_SADRZAJ) {
            // Linija sadrzi bajtove sekcije u hex-u, razdvojene razmakom.
            std::string bajt;
            while (ls >> bajt) {
                program.sekcije[trenutnaSekcija].sadrzaj.push_back(hexUBajt(bajt));
            }
        }
        else if (stanje == Stanje::RELOKACIJE) {
            // Format: offset tip simbol addend
            Relokacija r;
            std::string tip;
            ls >> r.offset >> tip >> r.indeksSimbola >> r.addend;
            // Trenutno postoji samo jedan tip relokacije.
            r.tip = TipRelokacije::APSOLUTNA_32;
            program.sekcije[trenutnaSekcija].relokacije.push_back(r);
        }
    }

    return true;
}