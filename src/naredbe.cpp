// ============================================================================
// naredbe.cpp
//
// Generisanje masinskog koda za asemblerske naredbe (instrukcije).
//
// U ovoj fazi podrzane su:
//   - naredbe bez operanada: halt, int
//   - naredbe sa registrima: xchg, add, sub, mul, div, not, and, or, xor,
//     shl, shr, csrrd, csrwr
//
// Naredbe sa operandima (ld, st, skokovi, push, pop, call, ret, iret, jmp)
// dodaju se u narednoj fazi zajedno sa mehanizmom relokacija, jer nose
// pomeraj ili referencu na simbol.
// ============================================================================

#include "asembler.hpp"
#include "instrukcije.hpp"
#include "registri.hpp"

// Pomocna funkcija: iz tokena na poziciji i procita registar (mora biti
// vrste REGISTAR i ispravnog imena) i vrati njegov indeks. U suprotnom baca
// gresku. Po povratku, i pokazuje na sledeci token.
static u8 procitajGpr(const std::vector<Token>& t, size_t& i, const std::string& naredba) {
    if (i >= t.size() || t[i].vrsta != VrstaTokena::REGISTAR) {
        throw AsemblerskaGreska{naredba + ": ocekivan registar"};
    }
    int idx = indeksGpr(t[i].tekst);
    if (idx < 0) {
        throw AsemblerskaGreska{naredba + ": nepoznat registar '%" + t[i].tekst + "'"};
    }
    i++;
    return static_cast<u8>(idx);
}

// Pomocna funkcija: procita kontrolni registar (status/handler/cause).
static u8 procitajCsr(const std::vector<Token>& t, size_t& i, const std::string& naredba) {
    if (i >= t.size() || t[i].vrsta != VrstaTokena::REGISTAR) {
        throw AsemblerskaGreska{naredba + ": ocekivan kontrolni registar"};
    }
    int idx = indeksCsr(t[i].tekst);
    if (idx < 0) {
        throw AsemblerskaGreska{naredba + ": nepoznat kontrolni registar '%" + t[i].tekst + "'"};
    }
    i++;
    return static_cast<u8>(idx);
}

// Pomocna funkcija: proverava da je na poziciji i zapeta i preskace je.
static void ocekujZapetu(const std::vector<Token>& t, size_t& i, const std::string& naredba) {
    if (i >= t.size() || t[i].vrsta != VrstaTokena::ZAPETA) {
        throw AsemblerskaGreska{naredba + ": ocekivana zapeta izmedju operanada"};
    }
    i++;
}

// Pomocna funkcija: proverava da je linija zavrsena (nema viska tokena).
static void ocekujKraj(const std::vector<Token>& t, size_t i, const std::string& naredba) {
    if (i < t.size() && t[i].vrsta != VrstaTokena::KRAJ) {
        throw AsemblerskaGreska{naredba + ": visak tokena na kraju naredbe"};
    }
}

