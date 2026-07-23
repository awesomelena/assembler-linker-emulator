// ============================================================================
// bazen.cpp
//
// Implementacija mehanizma bazena literala i relokacija.
//
// Podsetnik na mehanizam (iz teorije):
//  - kada instrukcija koristi simbol ili literal koji ne staje u 12 bita,
//    vrednost se smesta u "bazen" na kraju sekcije (4 bajta),
//  - instrukcija dobija PC-relativni pomeraj do te stavke (zakrpljuje se na
//    kraju sekcije, kada znamo gde bazen pocinje),
//  - za simbolske stavke pravi se relokacija koju ce linker razresiti.
// ============================================================================

#include "asembler.hpp"

size_t Asembler::dodajLiteralUBazen(u32 vrednost) {
    Sekcija* sek = aktivnaSekcija();
    StavkaBazena st;
    st.jeSimbol = false;
    st.vrednost = vrednost;
    sek->bazen.push_back(st);
    return sek->bazen.size() - 1;
}

size_t Asembler::dodajSimbolUBazen(const std::string& ime) {
    Sekcija* sek = aktivnaSekcija();
    StavkaBazena st;
    st.jeSimbol = true;
    st.vrednost = 0;             // prava vrednost dolazi od linkera preko relokacije
    st.imeSimbola = ime;
    sek->bazen.push_back(st);
    return sek->bazen.size() - 1;
}

void Asembler::napraviRelokaciju(u32 offset, const std::string& imeSimbola) {
    Sekcija* sek = aktivnaSekcija();

    // Osiguraj da simbol postoji u tabeli (bar kao referenca).
    tabelaSimbola.nadjiIliDodaj(imeSimbola);

    // Ne donosimo odluku lokalni/globalni odmah - simbol mozda jos nije
    // definisan (npr. definisan je kasnije u fajlu, u drugoj sekciji). Samo
    // zabelezimo ime; konacnu odluku donosi finalizujRelokacije() na kraju
    // asembliranja, kada su svi simboli u fajlu poznati.
    Relokacija r;
    r.offset = offset;
    r.tip = TipRelokacije::APSOLUTNA_32;
    r.indeksSimbola = 0;         // popunjava se pri finalizaciji
    r.addend = 0;                // popunjava se pri finalizaciji
    r.imeSimbola = imeSimbola;

    sek->relokacije.push_back(r);
}

void Asembler::isprazniBazen() {
    Sekcija* sek = aktivnaSekcija();

    // 1) Odredi gde ce svaka stavka biti smestena (offset u sekciji) i upisi je.
    for (size_t k = 0; k < sek->bazen.size(); k++) {
        StavkaBazena& st = sek->bazen[k];
        st.offsetUBazenu = sek->lokacija();   // trenutni kraj sekcije

        if (st.jeSimbol) {
            // U bazen ide 0; linker ce upisati pravu adresu preko relokacije.
            u32 offsetStavke = sek->lokacija();
            sek->dodajRec(0);
            napraviRelokaciju(offsetStavke, st.imeSimbola);
        } else {
            sek->dodajRec(st.vrednost);
        }
    }

    // 2) Zakrpi pomeraje u instrukcijama koje koriste bazen.
    //    Pomeraj je PC-relativan u odnosu na adresu SLEDECE instrukcije.
    //    U trenutku izvrsavanja instrukcije na offsetu X, pc vec pokazuje na
    //    X+4 (sledeca instrukcija). Zato je:
    //        pomeraj = offsetStavke - (offsetInstrukcije + 4)
    for (const Zakrpa& z : sek->zakrpe) {
        u32 offsetStavke = sek->bazen[z.indeksStavke].offsetUBazenu;
        i32 pomeraj = static_cast<i32>(offsetStavke) -
                      static_cast<i32>(z.offsetInstrukcije + 4);

        // Pomeraj mora da stane u 12 bita. Ako sekcija postane prevelika da
        // bazen bude na dohvat, to je greska (u praksi se ne desava za razumne
        // velicine sekcija u okviru ovog projekta).
        if (pomeraj < -2048 || pomeraj > 2047) {
            throw AsemblerskaGreska{
                "pomeraj do bazena literala ne staje u 12 bita (sekcija '" +
                sek->ime + "')"};
        }

        // Upisi donjih 12 bita pomeraja u instrukciju, cuvajuci ostala polja.
        // Pomeraj je u trecem i cetvrtom bajtu instrukcije (donjih 12 bita reci).
        u32 offInstr = z.offsetInstrukcije;
        // Procitaj postojecu instrukciju (little-endian) da sacuvamo gornja polja.
        u32 instr = static_cast<u32>(sek->sadrzaj[offInstr + 0]) |
                    (static_cast<u32>(sek->sadrzaj[offInstr + 1]) << 8) |
                    (static_cast<u32>(sek->sadrzaj[offInstr + 2]) << 16) |
                    (static_cast<u32>(sek->sadrzaj[offInstr + 3]) << 24);
        instr = (instr & 0xFFFFF000u) | (static_cast<u32>(pomeraj) & 0xFFF);
        sek->upisiRecNa(offInstr, instr);
    }

    // 3) Bazen i zakrpe su obradjeni - ocisti ih za eventualnu sledecu upotrebu.
    sek->bazen.clear();
    sek->zakrpe.clear();
}

