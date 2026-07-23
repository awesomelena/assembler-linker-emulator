// ============================================================================
// parser.cpp
//
// Implementacija glavne logike asembliranja: citanje linija, prepoznavanje
// labela, direktiva i naredbi, i punjenje tabele simbola i sekcija.
//
// Napomena o organizaciji: klasa Asembler je deklarisana u asembler.hpp.
// Njenu logiku parsiranja drzimo u ovom fajlu (parser.cpp), a ulaznu tacku
// programa (main) u src/asembler.cpp. Tako je logika odvojena od pokretanja.
// ============================================================================

#include "asembler.hpp"
#include <map>

// ----------------------------------------------------------------------------
// Vraca aktivnu sekciju ili baca gresku ako nijedna nije zapoceta.
// ----------------------------------------------------------------------------
Sekcija* Asembler::aktivnaSekcija() {
    if (trenutnaSekcija < 0) {
        throw AsemblerskaGreska{"sadrzaj se pojavljuje pre prve .section direktive"};
    }
    return &sekcije[trenutnaSekcija];
}

// ----------------------------------------------------------------------------
// Zapocinje novu sekciju: pravi joj unos u tabeli simbola i u vektoru sekcija,
// pa je postavlja kao trenutno aktivnu. Time se prethodna sekcija automatski
// "zavrsava" (samo prestajemo da pisemo u nju).
// ----------------------------------------------------------------------------
void Asembler::zapocniSekciju(const std::string& ime) {
    // Pre otvaranja nove sekcije, isprazni bazen literala prethodne sekcije
    // (ako je neka bila aktivna). Time se bazen svake sekcije nalazi tacno na
    // njenom kraju, iza generisanog koda.
    if (trenutnaSekcija >= 0) {
        isprazniBazen();
    }

    // Sekcija je istovremeno i simbol u tabeli simbola. Ako vec postoji simbol
    // sa tim imenom koji nije sekcija, to je greska (sudar imena).
    Simbol* postojeci = tabelaSimbola.nadji(ime);
    if (postojeci != nullptr && postojeci->vrsta != VrstaSimbola::SEKCIJA) {
        throw AsemblerskaGreska{"ime sekcije '" + ime + "' vec je upotrebljeno kao simbol"};
    }

    // Napravi novu sekciju.
    Sekcija s;
    s.ime = ime;

    // Napravi (ili pronadji) odgovarajuci unos u tabeli simbola.
    Simbol* sim = tabelaSimbola.nadjiIliDodaj(ime);
    sim->vrsta = VrstaSimbola::SEKCIJA;
    sim->jeDefinisan = true;
    sim->vrednost = 0;                 // pocetak sekcije je offset 0 unutar sebe
    s.indeksSimbola = sim->redniBroj;
    sim->indeksSekcije = sim->redniBroj; // sekcija pripada samoj sebi

    sekcije.push_back(s);
    trenutnaSekcija = static_cast<int>(sekcije.size()) - 1;
}

// ----------------------------------------------------------------------------
// Definise labelu na trenutnoj lokaciji u aktivnoj sekciji.
// ----------------------------------------------------------------------------
void Asembler::definisiLabelu(const std::string& ime) {
    Sekcija* sek = aktivnaSekcija();

    Simbol* sim = tabelaSimbola.nadjiIliDodaj(ime);

    // Dvostruka definicija iste labele je greska.
    if (sim->jeDefinisan) {
        throw AsemblerskaGreska{"visestruka definicija simbola '" + ime + "'"};
    }
    // Labela ne sme da bude prethodno proglasena eksternom.
    if (sim->jeEksteran) {
        throw AsemblerskaGreska{"simbol '" + ime + "' je .extern pa ne moze biti definisan ovde"};
    }

    sim->jeDefinisan = true;
    sim->vrsta = VrstaSimbola::SIMBOL;
    sim->indeksSekcije = sek->indeksSimbola;  // pripada trenutnoj sekciji
    sim->vrednost = sek->lokacija();          // offset = trenutni location counter
}

