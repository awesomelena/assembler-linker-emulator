// ============================================================================
// izlaz.hpp
//
// Ispis predmetnog programa (rezultata asembliranja) u tekstualnu datoteku.
// Format je citljiv coveku i lako parsirljiv za linker: svaki blok pocinje
// markerom (npr. #tabela_simbola), a polja su razdvojena razmacima.
// ============================================================================

#ifndef IZLAZ_HPP
#define IZLAZ_HPP

#include "tabela_simbola.hpp"
#include "sekcija.hpp"
#include <string>
#include <vector>

// Ispisuje predmetni program u datoteku na datoj putanji.
// Vraca true ako je uspesno, false ako datoteka ne moze da se otvori za pisanje.
bool ispisiPredmetniProgram(const std::string& putanja,
                            const TabelaSimbola& tabela,
                            const std::vector<Sekcija>& sekcije);

// Overload koji prima vektor simbola direktno (koristi linker za -relocatable
// izlaz, gde nema TabelaSimbola objekta nego gotov vektor simbola).
bool ispisiPredmetniProgram(const std::string& putanja,
                            const std::vector<Simbol>& simboli,
                            const std::vector<Sekcija>& sekcije);

#endif // IZLAZ_HPP