// ----------------------------------------------------------------------------
// Finalizuje sve relokacije nakon sto je celo asembliranje zavrseno i svi
// simboli su definisani. Za svaku relokaciju sa zabelezenim imenom simbola
// donosi odluku: lokalni simbol -> relokacija na sekciju + addend (offset
// simbola); globalni/eksterni -> relokacija na sam simbol.
// ----------------------------------------------------------------------------
void Asembler::finalizujRelokacije() {
    for (Sekcija& sek : sekcije) {
        for (Relokacija& r : sek.relokacije) {
            // Preskoci relokacije bez imena (ne bi trebalo da ih ima u ovoj
            // fazi, ali radi sigurnosti).
            if (r.imeSimbola.empty()) {
                continue;
            }

            Simbol* sim = tabelaSimbola.nadji(r.imeSimbola);
            if (sim == nullptr) {
                throw AsemblerskaGreska{
                    "relokacija na nepoznat simbol '" + r.imeSimbola + "'"};
            }

            // Apsolutan simbol (npr. definisan preko .equ) ima vrednost koja
            // je poznata vec u vreme asembliranja i ne zavisi od rasporeda
            // sekcija. Za njega relokacija nije potrebna: vrednost upisujemo
            // direktno u sadrzaj, a relokaciju oznacavamo za uklanjanje.
            bool apsolutan = sim->jeDefinisan && !sim->jeEksteran &&
                             sim->indeksSekcije == NEDEFINISANA_SEKCIJA &&
                             sim->vrsta != VrstaSimbola::SEKCIJA;
            if (apsolutan) {
                sek.upisiRecNa(r.offset, sim->vrednost);
                r.imeSimbola = "";          // markiraj kao obradjenu
                r.indeksSimbola = 0xFFFFFFFF;
                continue;
            }

            // Lokalan = definisan u ovom fajlu i nije .global i nije .extern.
            bool lokalan = sim->jeDefinisan && !sim->jeGlobalan && !sim->jeEksteran;

            if (lokalan) {
                // Relokacija pokazuje na SEKCIJU simbola, addend = offset simbola.
                r.indeksSimbola = sim->indeksSekcije;
                r.addend = static_cast<i32>(sim->vrednost);
            } else {
                // Globalan ili eksteran: pokazuje na sam simbol, addend 0.
                r.indeksSimbola = sim->redniBroj;
                r.addend = 0;
            }
        }
    }

    // Ukloni relokacije koje su razresene na licu mesta (apsolutni simboli).
    for (Sekcija& sek : sekcije) {
        std::vector<Relokacija> preostale;
        for (const Relokacija& r : sek.relokacije) {
            if (r.indeksSimbola != 0xFFFFFFFF) {
                preostale.push_back(r);
            }
        }
        sek.relokacije = preostale;
    }
}