// ----------------------------------------------------------------------------
// Obrada direktive. token[pocetak] je IDENT cije ime pocinje tackom.
// ----------------------------------------------------------------------------
void Asembler::obradiDirektivu(const std::vector<Token>& tokeni, size_t pocetak) {
    const std::string& ime = tokeni[pocetak].tekst;

    if (ime == ".section") {
        // Ocekujemo tacno jedan IDENT kao ime sekcije.
        if (pocetak + 1 >= tokeni.size() || tokeni[pocetak + 1].vrsta != VrstaTokena::IDENT) {
            throw AsemblerskaGreska{".section zahteva ime sekcije"};
        }
        zapocniSekciju(tokeni[pocetak + 1].tekst);
        return;
    }

    if (ime == ".global" || ime == ".extern") {
        // Oba primaju listu simbola razdvojenih zapetama.
        // Ocekivani oblik: .global sim1, sim2, sim3
        bool globalan = (ime == ".global");
        size_t i = pocetak + 1;
        if (i >= tokeni.size() || tokeni[i].vrsta == VrstaTokena::KRAJ) {
            throw AsemblerskaGreska{ime + " zahteva bar jedan simbol"};
        }
        while (i < tokeni.size() && tokeni[i].vrsta != VrstaTokena::KRAJ) {
            if (tokeni[i].vrsta != VrstaTokena::IDENT) {
                throw AsemblerskaGreska{ime + " ocekuje ime simbola"};
            }
            Simbol* sim = tabelaSimbola.nadjiIliDodaj(tokeni[i].tekst);
            if (globalan) {
                sim->jeGlobalan = true;
            } else {
                sim->jeEksteran = true;
                // Eksteran simbol nema svoju sekciju dok ga linker ne razresi.
                sim->indeksSekcije = NEDEFINISANA_SEKCIJA;
            }
            i++;
            // Iza simbola sledi ili zapeta (pa jos simbola) ili kraj.
            if (i < tokeni.size() && tokeni[i].vrsta == VrstaTokena::ZAPETA) {
                i++;  // preskoci zapetu i nastavi na sledeci simbol
                if (i >= tokeni.size() || tokeni[i].vrsta == VrstaTokena::KRAJ) {
                    throw AsemblerskaGreska{ime + ": nedostaje simbol posle zapete"};
                }
            }
        }
        return;
    }

    if (ime == ".end") {
        zavrseno = true;
        return;
    }

    if (ime == ".skip") {
        // .skip <literal> - rezervisi toliko bajtova i popuni ih nulama.
        if (pocetak + 1 >= tokeni.size() || tokeni[pocetak + 1].vrsta != VrstaTokena::BROJ) {
            throw AsemblerskaGreska{".skip zahteva brojcani literal"};
        }
        i32 broj = tokeni[pocetak + 1].broj;
        if (broj < 0) {
            throw AsemblerskaGreska{".skip ne moze imati negativan argument"};
        }
        Sekcija* sek = aktivnaSekcija();
        for (i32 k = 0; k < broj; k++) {
            sek->dodajBajt(0);
        }
        return;
    }

    if (ime == ".word") {
        // .word <lista> - za svaki element dodaj 4 bajta.
        // U ovoj fazi podrzavamo literale; simbole dodajemo uz relokacije.
        Sekcija* sek = aktivnaSekcija();
        size_t i = pocetak + 1;
        if (i >= tokeni.size() || tokeni[i].vrsta == VrstaTokena::KRAJ) {
            throw AsemblerskaGreska{".word zahteva bar jedan inicijalizator"};
        }
        while (i < tokeni.size() && tokeni[i].vrsta != VrstaTokena::KRAJ) {
            if (tokeni[i].vrsta == VrstaTokena::BROJ) {
                sek->dodajRec(static_cast<u32>(tokeni[i].broj));
            } else if (tokeni[i].vrsta == VrstaTokena::IDENT) {
                // Simbol kao inicijalizator: upisi 0 i napravi relokaciju koja
                // ce linkeru reci da tu upise vrednost (adresu) simbola.
                u32 offset = sek->lokacija();
                sek->dodajRec(0);
                napraviRelokaciju(offset, tokeni[i].tekst);
            } else {
                throw AsemblerskaGreska{".word ocekuje literal ili simbol"};
            }
            i++;
            if (i < tokeni.size() && tokeni[i].vrsta == VrstaTokena::ZAPETA) {
                i++;
                if (i >= tokeni.size() || tokeni[i].vrsta == VrstaTokena::KRAJ) {
                    throw AsemblerskaGreska{".word: nedostaje vrednost posle zapete"};
                }
            }
        }
        return;
    }

    if (ime == ".ascii") {
        // .ascii <string> - alocira po jedan bajt za svaki karakter stringa i
        // inicijalizuje ga ASCII vrednoscu tog karaktera.
        if (pocetak + 1 >= tokeni.size() || tokeni[pocetak + 1].vrsta != VrstaTokena::STRING) {
            throw AsemblerskaGreska{".ascii zahteva string u navodnicima"};
        }
        Sekcija* sek = aktivnaSekcija();
        const std::string& tekst = tokeni[pocetak + 1].tekst;
        for (char c : tekst) {
            sek->dodajBajt(static_cast<u8>(c));
        }
        return;
    }

    if (ime == ".equ") {
        // .equ <novi_simbol>, <izraz>
        // Izracunavanje se ODLAZE do kraja asembliranja, jer izraz sme da
        // koristi simbole definisane kasnije u fajlu (npr. razliku dve labele
        // koje slede iza ove direktive).
        size_t i = pocetak + 1;
        if (i >= tokeni.size() || tokeni[i].vrsta != VrstaTokena::IDENT) {
            throw AsemblerskaGreska{".equ zahteva ime novog simbola"};
        }
        OdlozenaEqu odlozena;
        odlozena.ime = tokeni[i].tekst;
        i++;

        if (i >= tokeni.size() || tokeni[i].vrsta != VrstaTokena::ZAPETA) {
            throw AsemblerskaGreska{".equ zahteva zapetu izmedju simbola i izraza"};
        }
        i++;

        // Sacuvaj tokene izraza do kraja linije.
        while (i < tokeni.size() && tokeni[i].vrsta != VrstaTokena::KRAJ) {
            odlozena.izraz.push_back(tokeni[i]);
            i++;
        }
        if (odlozena.izraz.empty()) {
            throw AsemblerskaGreska{".equ zahteva izraz"};
        }

        odlozeneEqu.push_back(odlozena);
        return;
    }

    // Direktiva .equ (nivo C) dodaje se u kasnijoj fazi.
    throw AsemblerskaGreska{"direktiva '" + ime + "' jos nije implementirana u ovoj fazi"};
}

