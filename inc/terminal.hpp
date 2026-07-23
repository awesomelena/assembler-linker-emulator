// ============================================================================
// terminal.hpp
//
// Terminal periferija: displej (izlaz) i tastatura (ulaz).
//
// Programski dostupni registri mapirani su u memorijski adresni prostor:
//   term_out [0xFFFFFF00-0xFFFFFF03] - upis ispisuje znak na displej
//   term_in  [0xFFFFFF04-0xFFFFFF07] - sadrzi ASCII kod pritisnutog tastera
//
// Ovaj modul pokriva izlaznu stranu (term_out) i pripremu za ulaznu.
// ============================================================================

#ifndef TERMINAL_HPP
#define TERMINAL_HPP

#include "tipovi.hpp"

// Adrese memorijski mapiranih registara terminala (iz priloga).
constexpr u32 ADR_TERM_OUT = 0xFFFFFF00;
constexpr u32 ADR_TERM_IN  = 0xFFFFFF04;

// Ispisuje znak na displej na osnovu vrednosti upisane u term_out.
// Znak je odredjen ASCII tabelom za donji bajt upisane vrednosti.
void terminalIspisi(u32 vrednost);

// ----------------------------------------------------------------------------
// Ulazna strana terminala (tastatura).
//
// Terminal se prebacuje u "sirovi" rezim kako bi svaki pritisak tastera stigao
// odmah, bez cekanja Enter-a i bez eho prikaza. Originalna podesavanja se
// obavezno vracaju pri zavrsetku emulacije.
// ----------------------------------------------------------------------------

// Priprema terminal za rad: iskljucuje baferovanje po liniji i eho.
// Poziva se jednom, na pocetku emulacije.
void terminalPripremi();

// Vraca terminal u prvobitno stanje. Poziva se pri zavrsetku emulacije
// (i u slucaju greske) da korisnikov terminal ne ostane u sirovom rezimu.
void terminalVrati();

// Neblokirajuce proverava da li je pritisnut taster.
// Ako jeste, vraca true i upisuje ASCII kod u izlazni parametar.
// Ako nije, odmah vraca false (ne ceka).
bool terminalProveriTaster(u8& kod);

#endif // TERMINAL_HPP