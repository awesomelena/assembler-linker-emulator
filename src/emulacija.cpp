// ============================================================================
// emulacija.cpp
//
// Implementacija fetch-decode-execute ciklusa i izvrsavanja svih instrukcija
// za nivo A (bez prekida i periferija).
// ============================================================================

#include "emulacija.hpp"
#include "instrukcije.hpp"
#include "terminal.hpp"
#include "tajmer.hpp"
#include <string>

// ----------------------------------------------------------------------------
// Upis reci u memoriju uz presretanje memorijski mapiranih registara periferija.
// Ako adresa odgovara registru periferije, upis se prosledjuje toj periferiji;
// u suprotnom ide u obicnu memoriju.
// ----------------------------------------------------------------------------
static void upisiUMemoriju(Memorija& mem, u32 adresa, u32 vrednost) {
    if (adresa == ADR_TERM_OUT) {
        // Upis u term_out ispisuje znak na displej.
        terminalIspisi(vrednost);
        return;
    }
    if (adresa == ADR_TIM_CFG) {
        // Upis u tim_cfg menja periodu tajmera. Vrednost cuvamo i u memoriji
        // kako bi je program mogao procitati nazad.
        tajmerPostaviPeriodu(vrednost);
        mem.pisiRec(adresa, vrednost);
        return;
    }
    mem.pisiRec(adresa, vrednost);
}