// ----------------------------------------------------------------------------
// Obrada jedne tokenizovane linije: prepoznaje labelu, pa direktivu ili
// naredbu u ostatku linije.
// ----------------------------------------------------------------------------
void Asembler::obradiLiniju(const std::vector<Token>& tokeni) {
    // Prazna linija: samo KRAJ token.
    if (tokeni.empty() || tokeni[0].vrsta == VrstaTokena::KRAJ) {
        return;
    }

    size_t i = 0;

    // 1) Opciona labela na pocetku: IDENT pa DVOTACKA.
    if (tokeni.size() >= 2 &&
        tokeni[0].vrsta == VrstaTokena::IDENT &&
        tokeni[1].vrsta == VrstaTokena::DVOTACKA) {
        definisiLabelu(tokeni[0].tekst);
        i = 2;  // predji preko "ime :" i nastavi da gledas ostatak linije
    }

    // Posle labele linija se moze zavrsiti (labela stoji sama).
    if (i >= tokeni.size() || tokeni[i].vrsta == VrstaTokena::KRAJ) {
        return;
    }

    // 2) Ostatak linije je direktiva ili naredba - oba pocinju IDENT-om.
    if (tokeni[i].vrsta != VrstaTokena::IDENT) {
        throw AsemblerskaGreska{"ocekivana direktiva ili naredba"};
    }

    const std::string& ime = tokeni[i].tekst;
    if (!ime.empty() && ime[0] == '.') {
        // Direktiva (ime pocinje tackom).
        obradiDirektivu(tokeni, i);
    } else {
        // Naredba (instrukcija) - generise masinski kod.
        obradiNaredbu(tokeni, i);
    }
}

// ----------------------------------------------------------------------------
// Glavna petlja: prolazi kroz sve linije, tokenizuje i obradjuje svaku.
// ----------------------------------------------------------------------------
void Asembler::asembliraj(const std::vector<std::string>& linije) {
    for (size_t brLinije = 0; brLinije < linije.size(); brLinije++) {
        // .end zaustavlja asembliranje - ostatak datoteke se odbacuje.
        if (zavrseno) {
            break;
        }

        try {
            std::vector<Token> tokeni = tokenizuj(linije[brLinije]);
            obradiLiniju(tokeni);
        } catch (const LeksickaGreska& g) {
            // Leksicku gresku "obogatimo" brojem linije i prosledimo dalje.
            throw AsemblerskaGreska{
                "linija " + std::to_string(brLinije + 1) + ": " + g.poruka};
        } catch (const AsemblerskaGreska& g) {
            // Ako poruka vec ne sadrzi broj linije, dodajemo ga.
            throw AsemblerskaGreska{
                "linija " + std::to_string(brLinije + 1) + ": " + g.poruka};
        }
    }

    // Na kraju asembliranja isprazni bazen poslednje aktivne sekcije. Ovo
    // pokriva i slucaj kada izvorni kod nema eksplicitnu .end direktivu.
    if (trenutnaSekcija >= 0) {
        isprazniBazen();
    }

    // Izracunaj odlozene .equ definicije (sada su svi simboli poznati).
    izracunajEquDefinicije();

    // Finalizuj sve relokacije sada kada su svi simboli u fajlu definisani.
    finalizujRelokacije();
}

