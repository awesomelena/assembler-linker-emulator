// ============================================================================
// tipovi.hpp
//
// Osnovni tipovi podataka koje koristi ceo projekat (asembler, linker,
// emulator). Ovde drzimo kratke aliase za cele brojeve fiksne sirine kako
// bi kod bio precizan i citljiv.
// ============================================================================

#ifndef TIPOVI_HPP
#define TIPOVI_HPP

#include <cstdint>

// Aliasi za cele brojeve fiksne sirine.
// Koristimo ih umesto "int", "unsigned" i slicno jer na nivou masinskog koda
// tacna sirina (broj bitova) ima znacaj - instrukcije su 4 bajta, adrese
// 32 bita, pojedinacni bajtovi 8 bita itd.
using u8  = uint8_t;    // jedan bajt bez znaka (0..255)
using u16 = uint16_t;   // dva bajta bez znaka
using u32 = uint32_t;   // cetiri bajta bez znaka - sirina reci i adrese
using i32 = int32_t;    // cetiri bajta sa znakom - kada nam treba oznaceno

#endif // TIPOVI_HPP