// Izvrsava jednu dekodiranu instrukciju. Menja stanje procesora i memorije.
// Baca EmulacionaGreska za nekorektnu instrukciju/nacin adresiranja.
static void izvrsi(const DekodiranaInstrukcija& d, Procesor& cpu, Memorija& mem) {
    // Skracenice za citljivost.
    auto A = d.regA;
    auto B = d.regB;
    auto C = d.regC;
    auto D = d.disp;

    switch (d.oc) {

    case OC_HALT:
        cpu.zaustavljen = true;
        return;

    case OC_INT:
        // Softverski prekid: udji u prekidnu rutinu sa uzrokom "softver".
        udjiUPrekid(cpu, mem, CAUSE_SOFTVER);
        return;

    case OC_XCHG: {
        // temp<=gpr[B]; gpr[B]<=gpr[C]; gpr[C]<=temp;
        u32 temp = cpu.citajGpr(B);
        cpu.pisiGpr(B, cpu.citajGpr(C));
        cpu.pisiGpr(C, temp);
        return;
    }

    case OC_ARIT: {
        u32 b = cpu.citajGpr(B);
        u32 c = cpu.citajGpr(C);
        u32 rez = 0;
        switch (d.mod) {
            case 0b0000: rez = b + c; break;
            case 0b0001: rez = b - c; break;
            case 0b0010: rez = b * c; break;
            case 0b0011:
                if (c == 0) throw EmulacionaGreska{"deljenje nulom"};
                rez = static_cast<u32>(static_cast<i32>(b) / static_cast<i32>(c));
                break;
            default: throw EmulacionaGreska{"nepoznat modifikator aritmetike"};
        }
        cpu.pisiGpr(A, rez);
        return;
    }

    case OC_LOGIKA: {
        u32 b = cpu.citajGpr(B);
        u32 c = cpu.citajGpr(C);
        u32 rez = 0;
        switch (d.mod) {
            case 0b0000: rez = ~b; break;         // not
            case 0b0001: rez = b & c; break;      // and
            case 0b0010: rez = b | c; break;      // or
            case 0b0011: rez = b ^ c; break;      // xor
            default: throw EmulacionaGreska{"nepoznat modifikator logike"};
        }
        cpu.pisiGpr(A, rez);
        return;
    }

    case OC_POMER: {
        u32 b = cpu.citajGpr(B);
        u32 c = cpu.citajGpr(C);
        u32 rez = 0;
        switch (d.mod) {
            case 0b0000: rez = b << c; break;  // shl
            case 0b0001: rez = b >> c; break;  // shr (logicko za u32)
            default: throw EmulacionaGreska{"nepoznat modifikator pomeranja"};
        }
        cpu.pisiGpr(A, rez);
        return;
    }

    case OC_CALL: {
        // MMMM==0000: push pc; pc<=gpr[A]+gpr[B]+D
        // MMMM==0001: push pc; pc<=mem32[gpr[A]+gpr[B]+D]
        u32 adresa = cpu.citajGpr(A) + cpu.citajGpr(B) + static_cast<u32>(D);
        // push pc: sp-=4; mem[sp]=pc
        cpu.postaviSp(cpu.sp() - 4);
        mem.pisiRec(cpu.sp(), cpu.pc());
        if (d.mod == 0b0000) {
            cpu.postaviPc(adresa);
        } else if (d.mod == 0b0001) {
            cpu.postaviPc(mem.citajRec(adresa));
        } else {
            throw EmulacionaGreska{"nepoznat modifikator call"};
        }
        return;
    }

    case OC_JMP: {
        // Racunanje adrese zavisi od toga da li je direktni ili mem-oblik.
        // MMMM 0xxx: pc<=gpr[A]+D (ili uslovno)
        // MMMM 1xxx: pc<=mem32[gpr[A]+D] (ili uslovno)
        bool memOblik = (d.mod & 0b1000) != 0;
        u32 baza = cpu.citajGpr(A) + static_cast<u32>(D);
        u32 odrediste = memOblik ? mem.citajRec(baza) : baza;

        u8 tip = d.mod & 0b0111;  // donja tri bita biraju uslov
        bool skoci = false;
        switch (tip) {
            case 0b000: skoci = true; break;  // bezuslovni jmp
            case 0b001: skoci = (cpu.citajGpr(B) == cpu.citajGpr(C)); break;  // beq
            case 0b010: skoci = (cpu.citajGpr(B) != cpu.citajGpr(C)); break;  // bne
            case 0b011: // bgt (oznaceno poredjenje)
                skoci = (static_cast<i32>(cpu.citajGpr(B)) >
                         static_cast<i32>(cpu.citajGpr(C)));
                break;
            default: throw EmulacionaGreska{"nepoznat modifikator skoka"};
        }
        if (skoci) {
            cpu.postaviPc(odrediste);
        }
        return;
    }

    case OC_ST: {
        // MMMM==0000: mem32[gpr[A]+gpr[B]+D] <= gpr[C]
        // MMMM==0010: mem32[mem32[gpr[A]+gpr[B]+D]] <= gpr[C]
        // MMMM==0001: gpr[A]<=gpr[A]+D; mem32[gpr[A]] <= gpr[C]  (push)
        u32 adresa = cpu.citajGpr(A) + cpu.citajGpr(B) + static_cast<u32>(D);
        switch (d.mod) {
            case 0b0000:
                upisiUMemoriju(mem, adresa, cpu.citajGpr(C));
                return;
            case 0b0010:
                upisiUMemoriju(mem, mem.citajRec(adresa), cpu.citajGpr(C));
                return;
            case 0b0001: {
                // pre-dekrement (push): gpr[A]+=D pa upisi na tu adresu
                u32 novaA = cpu.citajGpr(A) + static_cast<u32>(D);
                cpu.pisiGpr(A, novaA);
                upisiUMemoriju(mem, novaA, cpu.citajGpr(C));
                return;
            }
            default: throw EmulacionaGreska{"nepoznat modifikator st"};
        }
    }

    case OC_LD: {
        // Vise modova - vidi prilog.
        switch (d.mod) {
            case 0b0000:  // gpr[A]<=csr[B]
                cpu.pisiGpr(A, cpu.citajCsr(B));
                return;
            case 0b0001:  // gpr[A]<=gpr[B]+D
                cpu.pisiGpr(A, cpu.citajGpr(B) + static_cast<u32>(D));
                return;
            case 0b0010:  // gpr[A]<=mem32[gpr[B]+gpr[C]+D]
                cpu.pisiGpr(A, mem.citajRec(cpu.citajGpr(B) + cpu.citajGpr(C) + static_cast<u32>(D)));
                return;
            case 0b0011: {  // gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D  (pop)
                u32 vrednost = mem.citajRec(cpu.citajGpr(B));
                cpu.pisiGpr(A, vrednost);
                cpu.pisiGpr(B, cpu.citajGpr(B) + static_cast<u32>(D));
                return;
            }
            case 0b0100:  // csr[A]<=gpr[B]
                cpu.pisiCsr(A, cpu.citajGpr(B));
                return;
            case 0b0101:  // csr[A]<=csr[B]|D
                cpu.pisiCsr(A, cpu.citajCsr(B) | static_cast<u32>(D));
                return;
            case 0b0110:  // csr[A]<=mem32[gpr[B]+gpr[C]+D]
                cpu.pisiCsr(A, mem.citajRec(cpu.citajGpr(B) + cpu.citajGpr(C) + static_cast<u32>(D)));
                return;
            case 0b0111: {  // csr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D
                u32 vrednost = mem.citajRec(cpu.citajGpr(B));
                cpu.pisiCsr(A, vrednost);
                cpu.pisiGpr(B, cpu.citajGpr(B) + static_cast<u32>(D));
                return;
            }
            default: throw EmulacionaGreska{"nepoznat modifikator ld"};
        }
    }

    default:
        throw EmulacionaGreska{"nepoznat operacioni kod " + std::to_string(d.oc)};
    }
}

