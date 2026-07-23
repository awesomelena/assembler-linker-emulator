// ============================================================================
// lekser.cpp
//
// Implementacija leksera. Skenira liniju karakter po karakter i gradi tokene.
// ============================================================================

#include "lekser.hpp"
#include <cctype>    // za isspace, isalpha, isdigit, isalnum
#include <stdexcept>

// Pomocne funkcije za proveru kategorije karaktera.
// Izdvajamo ih radi citljivosti i da na jednom mestu definisemo sta se smatra
// pocetkom identifikatora a sta njegovim nastavkom.
//
// Pocetak identifikatora je slovo, donja crta ili tacka. Tacku dozvoljavamo
// zbog asemblerskih direktiva koje njome pocinju (.word, .global, .ascii...),
// pa ih tako lekser vidi kao jedan token (npr. IDENT(".word")), a parser
// prepoznaje direktivu jednostavno po tome sto ime pocinje tackom.
static bool jePocetakIdentifikatora(char c) {
    return std::isalpha(static_cast<unsigned char>(c)) || c == '_' || c == '.';
}

static bool jeNastavakIdentifikatora(char c) {
    return std::isalnum(static_cast<unsigned char>(c)) || c == '_';
}

// Cita ceo identifikator pocev od pozicije i. Po zavrsetku, i pokazuje na
// prvi karakter koji vise nije deo identifikatora.
static std::string procitajIdentifikator(const std::string& s, size_t& i) {
    size_t pocetak = i;
    // Prvi karakter je vec proveren kao pocetak identifikatora (slovo, _ ili .),
    // pa ga bezuslovno uzimamo. Time garantujemo napredak i kada je prvi znak
    // tacka (npr. ".word"), koja nije "nastavak" identifikatora.
    i++;
    while (i < s.size() && jeNastavakIdentifikatora(s[i])) {
        i++;
    }
    return s.substr(pocetak, i - pocetak);
}

// Cita celobrojni literal (decimalni ili heksadecimalni, uz opcioni predznak -).
// Vraca token vrste BROJ. Po zavrsetku, i pokazuje iza poslednje cifre.
static Token procitajBroj(const std::string& s, size_t& i) {
    size_t pocetak = i;

    // Opcioni predznak minus (npr. za .word -1 ili negativan pomeraj).
    if (s[i] == '-') {
        i++;
    }

    // Heksadecimalni zapis pocinje sa 0x ili 0X.
    bool heksadecimalno = false;
    if (i + 1 < s.size() && s[i] == '0' && (s[i + 1] == 'x' || s[i + 1] == 'X')) {
        heksadecimalno = true;
        i += 2;  // preskoci "0x"
        while (i < s.size() && std::isxdigit(static_cast<unsigned char>(s[i]))) {
            i++;
        }
    } else {
        // Decimalni zapis: niz cifara.
        while (i < s.size() && std::isdigit(static_cast<unsigned char>(s[i]))) {
            i++;
        }
    }

    std::string tekst = s.substr(pocetak, i - pocetak);

    // Pretvaramo tekst u broj. std::stol/std::stoll biraju osnovu 0 kada joj
    // eksplicitno kazemo (prepoznaje 0x prefiks), ali mi vec znamo osnovu.
    Token t;
    t.vrsta = VrstaTokena::BROJ;
    t.tekst = tekst;
    try {
        // Osnova 16 za heks, 10 za decimalni. stol podnosi predznak minus.
        t.broj = static_cast<i32>(std::stol(tekst, nullptr, heksadecimalno ? 16 : 10));
    } catch (const std::exception&) {
        throw LeksickaGreska{"neispravan brojcani literal: " + tekst};
    }
    return t;
}

std::vector<Token> tokenizuj(const std::string& linija) {
    std::vector<Token> tokeni;
    size_t i = 0;
    const size_t n = linija.size();

    while (i < n) {
        char c = linija[i];

        // 1) Preskoci bele znakove.
        if (std::isspace(static_cast<unsigned char>(c))) {
            i++;
            continue;
        }

        // 2) Komentar: sve od # do kraja linije se ignorise.
        if (c == '#') {
            break;
        }

        // 3) Identifikator (ime instrukcije, direktive, labele, simbola).
        if (jePocetakIdentifikatora(c)) {
            Token t;
            t.vrsta = VrstaTokena::IDENT;
            t.tekst = procitajIdentifikator(linija, i);
            tokeni.push_back(t);
            continue;
        }

        // 4) Broj: cifra, ili minus iza kojeg sledi cifra.
        //    Minus je PREDZNAK broja samo ako ne dolazi neposredno posle necega
        //    sto zavrsava vrednost (broja, imena ili zatvorene zagrade). U tom
        //    slucaju je operator oduzimanja, npr. u izrazu "kraj - pocetak".
        bool prethodniZavrsavaVrednost =
            !tokeni.empty() &&
            (tokeni.back().vrsta == VrstaTokena::BROJ ||
             tokeni.back().vrsta == VrstaTokena::IDENT ||
             tokeni.back().vrsta == VrstaTokena::UGL_ZATV);

        if (std::isdigit(static_cast<unsigned char>(c)) ||
            (c == '-' && !prethodniZavrsavaVrednost &&
             i + 1 < n && std::isdigit(static_cast<unsigned char>(linija[i + 1])))) {
            tokeni.push_back(procitajBroj(linija, i));
            continue;
        }

        // 5) Registar: % pa ime registra.
        if (c == '%') {
            i++;  // preskoci %
            if (i >= n || !jePocetakIdentifikatora(linija[i])) {
                throw LeksickaGreska{"ocekivano ime registra iza %"};
            }
            Token t;
            t.vrsta = VrstaTokena::REGISTAR;
            t.tekst = procitajIdentifikator(linija, i);  // ime bez %
            tokeni.push_back(t);
            continue;
        }

        // 6) String literal za .ascii: "..." .
        if (c == '"') {
            i++;  // preskoci pocetni navodnik
            std::string sadrzaj;
            while (i < n && linija[i] != '"') {
                sadrzaj += linija[i];
                i++;
            }
            if (i >= n) {
                throw LeksickaGreska{"nezatvoren string (nedostaje navodnik)"};
            }
            i++;  // preskoci zavrsni navodnik
            Token t;
            t.vrsta = VrstaTokena::STRING;
            t.tekst = sadrzaj;
            tokeni.push_back(t);
            continue;
        }

        // 7) Interpunkcija - svaki znak je token za sebe.
        Token t;
        t.tekst = std::string(1, c);
        switch (c) {
            case ',': t.vrsta = VrstaTokena::ZAPETA;   break;
            case ':': t.vrsta = VrstaTokena::DVOTACKA; break;
            case '[': t.vrsta = VrstaTokena::UGL_OTV;  break;
            case ']': t.vrsta = VrstaTokena::UGL_ZATV; break;
            case '$': t.vrsta = VrstaTokena::DOLAR;    break;
            case '+': t.vrsta = VrstaTokena::PLUS;     break;
            case '-': t.vrsta = VrstaTokena::MINUS;    break;
            default:
                throw LeksickaGreska{std::string("nepoznat karakter: ") + c};
        }
        tokeni.push_back(t);
        i++;
    }

    // Svaka linija se zavrsava markerom KRAJ - parseru olaksava da zna granicu.
    Token kraj;
    kraj.vrsta = VrstaTokena::KRAJ;
    tokeni.push_back(kraj);

    return tokeni;
}