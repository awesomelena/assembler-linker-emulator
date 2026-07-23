// ============================================================================
// registri.hpp
//
// Pomocne funkcije za prepoznavanje opstenamenskih i kontrolnih registara na
// osnovu njihovog imena (onako kako ih lekser vrati, bez znaka %).
// ============================================================================

#ifndef REGISTRI_HPP
#define REGISTRI_HPP

#include "tipovi.hpp"
#include <string>

// Vraca indeks opstenamenskog registra (0..15) za dato ime, ili -1 ako ime
// ne predstavlja ispravan opstenamenski registar.
// Prihvata oblike: r0..r15, kao i aliase sp (=r14) i pc (=r15).
int indeksGpr(const std::string& ime);

// Vraca indeks kontrolnog/statusnog registra (status=0, handler=1, cause=2),
// ili -1 ako ime ne predstavlja ispravan kontrolni registar.
int indeksCsr(const std::string& ime);

#endif // REGISTRI_HPP