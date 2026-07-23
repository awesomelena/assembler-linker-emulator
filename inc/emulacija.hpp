// ============================================================================
// emulacija.hpp
//
// Emulaciona petlja: fetch-decode-execute ciklus koji izvrsava program iz
// memorije pocev od adrese 0x40000000 do halt instrukcije.
// ============================================================================

#ifndef EMULACIJA_HPP
#define EMULACIJA_HPP

#include "procesor.hpp"
#include "memorija.hpp"
#include <string>

// Pocetna adresa izvrsavanja nakon reseta (iz priloga).
constexpr u32 POCETNA_ADRESA = 0x40000000;

// Izuzetak za greske u toku emulacije (npr. nekorektna instrukcija).
struct EmulacionaGreska {
    std::string poruka;
};

// Pokrece emulaciju: postavlja pc na pocetnu adresu i vrti petlju do halt-a.
// Baca EmulacionaGreska u slucaju nekorektne instrukcije.
void emuliraj(Procesor& cpu, Memorija& mem);

// Ulazak u prekidnu rutinu sa datim uzrokom (cause). Cuva status i pc na stek,
// postavlja cause, globalno maskira prekide i skace na adresu iz handler-a.
// Ovu funkciju dele svi izvori prekida (softverski, terminal, tajmer, greska).
void udjiUPrekid(Procesor& cpu, Memorija& mem, u32 uzrok);

#endif // EMULACIJA_HPP