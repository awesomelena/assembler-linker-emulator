// ============================================================================
// tabela_simbola.hpp
//
// Tabela simbola je centralna struktura asemblera. Cuva sve simbole (labele,
// simbole iz .global/.extern/.equ direktiva, kao i same sekcije) zajedno sa
// njihovim atributima potrebnim za asembliranje i kasnije za linkovanje.
// ============================================================================

#ifndef TABELA_SIMBOLA_HPP
#define TABELA_SIMBOLA_HPP

#include "tipovi.hpp"
#include <string>
#include <vector>
#include <unordered_map>

// ----------------------------------------------------------------------------
// Vrsta simbola: obican simbol (labela, .equ, .extern) ili sekcija.
// Sekcije se takodje vode u tabeli simbola jer se relokacije lokalnih simbola
// izrazavaju u odnosu na sekciju kojoj simbol pripada.
// ----------------------------------------------------------------------------
enum class VrstaSimbola {
    SIMBOL,   // labela, .equ simbol, .extern simbol
    SEKCIJA   // unos koji predstavlja jednu asemblersku sekciju
};

// ----------------------------------------------------------------------------
// Jedan red tabele simbola.
// ----------------------------------------------------------------------------
struct Simbol {
    std::string ime;        // ime simbola (npr. "start", "text", "my_counter")

    // Indeks sekcije kojoj simbol pripada (redni broj unosa sekcije u tabeli).
    // Za sekciju, ovo pokazuje na samu sebe. Za .extern simbol nema sekcije,
    // pa koristimo posebnu vrednost (videti NEDEFINISANA_SEKCIJA nize).
    u32 indeksSekcije = 0;

    // Vrednost simbola:
    //  - za labelu: offset (location counter) od pocetka svoje sekcije,
    //  - za .equ simbol: izracunata vrednost izraza,
    //  - za sekciju: 0 (pocetak sekcije je uvek offset 0 unutar sebe),
    //  - za .extern: nepoznato dok linker ne razresi (drzimo 0 kao rezervu).
    u32 vrednost = 0;

    bool jeDefinisan = false;  // da li je simbol stvarno dobio definiciju
    bool jeGlobalan  = false;  // .global - izvezen ka drugim fajlovima
    bool jeEksteran  = false;  // .extern - uvezen, definisan negde drugde

    VrstaSimbola vrsta = VrstaSimbola::SIMBOL;

    u32 redniBroj = 0;  // jedinstven indeks ovog simbola u tabeli
};

// Posebna vrednost za indeksSekcije kada simbol nije vezan ni za jednu sekciju
// (tipicno .extern simbol pre razresenja).
constexpr u32 NEDEFINISANA_SEKCIJA = 0xFFFFFFFF;

// ----------------------------------------------------------------------------
// Tabela simbola: cuva sve simbole i omogucava dodavanje i pretragu po imenu.
// ----------------------------------------------------------------------------
class TabelaSimbola {
public:
    TabelaSimbola() = default;

    // Vraca pokazivac na simbol sa datim imenom ili nullptr ako ne postoji.
    // Pokazivac je zgodan jer nam dozvoljava da kroz njega menjamo simbol
    // (npr. da naknadno oznacimo da je postao definisan ili globalan).
    Simbol* nadji(const std::string& ime);

    // Dodaje nov simbol sa datim imenom i vraca pokazivac na njega.
    // Pretpostavlja da simbol sa tim imenom jos ne postoji (pozivalac je
    // duzan da prvo proveri preko nadji ako to treba).
    Simbol* dodaj(const std::string& ime);

    // Zgodna skracenica: ako simbol postoji, vrati ga; inace ga napravi.
    Simbol* nadjiIliDodaj(const std::string& ime);

    // Pristup svim simbolima (npr. radi ispisa u predmetni program).
    const std::vector<Simbol>& sviSimboli() const { return simboli; }

private:
    // Simboli se cuvaju u vektoru (zadrzavaju redosled dodavanja i redni broj
    // odgovara poziciji). Mapa ime->indeks sluzi za brzu pretragu po imenu.
    std::vector<Simbol> simboli;
    std::unordered_map<std::string, u32> imeUIndeks;
};

#endif // TABELA_SIMBOLA_HPP