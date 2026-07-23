// ============================================================================
// tajmer.hpp
//
// Tajmer periferija: periodicno generise zahtev za prekid.
//
// Programski dostupan registar mapiran u memorijski adresni prostor:
//   tim_cfg [0xFFFFFF10-0xFFFFFF13] - konfiguracija periode
//
// Vrednost tim_cfg odredjuje periodu prema prilogu:
//   0x0 -> 500ms, 0x1 -> 1000ms, 0x2 -> 1500ms, 0x3 -> 2000ms,
//   0x4 -> 5000ms, 0x5 -> 10s, 0x6 -> 30s, 0x7 -> 60s
// Pocetna vrednost registra nakon reseta je 0x0.
// ============================================================================

#ifndef TAJMER_HPP
#define TAJMER_HPP

#include "tipovi.hpp"

// Adresa memorijski mapiranog registra tajmera (iz priloga).
constexpr u32 ADR_TIM_CFG = 0xFFFFFF10;

// Priprema tajmer: postavlja pocetnu periodu i pocetni trenutak merenja.
// Poziva se jednom, na pocetku emulacije.
void tajmerPripremi();

// Postavlja periodu na osnovu vrednosti upisane u tim_cfg registar.
// Nepoznate vrednosti se zanemaruju (perioda ostaje nepromenjena).
void tajmerPostaviPeriodu(u32 vrednost);

// Proverava da li je od poslednjeg otkucaja proteklo vreme jednako periodi.
// Ako jeste, vraca true i belezi novi trenutak otkucaja; inace vraca false.
bool tajmerProveriOtkucaj();

#endif // TAJMER_HPP