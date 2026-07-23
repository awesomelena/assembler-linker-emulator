// ============================================================================
// procesor.hpp
//
// Stanje emuliranog procesora: opstenamenski registri, kontrolni/statusni
// registri i pravila pristupa (r0 je uvek nula, r15=pc, r14=sp).
// ============================================================================

#ifndef PROCESOR_HPP
#define PROCESOR_HPP

#include "tipovi.hpp"
#include "instrukcije.hpp"  // REG_PC, REG_SP, CSR_* konstante

// Indeksi kontrolnih registara vec su definisani u instrukcije.hpp
// (CSR_STATUS=0, CSR_HANDLER=1, CSR_CAUSE=2).

struct Procesor {
    // 16 opstenamenskih registara. gpr[15]=pc, gpr[14]=sp, gpr[0] uvek 0.
    u32 gpr[16] = {0};

    // Kontrolni/statusni registri: status, handler, cause (indeksi 0,1,2).
    u32 csr[3] = {0};

    bool zaustavljen = false;  // postaje true kada se izvrsi halt

    // Citanje opstenamenskog registra. r0 se uvek cita kao 0.
    u32 citajGpr(u8 indeks) const {
        if (indeks == 0) return 0;
        return gpr[indeks & 0xF];
    }

    // Upis u opstenamenski registar. Upis u r0 se ignorise (ozicen na nulu).
    void pisiGpr(u8 indeks, u32 vrednost) {
        if (indeks == 0) return;
        gpr[indeks & 0xF] = vrednost;
    }

    // Citanje/upis kontrolnog registra (indeks 0..2).
    u32 citajCsr(u8 indeks) const { return csr[indeks % 3]; }
    void pisiCsr(u8 indeks, u32 vrednost) { csr[indeks % 3] = vrednost; }

    // Pristup pc i sp radi citljivosti.
    u32 pc() const { return gpr[REG_PC]; }
    void postaviPc(u32 v) { gpr[REG_PC] = v; }
    u32 sp() const { return gpr[REG_SP]; }
    void postaviSp(u32 v) { gpr[REG_SP] = v; }
};

#endif // PROCESOR_HPP