// ============================================================================
// emulator.cpp
//
// Ulazna tacka (main) emulatora. Ucitava -hex zapis u memoriju, pokrece
// emulaciju do halt instrukcije i ispisuje zavrsno stanje procesora.
//
// Nacin pokretanja:
//   ./emulator <ulazna_datoteka>
// ============================================================================

#include "emulacija.hpp"
#include "ucitavanje.hpp"
#include <iostream>
#include <cstdio>
#include <string>

// Ispisuje zavrsno stanje procesora u formatu iz postavke.
static void ispisiStanje(const Procesor& cpu) {
    printf("-----------------------------------------------------------------\n");
    printf("Emulated processor executed halt instruction\n");
    printf("Emulated processor state:\n");
    for (int i = 0; i < 16; i++) {
        printf("r%d=0x%08X", i, cpu.gpr[i]);
        // Cetiri registra po redu, razdvojena razmakom; novi red na svaka 4.
        if ((i + 1) % 4 == 0) {
            printf("\n");
        } else {
            printf(" ");
        }
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Greska: emulator ocekuje tacno jedan argument.\n";
        std::cerr << "Upotreba: emulator <ulaz.hex>\n";
        return 1;
    }

    std::string ulaznaDatoteka = argv[1];

    // Ucitaj hex zapis u memoriju.
    Memorija mem;
    std::string greska;
    if (!ucitajHexUMemoriju(ulaznaDatoteka, mem, greska)) {
        std::cerr << "Greska pri ucitavanju: " << greska << "\n";
        return 1;
    }

    // Pokreni emulaciju.
    Procesor cpu;
    try {
        emuliraj(cpu, mem);
    } catch (const EmulacionaGreska& g) {
        std::cerr << "Greska pri emulaciji: " << g.poruka << "\n";
        return 1;
    }

    // Ispisi zavrsno stanje.
    ispisiStanje(cpu);
    return 0;
}