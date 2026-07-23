// ============================================================================
// jezgro.cpp
//
// Implementacija spajanja sekcija i izgradnje globalne tabele simbola.
// ============================================================================

#include "linker.hpp"

void Linker::dodajProgram(const PredmetniProgram& program) {
    programi.push_back(program);
}

SpojenaSekcija* Linker::nadjiSpojenu(const std::string& ime) {
    for (SpojenaSekcija& s : sekcije) {
        if (s.ime == ime) return &s;
    }
    return nullptr;
}

GlobalniSimbol* Linker::nadjiGlobalni(const std::string& ime) {
    for (GlobalniSimbol& g : globalna) {
        if (g.ime == ime) return &g;
    }
    return nullptr;
}

// ----------------------------------------------------------------------------
// Korak 1: spoji istoimene sekcije iz svih fajlova, redom kako su navedeni.
// Za svaki doprinos zapamti bazni offset (koliko je bilo pre njega u spojenoj
// sekciji) - trebace za pomeranje simbola i relokacija.
// ----------------------------------------------------------------------------
void Linker::spojiSekcije() {
    for (size_t f = 0; f < programi.size(); f++) {
        const PredmetniProgram& prog = programi[f];

        for (const Sekcija& sek : prog.sekcije) {
            // Nadji ili napravi spojenu sekciju istog imena.
            SpojenaSekcija* spojena = nadjiSpojenu(sek.ime);
            if (spojena == nullptr) {
                SpojenaSekcija nova;
                nova.ime = sek.ime;
                sekcije.push_back(nova);
                spojena = &sekcije.back();
            }

            // Bazni offset ovog doprinosa = trenutna velicina spojene sekcije.
            u32 bazni = static_cast<u32>(spojena->sadrzaj.size());
            bazniOffset[KljucDoprinosa{f, sek.ime}] = bazni;

            // Nadovezi sadrzaj.
            spojena->sadrzaj.insert(spojena->sadrzaj.end(),
                                    sek.sadrzaj.begin(), sek.sadrzaj.end());
        }
    }
}

// ----------------------------------------------------------------------------
// Korak 2: izgradi globalnu tabelu simbola.
// Prolazi kroz sve simbole svih fajlova i za svaki globalni/eksterni simbol
// pravi ili dopunjava unos u globalnoj tabeli. Vrednost (offset u spojenoj
// sekciji) racuna se kao bazni offset doprinosa + offset simbola u fajlu.
// Hvata visestruku definiciju istog globalnog simbola.
// ----------------------------------------------------------------------------
void Linker::izgradiGlobalnuTabelu() {
    for (size_t f = 0; f < programi.size(); f++) {
        const PredmetniProgram& prog = programi[f];

        for (const Simbol& sim : prog.simboli) {
            // Zanimaju nas samo simboli vidljivi medju fajlovima: globalni
            // (izvezeni) i eksterni (uvezeni). Lokalni i sekcije preskacemo -
            // oni se razresavaju lokalno, preko svoje sekcije.
            if (!sim.jeGlobalan && !sim.jeEksteran) {
                continue;
            }
            if (sim.vrsta == VrstaSimbola::SEKCIJA) {
                continue;
            }

            GlobalniSimbol* g = nadjiGlobalni(sim.ime);
            if (g == nullptr) {
                // Prvi put vidimo ovaj simbol - napravi unos.
                GlobalniSimbol novi;
                novi.ime = sim.ime;
                globalna.push_back(novi);
                g = &globalna.back();
            }

            // Ako je ovaj pojavak definicija (globalan i definisan u ovom fajlu):
            if (sim.jeGlobalan && sim.jeDefinisan) {
                if (g->definisan) {
                    // Vec je bio definisan negde - visestruka definicija.
                    throw LinkerskaGreska{
                        "visestruka definicija simbola '" + sim.ime + "'"};
                }
                g->definisan = true;

                // Apsolutan simbol (bez sekcije, npr. iz .equ): vrednost je
                // vec konacna i ne zavisi od rasporeda sekcija.
                if (sim.indeksSekcije == NEDEFINISANA_SEKCIJA) {
                    g->apsolutan = true;
                    g->apsolutnaVrednost = sim.vrednost;
                    continue;
                }

                // Nadji ime sekcije u kojoj je simbol definisan (preko indeksa
                // sekcije u tabeli simbola ovog fajla).
                const Simbol* sekSim = prog.simbolPoIndeksu(sim.indeksSekcije);
                if (sekSim != nullptr) {
                    g->imeSekcije = sekSim->ime;
                    // Offset u spojenoj sekciji = bazni offset doprinosa ovog
                    // fajla toj sekciji + offset simbola u fajlu.
                    u32 bazni = bazniOffset[KljucDoprinosa{f, sekSim->ime}];
                    g->offsetUSekciji = bazni + sim.vrednost;
                }
            }
            // Eksterni pojavci samo obezbedjuju da unos postoji; definiciju
            // ocekujemo iz nekog drugog fajla (proverava se pri relokaciji).
        }
    }
}

