// ============================================================================
// linker.hpp
//
// Jezgro linkera: spajanje istoimenih sekcija iz vise predmetnih programa,
// izgradnja globalne tabele simbola, raspoređivanje na adrese i razresavanje
// relokacija.
//
// Ova faza pokriva spajanje sekcija i globalnu tabelu simbola. Raspoređivanje
// i relokacije dodaju se u narednim fazama.
// ============================================================================

#ifndef LINKER_HPP
#define LINKER_HPP

#include "tipovi.hpp"
#include "predmetni_program.hpp"
#include <string>
#include <vector>
#include <unordered_map>

// Izuzetak za greske u procesu linkovanja (visestruka definicija, nerazresen
// simbol, preklapanje sekcija...). Naziv simbola/sekcije je deo poruke.
struct LinkerskaGreska {
    std::string poruka;
};

// ----------------------------------------------------------------------------
// Spojena sekcija: konkatenacija svih istoimenih sekcija iz ulaznih fajlova.
// ----------------------------------------------------------------------------
struct SpojenaSekcija {
    std::string ime;
    std::vector<u8> sadrzaj;           // spojeni sadrzaj svih doprinosa
    u32 bazna_adresa = 0;              // adresa pocetka (dodeljuje se kasnije)
    bool adresaDodeljena = false;      // da li je bazna_adresa vec fiksirana
};

// ----------------------------------------------------------------------------
// Unos globalne tabele simbola: jedan globalni/eksterni simbol vidljiv medju
// fajlovima, sa svojom konacnom pozicijom.
// ----------------------------------------------------------------------------
struct GlobalniSimbol {
    std::string ime;
    bool definisan = false;            // da li je negde definisan
    std::string imeSekcije;            // sekcija u kojoj je definisan (ako jeste)
    u32 offsetUSekciji = 0;            // offset u SPOJENOJ sekciji

    // Apsolutan simbol (npr. definisan preko .equ) nema sekciju; njegova
    // vrednost je konstanta poznata jos u vreme asembliranja.
    bool apsolutan = false;
    u32 apsolutnaVrednost = 0;
    // Konacna adresa se racuna kao bazna_adresa(sekcija) + offsetUSekciji
    // tek nakon raspoređivanja.
};

// ----------------------------------------------------------------------------
// Kljuc za mapu baznih offseta: koji fajl i koja njegova sekcija.
// Vrednost je bazni offset tog doprinosa u spojenoj sekciji.
// ----------------------------------------------------------------------------
struct KljucDoprinosa {
    size_t indeksFajla;
    std::string imeSekcije;

    bool operator==(const KljucDoprinosa& d) const {
        return indeksFajla == d.indeksFajla && imeSekcije == d.imeSekcije;
    }
};

// Hash funkcija za KljucDoprinosa da bismo ga koristili u unordered_map.
struct HashDoprinosa {
    size_t operator()(const KljucDoprinosa& d) const {
        return std::hash<size_t>()(d.indeksFajla) ^
               (std::hash<std::string>()(d.imeSekcije) << 1);
    }
};

// ----------------------------------------------------------------------------
// Jedna -place direktiva: sekcija ide na fiksnu adresu.
// ----------------------------------------------------------------------------
struct PlaceDirektiva {
    std::string imeSekcije;
    u32 adresa;
};

// ----------------------------------------------------------------------------
// Linker: drzi ulazne programe i rezultate spajanja.
// ----------------------------------------------------------------------------
class Linker {
public:
    // Dodaje ucitani predmetni program na listu ulaza.
    void dodajProgram(const PredmetniProgram& program);

    // Dodaje -place direktivu (eksplicitna adresa sekcije).
    void dodajPlace(const std::string& imeSekcije, u32 adresa);

    // Spaja istoimene sekcije i gradi globalnu tabelu simbola.
    // Baca LinkerskaGreska u slucaju visestruke definicije simbola.
    void spoji();

    // Raspoređuje sekcije na adrese (prvo -place, pa podrazumevano).
    // Baca LinkerskaGreska u slucaju preklapanja sekcija.
    void rasporedi();

    // Razresava sve relokacije: za svaku upisuje izracunatu vrednost u sadrzaj
    // spojene sekcije. Baca LinkerskaGreska za nerazresen simbol.
    void razresiRelokacije();

    // Generise -relocatable izlaz: spojen predmetni program (isti format kao
    // izlaz asemblera) koji moze ponovo biti ulaz linkera. Sekcije ostaju od
    // nulte adrese, relokacije se prenose i prilagodjavaju spojenom prostoru.
    // Rezultat se vraca kroz izlazne parametre.
    void generisiRelocatable(std::vector<Simbol>& izlazniSimboli,
                             std::vector<Sekcija>& izlazneSekcije);

    // Pristup rezultatima (za testove i naredne faze).
    const std::vector<SpojenaSekcija>& spojeneSekcije() const { return sekcije; }
    const std::vector<GlobalniSimbol>& globalniSimboli() const { return globalna; }

private:
    std::vector<PredmetniProgram> programi;
    std::vector<SpojenaSekcija> sekcije;
    std::vector<GlobalniSimbol> globalna;
    std::vector<PlaceDirektiva> placeDirektive;

    // Bazni offset svakog (fajl, sekcija) doprinosa u spojenoj sekciji.
    std::unordered_map<KljucDoprinosa, u32, HashDoprinosa> bazniOffset;

    // Pomocne metode.
    SpojenaSekcija* nadjiSpojenu(const std::string& ime);
    GlobalniSimbol* nadjiGlobalni(const std::string& ime);
    void spojiSekcije();
    void izgradiGlobalnuTabelu();
    void proveriPreklapanja();
};

#endif // LINKER_HPP