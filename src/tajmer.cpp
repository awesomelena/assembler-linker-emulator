// ============================================================================
// tajmer.cpp
//
// Implementacija tajmer periferije. Perioda se meri u realnom vremenu, kako
// bi ponasanje odgovaralo opisu iz priloga (500ms, 1000ms itd.).
// ============================================================================

#include "tajmer.hpp"
#include <chrono>

// Tip za trenutak u vremenu; koristimo monotoni sat da merenje ne bude
// poremeceno eventualnom promenom sistemskog vremena.
using Sat = std::chrono::steady_clock;

// Trenutak poslednjeg otkucaja tajmera.
static Sat::time_point poslednjiOtkucaj;

// Trenutna perioda u milisekundama. Pocetna vrednost odgovara tim_cfg = 0x0.
static unsigned periodaMs = 500;

// Prevodi vrednost tim_cfg registra u periodu u milisekundama, prema prilogu.
// Vraca 0 ako vrednost nije prepoznata.
static unsigned periodaZaVrednost(u32 vrednost) {
    switch (vrednost) {
        case 0x0: return 500;
        case 0x1: return 1000;
        case 0x2: return 1500;
        case 0x3: return 2000;
        case 0x4: return 5000;
        case 0x5: return 10000;
        case 0x6: return 30000;
        case 0x7: return 60000;
        default:  return 0;  // nepoznata vrednost
    }
}

void tajmerPripremi() {
    periodaMs = 500;                  // pocetna vrednost tim_cfg je 0x0
    poslednjiOtkucaj = Sat::now();
}

void tajmerPostaviPeriodu(u32 vrednost) {
    unsigned nova = periodaZaVrednost(vrednost);
    if (nova == 0) {
        return;  // nepoznata vrednost - perioda ostaje nepromenjena
    }
    periodaMs = nova;
    // Novi ciklus merenja krece od trenutka promene konfiguracije.
    poslednjiOtkucaj = Sat::now();
}

bool tajmerProveriOtkucaj() {
    Sat::time_point sada = Sat::now();
    auto proteklo = std::chrono::duration_cast<std::chrono::milliseconds>(
                        sada - poslednjiOtkucaj).count();

    if (proteklo >= static_cast<long long>(periodaMs)) {
        // Novi ciklus merimo od sadasnjeg trenutka.
        poslednjiOtkucaj = sada;
        return true;
    }
    return false;
}