void Asembler::obradiNaredbu(const std::vector<Token>& tokeni, size_t pocetak) {
    Sekcija* sek = aktivnaSekcija();
    const std::string& ime = tokeni[pocetak].tekst;
    size_t i = pocetak + 1;  // pozicija prvog operanda (ako ga ima)

    // --- Naredbe bez operanada ---------------------------------------------

    if (ime == "halt") {
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_HALT, 0, 0, 0, 0, 0));
        return;
    }
    if (ime == "int") {
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_INT, 0, 0, 0, 0, 0));
        return;
    }

    // --- Atomicna zamena: xchg %gprS, %gprD --------------------------------
    // Masinski: temp<=gpr[B]; gpr[B]<=gpr[C]; gpr[C]<=temp;  (B i C su operandi)
    if (ime == "xchg") {
        u8 s = procitajGpr(tokeni, i, ime);
        ocekujZapetu(tokeni, i, ime);
        u8 d = procitajGpr(tokeni, i, ime);
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_XCHG, 0, 0, s, d, 0));
        return;
    }

    // --- Aritmetika: add/sub/mul/div %gprS, %gprD --------------------------
    // Asemblerski: gprD <= gprD op gprS
    // Masinski:    gpr[A] <= gpr[B] op gpr[C]  =>  A=D, B=D, C=S
    if (ime == "add" || ime == "sub" || ime == "mul" || ime == "div") {
        u8 mod;
        if (ime == "add") mod = 0b0000;
        else if (ime == "sub") mod = 0b0001;
        else if (ime == "mul") mod = 0b0010;
        else mod = 0b0011; // div
        u8 s = procitajGpr(tokeni, i, ime);
        ocekujZapetu(tokeni, i, ime);
        u8 d = procitajGpr(tokeni, i, ime);
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_ARIT, mod, d, d, s, 0));
        return;
    }

    // --- Logika: not %gpr  te  and/or/xor %gprS, %gprD ---------------------
    if (ime == "not") {
        // Asemblerski: gpr <= ~gpr.  Masinski: gpr[A] <= ~gpr[B]  => A=B=gpr
        u8 g = procitajGpr(tokeni, i, ime);
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_LOGIKA, 0b0000, g, g, 0, 0));
        return;
    }
    if (ime == "and" || ime == "or" || ime == "xor") {
        u8 mod;
        if (ime == "and") mod = 0b0001;
        else if (ime == "or") mod = 0b0010;
        else mod = 0b0011; // xor
        u8 s = procitajGpr(tokeni, i, ime);
        ocekujZapetu(tokeni, i, ime);
        u8 d = procitajGpr(tokeni, i, ime);
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_LOGIKA, mod, d, d, s, 0));
        return;
    }

    // --- Pomeranje: shl/shr %gprS, %gprD -----------------------------------
    if (ime == "shl" || ime == "shr") {
        u8 mod = (ime == "shl") ? 0b0000 : 0b0001;
        u8 s = procitajGpr(tokeni, i, ime);
        ocekujZapetu(tokeni, i, ime);
        u8 d = procitajGpr(tokeni, i, ime);
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_POMER, mod, d, d, s, 0));
        return;
    }

    // --- Rad sa kontrolnim registrima --------------------------------------
    // csrrd %csr, %gpr  ->  gpr <= csr.  Masinski ld MOD=0000: gpr[A]<=csr[B]
    if (ime == "csrrd") {
        u8 csr = procitajCsr(tokeni, i, ime);
        ocekujZapetu(tokeni, i, ime);
        u8 g = procitajGpr(tokeni, i, ime);
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0000, g, csr, 0, 0));
        return;
    }
    // csrwr %gpr, %csr  ->  csr <= gpr.  Masinski ld MOD=0100: csr[A]<=gpr[B]
    if (ime == "csrwr") {
        u8 g = procitajGpr(tokeni, i, ime);
        ocekujZapetu(tokeni, i, ime);
        u8 csr = procitajCsr(tokeni, i, ime);
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0100, csr, g, 0, 0));
        return;
    }

    // --- Stek naredbe: push %gpr, pop %gpr ---------------------------------
    // push %g -> st sa pre-dekrementom sp: sp-=4; mem[sp]=g
    //   (st MOD=0001: gpr[A]<=gpr[A]+D; mem32[gpr[A]]<=gpr[C];  A=sp C=g D=-4)
    if (ime == "push") {
        u8 g = procitajGpr(tokeni, i, ime);
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_ST, 0b0001, REG_SP, 0, g, -4));
        return;
    }
    // pop %g -> ld sa post-inkrementom sp: g=mem[sp]; sp+=4
    //   (ld MOD=0011: gpr[A]<=mem32[gpr[B]]; gpr[B]<=gpr[B]+D;  A=g B=sp D=4)
    if (ime == "pop") {
        u8 g = procitajGpr(tokeni, i, ime);
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0011, g, REG_SP, 0, 4));
        return;
    }

    // --- ret / iret --------------------------------------------------------
    // ret -> pop pc:  pc=mem[sp]; sp+=4
    if (ime == "ret") {
        ocekujKraj(tokeni, i, ime);
        sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0011, REG_PC, REG_SP, 0, 4));
        return;
    }
    // iret -> vrati status i pc sa steka.
    // Na steku je (od vrha): pc, pa status (jer se pri ulasku u prekid gura
    // prvo status pa pc). Redosled instrukcija je bitan: ako bismo prvo vratili
    // pc, procesor bi odmah skocio i druga instrukcija se ne bi izvrsila.
    // Zato prvo citamo status sa sp+4 (bez pomeranja sp), pa zatim pc sa sp
    // uz pomeranje sp za 8 (obe reci se skidaju).
    if (ime == "iret") {
        ocekujKraj(tokeni, i, ime);
        // csr[status] <= mem32[sp + 4]   (ld MOD=0110: csr[A]<=mem32[gpr[B]+gpr[C]+D])
        sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0110, CSR_STATUS, REG_SP, 0, 4));
        // pc <= mem32[sp]; sp += 8       (ld MOD=0011: gpr[A]<=mem32[gpr[B]]; gpr[B]+=D)
        sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0011, REG_PC, REG_SP, 0, 8));
        return;
    }

    // Naredbe sa operandima (ld, st, skokovi, push, pop, call, ret, iret, jmp)
    // dolaze u narednoj fazi.

    // ld i st - naredbe za rad sa podacima sa svih osam nacina adresiranja.
    if (ime == "ld") {
        obradiLd(tokeni, i);
        ocekujKraj(tokeni, i, ime);
        return;
    }
    if (ime == "st") {
        obradiSt(tokeni, i);
        ocekujKraj(tokeni, i, ime);
        return;
    }

    // Skok i poziv: jmp, call, beq, bne, bgt.
    if (ime == "jmp" || ime == "call" || ime == "beq" || ime == "bne" || ime == "bgt") {
        obradiSkok(tokeni, i);
        ocekujKraj(tokeni, i, ime);
        return;
    }

    throw AsemblerskaGreska{"naredba '" + ime + "' jos nije implementirana u ovoj fazi"};
}
