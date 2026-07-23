// ============================================================================
// registri.cpp
//
// Implementacija prepoznavanja registara po imenu.
// ============================================================================

#include "registri.hpp"
#include "instrukcije.hpp"  // zbog REG_SP, REG_PC, CSR_* konstanti

int indeksGpr(const std::string& ime) {
    // Aliasi za registre sa posebnom ulogom.
    if (ime == "sp") return REG_SP;  // r14
    if (ime == "pc") return REG_PC;  // r15

    // Oblik r<broj>, gde je broj u opsegu 0..15.
    if (ime.size() >= 2 && ime[0] == 'r') {
        // Ostatak imena posle 'r' mora biti ceo broj.
        const std::string cifre = ime.substr(1);
        for (char c : cifre) {
            if (c < '0' || c > '9') return -1;  // nije cist broj
        }
        // Pretvaranje u broj uz zastitu od predugackog zapisa.
        int vrednost = 0;
        for (char c : cifre) {
            vrednost = vrednost * 10 + (c - '0');
            if (vrednost > 15) return -1;  // van opsega
        }
        return vrednost;
    }

    return -1;  // nepoznat oblik
}

int indeksCsr(const std::string& ime) {
    if (ime == "status")  return CSR_STATUS;
    if (ime == "handler") return CSR_HANDLER;
    if (ime == "cause")   return CSR_CAUSE;
    return -1;
}