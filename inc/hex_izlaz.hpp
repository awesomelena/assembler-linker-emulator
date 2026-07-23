// ============================================================================
// hex_izlaz.hpp
//
// Generisanje -hex izlaza linkera: zapis za inicijalizaciju memorije u vidu
// parova (adresa, sadrzaj). Emulator ovaj zapis ucitava u svoj memorijski
// adresni prostor.
//
// Format (primer):
//   40000000: 12 00 1F 92 08 00 F0 38
//   40000008: 00 00 00 00 00 00 00 00
// ============================================================================

#ifndef HEX_IZLAZ_HPP
#define HEX_IZLAZ_HPP

#include "linker.hpp"
#include <string>
#include <vector>

// Ispisuje spojene sekcije u -hex format na datoj putanji.
// Vraca true ako je uspesno, false ako datoteka ne moze da se otvori.
bool ispisiHex(const std::string& putanja,
               const std::vector<SpojenaSekcija>& sekcije);

#endif // HEX_IZLAZ_HPP