// ----------------------------------------------------------------------------
// Ulazak u prekidnu rutinu. Redosled je propisan prilogom:
//   push status; push pc; cause<=uzrok; status |= I (maskiraj); pc<=handler
// Guranje status PA pc znaci da je pc na vrhu steka, sto odgovara redosledu
// u iret naredbi (pop pc; pop status).
// ----------------------------------------------------------------------------
void udjiUPrekid(Procesor& cpu, Memorija& mem, u32 uzrok) {
    // push status: sp-=4; mem[sp]=status
    cpu.postaviSp(cpu.sp() - 4);
    mem.pisiRec(cpu.sp(), cpu.citajCsr(CSR_STATUS));

    // push pc: sp-=4; mem[sp]=pc
    cpu.postaviSp(cpu.sp() - 4);
    mem.pisiRec(cpu.sp(), cpu.pc());

    // Postavi uzrok prekida.
    cpu.pisiCsr(CSR_CAUSE, uzrok);

    // Globalno maskiraj prekide (postavi I bit) da rutina ne bude prekinuta.
    cpu.pisiCsr(CSR_STATUS, cpu.citajCsr(CSR_STATUS) | STATUS_I);

    // Skoci na prekidnu rutinu.
    cpu.postaviPc(cpu.citajCsr(CSR_HANDLER));
}

void emuliraj(Procesor& cpu, Memorija& mem) {
    // Reset: pc na pocetnu adresu.
    cpu.postaviPc(POCETNA_ADRESA);

    // Priprema terminala za rad (sirovi rezim). Vracanje u prvobitno stanje
    // radi se na kraju, ukljucujuci i slucaj greske.
    terminalPripremi();
    tajmerPripremi();

    // Da li cekaju neopsluzeni zahtevi za prekid od periferija.
    bool zahtevTerminala = false;
    bool zahtevTajmera = false;

    try {
        while (!cpu.zaustavljen) {
            // --- Fetch ---------------------------------------------------
            u32 rec = mem.citajRec(cpu.pc());

            // Pomeri pc na sledecu instrukciju PRE izvrsavanja (jer instrukcije
            // koje koriste pc ocekuju da vec pokazuje na sledecu).
            cpu.postaviPc(cpu.pc() + 4);

            // --- Decode --------------------------------------------------
            DekodiranaInstrukcija d = dekodirajInstrukciju(rec);

            // --- Execute -------------------------------------------------
            izvrsi(d, cpu, mem);

            // --- Provera periferija i prekida (na granici instrukcije) ----
            // Instrukcije se izvrsavaju atomicno: zahtev za prekid se opsluzuje
            // tek nakon sto se tekuca instrukcija zavrsi do kraja.

            // Da li je pritisnut taster? Ako jeste, upisi ASCII kod u term_in
            // i zabelezi zahtev za prekid od terminala.
            u8 kodTastera;
            if (terminalProveriTaster(kodTastera)) {
                mem.pisiRec(ADR_TERM_IN, static_cast<u32>(kodTastera));
                zahtevTerminala = true;
            }

            // Prekid od terminala se prihvata samo ako nije maskiran:
            // globalno (I bit) i pojedinacno za terminal (Tl bit).
            if (zahtevTerminala) {
                u32 status = cpu.citajCsr(CSR_STATUS);
                bool globalnoMaskiran = (status & STATUS_I) != 0;
                bool terminalMaskiran = (status & STATUS_TL) != 0;
                if (!globalnoMaskiran && !terminalMaskiran) {
                    zahtevTerminala = false;
                    udjiUPrekid(cpu, mem, CAUSE_TERMINAL);
                    continue;  // prekid je preuzeo tok; nastavi od handlera
                }
            }

            // Da li je proteklo vreme jednako periodi tajmera?
            if (tajmerProveriOtkucaj()) {
                zahtevTajmera = true;
            }

            // Prekid od tajmera se prihvata samo ako nije maskiran:
            // globalno (I bit) i pojedinacno za tajmer (Tr bit).
            if (zahtevTajmera) {
                u32 status = cpu.citajCsr(CSR_STATUS);
                bool globalnoMaskiran = (status & STATUS_I) != 0;
                bool tajmerMaskiran = (status & STATUS_TR) != 0;
                if (!globalnoMaskiran && !tajmerMaskiran) {
                    zahtevTajmera = false;
                    udjiUPrekid(cpu, mem, CAUSE_TAJMER);
                }
            }
        }
    } catch (...) {
        // U slucaju bilo kakve greske obavezno vrati terminal u prvobitno
        // stanje pre nego sto greska ode dalje, da korisnikov terminal ne
        // ostane u sirovom rezimu.
        terminalVrati();
        throw;
    }

    terminalVrati();
}