// ----------------------------------------------------------------------------
// Izracunava sve odlozene .equ definicije. Poziva se na kraju asembliranja,
// kada su svi simboli u fajlu definisani.
//
// Izraz je niz clanova razdvojenih operatorima + i -, gde clan moze biti
// literal ili simbol. Da bi rezultat bio konstanta, doprinosi sekcija moraju
// da se ponisti: "kraj - pocetak" (oba iz iste sekcije) jeste konstanta, dok
// "kraj + pocetak" nije. Zato uz numericku vrednost pratimo i "tezinu" svake
// sekcije: broj pojava sa plusom umanjen za broj pojava sa minusom. Na kraju
// sve tezine moraju biti nula.
// ----------------------------------------------------------------------------
void Asembler::izracunajEquDefinicije() {
    // Vise prolaza: .equ sme da koristi drugi .equ simbol, koji je mozda jos
    // neizracunat. Ponavljamo dok ima napretka.
    std::vector<bool> obradjena(odlozeneEqu.size(), false);
    size_t preostalo = odlozeneEqu.size();

    while (preostalo > 0) {
        size_t obradjenoUProlazu = 0;

        for (size_t k = 0; k < odlozeneEqu.size(); k++) {
            if (obradjena[k]) continue;

            const OdlozenaEqu& eq = odlozeneEqu[k];

            i32 vrednost = 0;
            std::map<u32, int> tezinaSekcije;  // indeks sekcije -> tezina
            int znak = 1;
            bool sviSimboliPoznati = true;

            size_t i = 0;
            while (i < eq.izraz.size()) {
                const Token& t = eq.izraz[i];

                if (t.vrsta == VrstaTokena::BROJ) {
                    vrednost += znak * t.broj;
                    i++;
                } else if (t.vrsta == VrstaTokena::IDENT) {
                    Simbol* sim = tabelaSimbola.nadji(t.tekst);
                    if (sim == nullptr || !sim->jeDefinisan) {
                        sviSimboliPoznati = false;
                        break;  // pokusacemo u narednom prolazu
                    }
                    vrednost += znak * static_cast<i32>(sim->vrednost);
                    // Simbol vezan za sekciju nosi doprinos te sekcije.
                    // Apsolutni simboli (npr. iz drugog .equ) nemaju doprinos.
                    if (sim->vrsta != VrstaSimbola::SEKCIJA &&
                        sim->indeksSekcije != NEDEFINISANA_SEKCIJA) {
                        tezinaSekcije[sim->indeksSekcije] += znak;
                    }
                    i++;
                } else {
                    throw AsemblerskaGreska{
                        ".equ '" + eq.ime + "': u izrazu se ocekuje literal ili simbol"};
                }

                // Iza clana sledi operator (+/-) pa novi clan, ili kraj izraza.
                if (i < eq.izraz.size() && eq.izraz[i].vrsta == VrstaTokena::PLUS) {
                    znak = 1;
                    i++;
                } else if (i < eq.izraz.size() && eq.izraz[i].vrsta == VrstaTokena::MINUS) {
                    znak = -1;
                    i++;
                } else {
                    break;
                }
            }

            if (!sviSimboliPoznati) {
                continue;  // sacekaj naredni prolaz
            }

            // Doprinosi sekcija moraju da se ponisti da bi izraz bio konstanta.
            for (const auto& par : tezinaSekcije) {
                if (par.second != 0) {
                    throw AsemblerskaGreska{
                        ".equ '" + eq.ime +
                        "': izraz nije konstanta (doprinosi sekcija se ne ponistavaju)"};
                }
            }

            // Definisi simbol kao apsolutan (ne pripada nijednoj sekciji).
            Simbol* novi = tabelaSimbola.nadjiIliDodaj(eq.ime);
            if (novi->jeDefinisan) {
                throw AsemblerskaGreska{"visestruka definicija simbola '" + eq.ime + "'"};
            }
            novi->jeDefinisan = true;
            novi->vrsta = VrstaSimbola::SIMBOL;
            novi->vrednost = static_cast<u32>(vrednost);
            novi->indeksSekcije = NEDEFINISANA_SEKCIJA;

            obradjena[k] = true;
            obradjenoUProlazu++;
            preostalo--;
        }

        // Ako u celom prolazu nista nije obradjeno, preostale definicije
        // zavise od simbola koji nikada nece biti definisani.
        if (obradjenoUProlazu == 0) {
            for (size_t k = 0; k < odlozeneEqu.size(); k++) {
                if (!obradjena[k]) {
                    throw AsemblerskaGreska{
                        ".equ '" + odlozeneEqu[k].ime +
                        "': izraz koristi simbol koji nije definisan"};
                }
            }
            break;
        }
    }
}