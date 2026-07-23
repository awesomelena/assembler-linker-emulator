// ============================================================================
// asembler.hpp
//
// Klasa Asembler vodi ceo proces asembliranja jedne ulazne datoteke:
// cita liniju po liniju, tokenizuje je, prepoznaje labele/direktive/naredbe
// i puni tabelu simbola i sekcije. U ovoj fazi implementirane su labele i
// jednostavne direktive (.section, .global, .extern, .end); generisanje
// masinskog koda naredbi i direktiva sa sadrzajem dodaje se kasnije.
// ============================================================================

#ifndef ASEMBLER_HPP
#define ASEMBLER_HPP

#include "tipovi.hpp"
#include "tabela_simbola.hpp"
#include "sekcija.hpp"
#include "lekser.hpp"
#include "operand.hpp"
#include <string>
#include <vector>

// Izuzetak za greske u procesu asembliranja. Poruka opisuje sta je poslo po
// zlu; parser uz nju vezuje i broj linije radi jasne dijagnostike.
struct AsemblerskaGreska {
    std::string poruka;
};

class Asembler {
public:
    Asembler() = default;

    // Asemblira izvorni kod dat kao lista linija. Po zavrsetku, tabela simbola
    // i sekcije su popunjene. U slucaju greske baca AsemblerskaGreska.
    void asembliraj(const std::vector<std::string>& linije);

    // Pristup rezultatima (za kasniji ispis predmetnog programa i za testove).
    const TabelaSimbola& tabela() const { return tabelaSimbola; }
    const std::vector<Sekcija>& sveSekcije() const { return sekcije; }

    // Jedna .equ definicija cije se izracunavanje odlaze do kraja asembliranja.
    struct OdlozenaEqu {
        std::string ime;              // ime simbola koji se definise
        std::vector<Token> izraz;     // tokeni izraza (bez .equ, imena i zapete)
        size_t brojLinije;            // radi jasne poruke o gresci
    };

private:
    TabelaSimbola tabelaSimbola;
    std::vector<OdlozenaEqu> odlozeneEqu;
    std::vector<Sekcija> sekcije;

    // Indeks trenutno aktivne sekcije u vektoru sekcije, ili -1 ako nijedna
    // sekcija jos nije zapoceta (kod pre prve .section direktive).
    int trenutnaSekcija = -1;

    bool zavrseno = false;  // postaje true kada se naidje na .end

    // --- Pomocne metode ----------------------------------------------------

    // Obrada jedne vec tokenizovane linije.
    void obradiLiniju(const std::vector<Token>& tokeni);

    // Obrada direktive (token[0] je IDENT koji pocinje tackom).
    void obradiDirektivu(const std::vector<Token>& tokeni, size_t pocetak);

    // Obrada naredbe (instrukcije). Generise masinski kod u aktivnu sekciju.
    void obradiNaredbu(const std::vector<Token>& tokeni, size_t pocetak);

    // Obrada ld i st naredbi (sa svih osam nacina adresiranja).
    void obradiLd(const std::vector<Token>& tokeni, size_t& i);
    void obradiSt(const std::vector<Token>& tokeni, size_t& i);

    // Obrada skok/poziv naredbi (jmp, call, beq, bne, bgt).
    void obradiSkok(const std::vector<Token>& tokeni, size_t& i);

    // --- Pomocne metode za bazen literala i relokacije ---------------------

    // Dodaje literal (poznatu vrednost) u bazen aktivne sekcije i vraca indeks
    // te stavke u bazenu. Stavka ce biti upisana pri praznjenju bazena.
    size_t dodajLiteralUBazen(u32 vrednost);

    // Dodaje simbol u bazen aktivne sekcije (u bazen ide 0, a linkeru se kasnije
    // ostavlja relokacija). Vraca indeks stavke u bazenu.
    size_t dodajSimbolUBazen(const std::string& ime);

    // Prazni bazen aktivne sekcije: upisuje sve stavke iza koda, zakrpljuje
    // pomeraje u instrukcijama i pravi relokacije za simbolske stavke.
    void isprazniBazen();

    // Pravi relokaciju za dati simbol na datom offsetu u aktivnoj sekciji.
    // Bira scenario (globalni/eksterni -> simbol, addend 0; lokalni -> sekcija,
    // addend = offset simbola) na osnovu atributa simbola.
    void napraviRelokaciju(u32 offset, const std::string& imeSimbola);

    // Finalizuje sve relokacije na kraju asembliranja: za svaku odlucuje da li
    // pokazuje na simbol (globalni/eksterni) ili na sekciju (lokalni), sada
    // kada su svi simboli u fajlu poznati.
    void finalizujRelokacije();

    // Izracunava sve odlozene .equ definicije na kraju asembliranja, kada su
    // svi simboli u fajlu definisani. Time .equ moze da koristi simbole koji
    // su definisani kasnije u fajlu (npr. message_end - message_start).
    void izracunajEquDefinicije();

    // Definise labelu sa datim imenom na trenutnoj lokaciji.
    void definisiLabelu(const std::string& ime);

    // Zapocinje novu sekciju datog imena (i pravi joj unos u tabeli simbola).
    void zapocniSekciju(const std::string& ime);

    // Vraca pokazivac na trenutnu sekciju ili baca gresku ako nije zapoceta.
    Sekcija* aktivnaSekcija();
};

#endif // ASEMBLER_HPP