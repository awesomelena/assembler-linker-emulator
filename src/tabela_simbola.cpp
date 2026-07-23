// ============================================================================
// tabela_simbola.cpp
//
// Implementacija metoda klase TabelaSimbola.
// ============================================================================

#include "tabela_simbola.hpp"

Simbol* TabelaSimbola::nadji(const std::string& ime) {
    auto it = imeUIndeks.find(ime);
    if (it == imeUIndeks.end()) {
        return nullptr;  // nema simbola sa tim imenom
    }
    // it->second je indeks u vektoru simboli.
    return &simboli[it->second];
}

Simbol* TabelaSimbola::dodaj(const std::string& ime) {
    // Redni broj novog simbola je trenutna velicina vektora (0, 1, 2, ...).
    u32 indeks = static_cast<u32>(simboli.size());

    Simbol s;
    s.ime = ime;
    s.redniBroj = indeks;
    simboli.push_back(s);

    // Zapamti gde se simbol nalazi radi kasnije brze pretrage po imenu.
    imeUIndeks[ime] = indeks;

    // VAZNO: vracamo pokazivac na element unutar vektora. To je bezbedno samo
    // dokle god se vektor kasnije ne "preseli" u memoriji (sto se desava kada
    // naraste preko svog kapaciteta pri push_back). Zato pozivalac NE sme da
    // cuva ovaj pokazivac preko narednih poziva dodaj(). Za trajno referisanje
    // koristimo redniBroj (indeks), koji ostaje validan uvek.
    return &simboli[indeks];
}

Simbol* TabelaSimbola::nadjiIliDodaj(const std::string& ime) {
    Simbol* postojeci = nadji(ime);
    if (postojeci != nullptr) {
        return postojeci;
    }
    return dodaj(ime);
}