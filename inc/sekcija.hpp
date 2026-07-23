// ============================================================================
// sekcija.hpp
//
// Sekcija je imenovan, kontinualan blok bajtova u predmetnom programu (npr.
// kod ili podaci). Pored samog sadrzaja, sekcija vodi svoj location counter
// (koliko je bajtova do sada generisano) i, kasnije, relokacione zapise.
// ============================================================================

#ifndef SEKCIJA_HPP
#define SEKCIJA_HPP

#include "tipovi.hpp"
#include "relokacija.hpp"
#include <string>
#include <vector>

// ----------------------------------------------------------------------------
// Jedna stavka bazena literala: vrednost koju treba smestiti na kraj sekcije
// (kao 4 bajta) da bi joj instrukcija pristupila PC-relativno.
//
// Stavka moze biti:
//  - cist literal (jeSimbol == false): u bazen ide "vrednost" direktno,
//  - simbol (jeSimbol == true): u bazen ide 0, a linkeru se ostavlja
//    relokacija koja ce upisati pravu adresu simbola.
// ----------------------------------------------------------------------------
struct StavkaBazena {
    bool jeSimbol;          // da li stavka nosi simbol (true) ili cist literal
    u32 vrednost;           // za literal: sama vrednost; za simbol: nebitno (0)
    std::string imeSimbola; // za simbol: ime (razresava se pri praznjenju bazena)

    // Offset u sekciji na kome ce stavka biti smestena kad se bazen isprazni.
    // Popunjava se tek pri praznjenju bazena (na kraju sekcije).
    u32 offsetUBazenu = 0;
};

// ----------------------------------------------------------------------------
// Jedna zakrpa (patch): instrukcija na datom offsetu treba u svom 12-bitnom
// polju pomeraja da dobije rastojanje do svoje stavke u bazenu.
//
// Pomeraj je PC-relativan: racuna se u odnosu na adresu SLEDECE instrukcije
// (pc vec pokazuje na sledecu instrukciju u trenutku izvrsavanja tekuce).
// ----------------------------------------------------------------------------
struct Zakrpa {
    u32 offsetInstrukcije;   // gde je instrukcija ciji pomeraj treba popraviti
    size_t indeksStavke;     // koja stavka bazena je cilj (indeks u vektoru bazen)
};

// ----------------------------------------------------------------------------
// Jedna sekcija.
// ----------------------------------------------------------------------------
struct Sekcija {
    std::string ime;      // ime sekcije (npr. "text", "data", "my_code")
    u32 indeksSimbola = 0; // redni broj ove sekcije u tabeli simbola

    // Sadrzaj sekcije - niz generisanih bajtova. Location counter je uvek
    // jednak trenutnoj velicini ovog vektora (sadrzaj.size()), pa nam ne treba
    // zaseban brojac: dodavanje bajta automatski pomera "lokaciju" za jedan.
    std::vector<u8> sadrzaj;

    // Relokacioni zapisi vezani za ovu sekciju.
    std::vector<Relokacija> relokacije;

    // Bazen literala: stavke koje cekaju da budu upisane na kraj sekcije.
    std::vector<StavkaBazena> bazen;

    // Patch-lista: instrukcije kojima treba zakrpiti pomeraj do bazena.
    std::vector<Zakrpa> zakrpe;

    // Trenutni location counter = koliko bajtova je do sada u sekciji.
    // Ovo je offset na koji ce se smestiti sledeci generisani bajt.
    u32 lokacija() const { return static_cast<u32>(sadrzaj.size()); }

    // Dodaje jedan bajt na kraj sekcije.
    void dodajBajt(u8 b) { sadrzaj.push_back(b); }

    // Dodaje 32-bitnu vrednost u little-endian rasporedu (najnizi bajt prvi),
    // sto odgovara rasporedu bajtova ciljne arhitekture.
    void dodajRec(u32 vrednost) {
        dodajBajt(static_cast<u8>(vrednost & 0xFF));
        dodajBajt(static_cast<u8>((vrednost >> 8) & 0xFF));
        dodajBajt(static_cast<u8>((vrednost >> 16) & 0xFF));
        dodajBajt(static_cast<u8>((vrednost >> 24) & 0xFF));
    }

    // Upisuje 32-bitnu vrednost na vec postojecu poziciju u sadrzaju (za zakrpe).
    void upisiRecNa(u32 offset, u32 vrednost) {
        sadrzaj[offset + 0] = static_cast<u8>(vrednost & 0xFF);
        sadrzaj[offset + 1] = static_cast<u8>((vrednost >> 8) & 0xFF);
        sadrzaj[offset + 2] = static_cast<u8>((vrednost >> 16) & 0xFF);
        sadrzaj[offset + 3] = static_cast<u8>((vrednost >> 24) & 0xFF);
    }
};

#endif // SEKCIJA_HPP