void Linker::spoji() {
    spojiSekcije();
    izgradiGlobalnuTabelu();
}

void Linker::dodajPlace(const std::string& imeSekcije, u32 adresa) {
    placeDirektive.push_back(PlaceDirektiva{imeSekcije, adresa});
}

// ----------------------------------------------------------------------------
// Proverava da se nijedne dve rasporedjene sekcije ne preklapaju u adresama.
// Sekcija zauzima opseg [bazna_adresa, bazna_adresa + velicina).
// ----------------------------------------------------------------------------
void Linker::proveriPreklapanja() {
    for (size_t a = 0; a < sekcije.size(); a++) {
        if (!sekcije[a].adresaDodeljena) continue;
        u32 pocetakA = sekcije[a].bazna_adresa;
        u32 krajA = pocetakA + static_cast<u32>(sekcije[a].sadrzaj.size());

        for (size_t b = a + 1; b < sekcije.size(); b++) {
            if (!sekcije[b].adresaDodeljena) continue;
            u32 pocetakB = sekcije[b].bazna_adresa;
            u32 krajB = pocetakB + static_cast<u32>(sekcije[b].sadrzaj.size());

            // Dva opsega se preklapaju ako pocetak jednog pada pre kraja drugog
            // i obrnuto (klasican test preseka poluotvorenih intervala).
            bool preklapaju = (pocetakA < krajB) && (pocetakB < krajA);
            if (preklapaju) {
                throw LinkerskaGreska{
                    "preklapanje sekcija '" + sekcije[a].ime + "' i '" +
                    sekcije[b].ime + "'"};
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Raspoređuje sekcije na adrese.
//   1) Sekcije navedene u -place dobijaju svoje fiksne adrese.
//   2) Proveri da se -place sekcije medjusobno ne preklapaju.
//   3) Ostale (podrazumevane) sekcije ređaju se redom, počev odmah iza
//      najvise zauzete adrese.
//   4) Zavrsna provera preklapanja svih sekcija.
// ----------------------------------------------------------------------------
void Linker::rasporedi() {
    // 1) Primeni -place direktive.
    for (const PlaceDirektiva& pd : placeDirektive) {
        SpojenaSekcija* s = nadjiSpojenu(pd.imeSekcije);
        if (s == nullptr) {
            throw LinkerskaGreska{
                "-place se odnosi na nepostojecu sekciju '" + pd.imeSekcije + "'"};
        }
        s->bazna_adresa = pd.adresa;
        s->adresaDodeljena = true;
    }

    // 2) Provera preklapanja medju vec fiksiranim (-place) sekcijama.
    proveriPreklapanja();

    // 3) Nadji najvisu zauzetu adresu (kraj sekcije koja seze najdalje).
    //    Podrazumevane sekcije ređamo počev odatle.
    u32 sledeca = 0;
    for (const SpojenaSekcija& s : sekcije) {
        if (s.adresaDodeljena) {
            u32 kraj = s.bazna_adresa + static_cast<u32>(s.sadrzaj.size());
            if (kraj > sledeca) sledeca = kraj;
        }
    }

    // Rasporedi podrazumevane sekcije (one bez -place) jednu za drugom.
    for (SpojenaSekcija& s : sekcije) {
        if (s.adresaDodeljena) continue;
        s.bazna_adresa = sledeca;
        s.adresaDodeljena = true;
        sledeca += static_cast<u32>(s.sadrzaj.size());
    }

    // 4) Zavrsna provera preklapanja svih sekcija.
    proveriPreklapanja();
}

// ----------------------------------------------------------------------------
// Pomocna: upisuje 32-bitnu vrednost na dati offset u sadrzaj spojene sekcije
// (little-endian).
// ----------------------------------------------------------------------------
static void upisiRec(std::vector<u8>& sadrzaj, u32 offset, u32 vrednost) {
    sadrzaj[offset + 0] = static_cast<u8>(vrednost & 0xFF);
    sadrzaj[offset + 1] = static_cast<u8>((vrednost >> 8) & 0xFF);
    sadrzaj[offset + 2] = static_cast<u8>((vrednost >> 16) & 0xFF);
    sadrzaj[offset + 3] = static_cast<u8>((vrednost >> 24) & 0xFF);
}

// ----------------------------------------------------------------------------
// Razresava sve relokacije. Radi iz originalnih programa jer su indeksi
// simbola u relokacijama lokalni za svoj fajl.
// ----------------------------------------------------------------------------
void Linker::razresiRelokacije() {
    for (size_t f = 0; f < programi.size(); f++) {
        const PredmetniProgram& prog = programi[f];

        for (const Sekcija& sek : prog.sekcije) {
            // Spojena sekcija u koju upisujemo i bazni offset ovog doprinosa.
            SpojenaSekcija* spojena = nadjiSpojenu(sek.ime);
            u32 bazniDoprinosa = bazniOffset[KljucDoprinosa{f, sek.ime}];

            for (const Relokacija& r : sek.relokacije) {
                // (1) Globalni offset u spojenoj sekciji gde upisujemo.
                u32 offsetUpisa = bazniDoprinosa + r.offset;

                // (2) Na sta pokazuje indeksSimbola u OVOM fajlu.
                const Simbol* cilj = prog.simbolPoIndeksu(r.indeksSimbola);
                if (cilj == nullptr) {
                    throw LinkerskaGreska{
                        "relokacija pokazuje na nepostojeci simbol u fajlu '" +
                        prog.imeDatoteke + "'"};
                }

                u32 vrednost = 0;

                if (cilj->vrsta == VrstaSimbola::SEKCIJA) {
                    // (3a) Lokalni slucaj: cilj je sekcija. Prava vrednost je
                    // adresa spojene sekcije + bazni offset doprinosa fajla toj
                    // sekciji + addend (offset simbola koji je asembler ostavio).
                    SpojenaSekcija* ciljSek = nadjiSpojenu(cilj->ime);
                    if (ciljSek == nullptr) {
                        throw LinkerskaGreska{
                            "relokacija na nepostojecu sekciju '" + cilj->ime + "'"};
                    }
                    u32 bazniCilj = bazniOffset[KljucDoprinosa{f, cilj->ime}];
                    vrednost = ciljSek->bazna_adresa + bazniCilj +
                               static_cast<u32>(r.addend);
                } else {
                    // (3b) Globalni/eksterni simbol: nadji ga u globalnoj tabeli.
                    GlobalniSimbol* g = nadjiGlobalni(cilj->ime);
                    if (g == nullptr || !g->definisan) {
                        throw LinkerskaGreska{
                            "nerazresen simbol '" + cilj->ime + "'"};
                    }
                    // Apsolutan simbol: vrednost je konstanta, bez sekcije.
                    if (g->apsolutan) {
                        upisiRec(spojena->sadrzaj, offsetUpisa,
                                 g->apsolutnaVrednost + static_cast<u32>(r.addend));
                        continue;
                    }
                    SpojenaSekcija* ciljSek = nadjiSpojenu(g->imeSekcije);
                    if (ciljSek == nullptr) {
                        throw LinkerskaGreska{
                            "simbol '" + cilj->ime + "' u nepostojecoj sekciji"};
                    }
                    vrednost = ciljSek->bazna_adresa + g->offsetUSekciji +
                               static_cast<u32>(r.addend);
                }

                // (4) Upisi izracunatu vrednost u spojenu sekciju.
                upisiRec(spojena->sadrzaj, offsetUpisa, vrednost);
            }
        }
    }
}

// ----------------------------------------------------------------------------
// Generise -relocatable izlaz: spojen predmetni program spreman za dalje
// linkovanje. Sekcije ostaju od nule; relokacije se prenose i prilagodjavaju.
// ----------------------------------------------------------------------------
void Linker::generisiRelocatable(std::vector<Simbol>& izlazniSimboli,
                                 std::vector<Sekcija>& izlazneSekcije) {
    izlazniSimboli.clear();
    izlazneSekcije.clear();

    // (1) Napravi izlazne sekcije sa spojenim sadrzajem i unos u tabeli simbola
    //     za svaku sekciju. Zapamti indeks svake sekcije u novoj tabeli.
    std::unordered_map<std::string, u32> indeksSekcijeUTabeli;

    for (const SpojenaSekcija& ss : sekcije) {
        // Unos sekcije u novu tabelu simbola.
        Simbol simSek;
        simSek.ime = ss.ime;
        simSek.redniBroj = static_cast<u32>(izlazniSimboli.size());
        simSek.vrsta = VrstaSimbola::SEKCIJA;
        simSek.jeDefinisan = true;
        simSek.indeksSekcije = simSek.redniBroj;
        simSek.vrednost = 0;
        indeksSekcijeUTabeli[ss.ime] = simSek.redniBroj;
        izlazniSimboli.push_back(simSek);

        // Izlazna sekcija sa spojenim sadrzajem (relokacije dodajemo nize).
        Sekcija sek;
        sek.ime = ss.ime;
        sek.indeksSimbola = simSek.redniBroj;
        sek.sadrzaj = ss.sadrzaj;
        izlazneSekcije.push_back(sek);
    }

    // (2) Dodaj globalne/eksterne simbole u novu tabelu i zapamti njihov indeks.
    std::unordered_map<std::string, u32> indeksSimbolaUTabeli;
    for (const GlobalniSimbol& gs : globalna) {
        Simbol sim;
        sim.ime = gs.ime;
        sim.redniBroj = static_cast<u32>(izlazniSimboli.size());
        sim.vrsta = VrstaSimbola::SIMBOL;
        sim.jeDefinisan = gs.definisan;
        sim.jeGlobalan = gs.definisan;      // definisan kod nas -> izvozimo ga
        sim.jeEksteran = !gs.definisan;     // nedefinisan -> ostaje eksteran
        if (gs.definisan) {
            sim.indeksSekcije = indeksSekcijeUTabeli[gs.imeSekcije];
            sim.vrednost = gs.offsetUSekciji;
        } else {
            sim.indeksSekcije = NEDEFINISANA_SEKCIJA;
            sim.vrednost = 0;
        }
        indeksSimbolaUTabeli[gs.ime] = sim.redniBroj;
        izlazniSimboli.push_back(sim);
    }

    // (3) Prilagodi i prenesi relokacije iz svih ulaznih fajlova.
    for (size_t f = 0; f < programi.size(); f++) {
        const PredmetniProgram& prog = programi[f];

        for (const Sekcija& sek : prog.sekcije) {
            u32 bazniDoprinosa = bazniOffset[KljucDoprinosa{f, sek.ime}];

            // Nadji izlaznu sekciju istog imena (u koju idu prilagodjene relok.).
            Sekcija* izlazSek = nullptr;
            for (Sekcija& s : izlazneSekcije) {
                if (s.ime == sek.ime) { izlazSek = &s; break; }
            }

            for (const Relokacija& r : sek.relokacije) {
                const Simbol* cilj = prog.simbolPoIndeksu(r.indeksSimbola);
                if (cilj == nullptr) continue;

                Relokacija nova;
                nova.tip = TipRelokacije::APSOLUTNA_32;
                // Novi offset = bazni offset doprinosa + stari offset.
                nova.offset = bazniDoprinosa + r.offset;

                if (cilj->vrsta == VrstaSimbola::SEKCIJA) {
                    // Lokalni slucaj: relokacija pokazuje na sekciju. Ciljni
                    // simbol se pomerio za bazni offset doprinosa te sekcije.
                    u32 bazniCilj = bazniOffset[KljucDoprinosa{f, cilj->ime}];
                    nova.indeksSimbola = indeksSekcijeUTabeli[cilj->ime];
                    nova.addend = r.addend + static_cast<i32>(bazniCilj);
                } else {
                    // Globalni/eksterni: pokazuje na simbol u novoj tabeli.
                    nova.indeksSimbola = indeksSimbolaUTabeli[cilj->ime];
                    nova.addend = r.addend;
                }

                izlazSek->relokacije.push_back(nova);
            }
        }
    }
}