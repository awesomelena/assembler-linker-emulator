// ============================================================================
// ucitavanje.hpp
//
// Ucitavanje -hex zapisa (izlaz linkera) u memoriju emulatora.
// Format reda: "ADRESA: BB BB BB ..." gde su ADRESA i BB heksadecimalni.
// ============================================================================

#ifndef UCITAVANJE_HPP
#define UCITAVANJE_HPP

#include "memorija.hpp"
#include <string>

// Ucitava -hex datoteku u datu memoriju.
// Vraca true ako je uspesno; u suprotnom false i postavlja poruku o gresci.
bool ucitajHexUMemoriju(const std::string& putanja,
                        Memorija& memorija,
                        std::string& greska);

#endif // UCITAVANJE_HPP