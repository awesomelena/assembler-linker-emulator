// ============================================================================
// operandi.cpp
//
// Parsiranje operanada naredbi za rad sa podacima i generisanje koda za
// ld i st naredbe, koje podrzavaju svih osam nacina adresiranja.
//
// Za nacine adresiranja koji zahtevaju punu 32-bitnu vrednost (literal ili
// simbol koji ne staje u 12 bita), koristi se bazen literala: vrednost se
// smesta na kraj sekcije, a instrukcija joj pristupa PC-relativno.
// ============================================================================

#include "asembler.hpp"
#include "instrukcije.hpp"
#include "registri.hpp"

// ----------------------------------------------------------------------------
// Parsira jedan operand naredbe za rad sa podacima pocev od pozicije i.
// Po povratku, i pokazuje na prvi token iza operanda. Baca gresku ako oblik
// operanda nije ispravan.
// ----------------------------------------------------------------------------
static Operand parsirajOperand(const std::vector<Token>& t, size_t& i,
                               const std::string& naredba) {
    Operand op;

    if (i >= t.size() || t[i].vrsta == VrstaTokena::KRAJ) {
        throw AsemblerskaGreska{naredba + ": nedostaje operand"};
    }

    // --- $<literal> ili $<simbol> : neposredna vrednost --------------------
    if (t[i].vrsta == VrstaTokena::DOLAR) {
        i++;  // preskoci $
        if (i >= t.size()) {
            throw AsemblerskaGreska{naredba + ": nedostaje vrednost iza $"};
        }
        if (t[i].vrsta == VrstaTokena::BROJ) {
            op.vrsta = VrstaOperanda::IMED_LITERAL;
            op.literal = t[i].broj;
            i++;
            return op;
        }
        if (t[i].vrsta == VrstaTokena::IDENT) {
            op.vrsta = VrstaOperanda::IMED_SIMBOL;
            op.simbol = t[i].tekst;
            i++;
            return op;
        }
        throw AsemblerskaGreska{naredba + ": iza $ se ocekuje literal ili simbol"};
    }

    // --- %<reg> : vrednost u registru --------------------------------------
    if (t[i].vrsta == VrstaTokena::REGISTAR) {
        int idx = indeksGpr(t[i].tekst);
        if (idx < 0) {
            throw AsemblerskaGreska{naredba + ": nepoznat registar '%" + t[i].tekst + "'"};
        }
        op.vrsta = VrstaOperanda::REG;
        op.reg = static_cast<u8>(idx);
        i++;
        return op;
    }

    // --- [%<reg>], [%<reg> + <literal>], [%<reg> + <simbol>] ---------------
    if (t[i].vrsta == VrstaTokena::UGL_OTV) {
        i++;  // preskoci [
        if (i >= t.size() || t[i].vrsta != VrstaTokena::REGISTAR) {
            throw AsemblerskaGreska{naredba + ": ocekivan registar posle ["};
        }
        int idx = indeksGpr(t[i].tekst);
        if (idx < 0) {
            throw AsemblerskaGreska{naredba + ": nepoznat registar '%" + t[i].tekst + "'"};
        }
        op.reg = static_cast<u8>(idx);
        i++;

        // Zatvarena zagrada odmah -> [%reg]
        if (i < t.size() && t[i].vrsta == VrstaTokena::UGL_ZATV) {
            i++;
            op.vrsta = VrstaOperanda::MEM_REG;
            return op;
        }
        // Inace mora biti + pa literal ili simbol.
        if (i >= t.size() || t[i].vrsta != VrstaTokena::PLUS) {
            throw AsemblerskaGreska{naredba + ": ocekivano '+' ili ']' u [ ] operandu"};
        }
        i++;  // preskoci +
        if (i >= t.size()) {
            throw AsemblerskaGreska{naredba + ": nedostaje vrednost posle '+'"};
        }
        if (t[i].vrsta == VrstaTokena::BROJ) {
            op.vrsta = VrstaOperanda::MEM_REG_LITERAL;
            op.literal = t[i].broj;
            i++;
        } else if (t[i].vrsta == VrstaTokena::IDENT) {
            op.vrsta = VrstaOperanda::MEM_REG_SIMBOL;
            op.simbol = t[i].tekst;
            i++;
        } else {
            throw AsemblerskaGreska{naredba + ": posle '+' se ocekuje literal ili simbol"};
        }
        if (i >= t.size() || t[i].vrsta != VrstaTokena::UGL_ZATV) {
            throw AsemblerskaGreska{naredba + ": nedostaje ']'"};
        }
        i++;  // preskoci ]
        return op;
    }

    // --- <literal> ili <simbol> : vrednost iz memorije na toj adresi -------
    if (t[i].vrsta == VrstaTokena::BROJ) {
        op.vrsta = VrstaOperanda::MEM_LITERAL;
        op.literal = t[i].broj;
        i++;
        return op;
    }
    if (t[i].vrsta == VrstaTokena::IDENT) {
        op.vrsta = VrstaOperanda::MEM_SIMBOL;
        op.simbol = t[i].tekst;
        i++;
        return op;
    }

    throw AsemblerskaGreska{naredba + ": neprepoznat oblik operanda"};
}

// ----------------------------------------------------------------------------
// ld operand, %gpr   ->  gpr <= operand
//
// Realizacija po nacinu adresiranja operanda (koristi ld modove iz priloga):
//   MOD=0001: gpr[A] <= gpr[B] + D
//   MOD=0010: gpr[A] <= mem32[gpr[B] + gpr[C] + D]
// ----------------------------------------------------------------------------
void Asembler::obradiLd(const std::vector<Token>& tokeni, size_t& i) {
    const std::string naredba = "ld";

    Operand op = parsirajOperand(tokeni, i, naredba);

    // Iza operanda sledi zapeta pa odredisni registar.
    if (i >= tokeni.size() || tokeni[i].vrsta != VrstaTokena::ZAPETA) {
        throw AsemblerskaGreska{naredba + ": ocekivana zapeta pre odredisnog registra"};
    }
    i++;
    if (i >= tokeni.size() || tokeni[i].vrsta != VrstaTokena::REGISTAR) {
        throw AsemblerskaGreska{naredba + ": ocekivan odredisni registar"};
    }
    int dstIdx = indeksGpr(tokeni[i].tekst);
    if (dstIdx < 0) {
        throw AsemblerskaGreska{naredba + ": nepoznat odredisni registar"};
    }
    u8 dst = static_cast<u8>(dstIdx);
    i++;

    Sekcija* sek = aktivnaSekcija();

    switch (op.vrsta) {
        case VrstaOperanda::REG:
            // gpr <= reg  =>  gpr[dst] <= gpr[reg] + 0
            sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0001, dst, op.reg, 0, 0));
            return;

        case VrstaOperanda::MEM_REG:
            // gpr <= mem[reg]  =>  gpr[dst] <= mem32[gpr[reg] + 0 + 0]
            sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0010, dst, op.reg, 0, 0));
            return;

        case VrstaOperanda::MEM_REG_LITERAL:
            // gpr <= mem[reg + D]; D mora da stane u 12 bita.
            if (!staje12bita(op.literal)) {
                throw AsemblerskaGreska{naredba + ": pomeraj ne staje u 12 bita"};
            }
            sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0010, dst, op.reg, 0, op.literal));
            return;

        case VrstaOperanda::IMED_LITERAL: {
            // gpr <= literal (vrednost). Literal ide u bazen, pa PC-relativno
            // ucitavamo iz bazena: gpr[dst] <= mem32[pc + D].
            size_t idxStavke = dodajLiteralUBazen(static_cast<u32>(op.literal));
            u32 offInstr = sek->lokacija();
            sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0010, dst, REG_PC, 0, 0));
            sek->zakrpe.push_back(Zakrpa{offInstr, idxStavke});
            return;
        }

        case VrstaOperanda::IMED_SIMBOL: {
            // gpr <= vrednost(simbol). Adresa simbola ide u bazen + relokacija,
            // pa PC-relativno ucitavamo tu adresu.
            size_t idxStavke = dodajSimbolUBazen(op.simbol);
            u32 offInstr = sek->lokacija();
            sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0010, dst, REG_PC, 0, 0));
            sek->zakrpe.push_back(Zakrpa{offInstr, idxStavke});
            return;
        }

        case VrstaOperanda::MEM_LITERAL: {
            // gpr <= mem[literal]. Adresa (literal) ide u bazen; prvo ucitamo
            // adresu PC-relativno u dst, pa iz nje ucitamo vrednost.
            size_t idxStavke = dodajLiteralUBazen(static_cast<u32>(op.literal));
            u32 offInstr = sek->lokacija();
            // dst <= mem32[pc + D]  (adresa)
            sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0010, dst, REG_PC, 0, 0));
            sek->zakrpe.push_back(Zakrpa{offInstr, idxStavke});
            // dst <= mem32[dst]  (vrednost sa te adrese)
            sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0010, dst, dst, 0, 0));
            return;
        }

        case VrstaOperanda::MEM_SIMBOL: {
            // gpr <= mem[simbol]. Adresa simbola u bazen + relokacija; ucitaj
            // adresu PC-relativno pa vrednost sa nje.
            size_t idxStavke = dodajSimbolUBazen(op.simbol);
            u32 offInstr = sek->lokacija();
            sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0010, dst, REG_PC, 0, 0));
            sek->zakrpe.push_back(Zakrpa{offInstr, idxStavke});
            sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0010, dst, dst, 0, 0));
            return;
        }

        case VrstaOperanda::MEM_REG_SIMBOL:
            throw AsemblerskaGreska{naredba + ": [reg + simbol] jos nije podrzano u ovoj fazi"};
    }
}

// ----------------------------------------------------------------------------
// st %gpr, operand   ->  operand <= gpr
//
// Realizacija po nacinu adresiranja (koristi st modove iz priloga):
//   MOD=0000: mem32[gpr[A] + gpr[B] + D] <= gpr[C]
// Za smestanje na apsolutnu adresu (literal/simbol) koristimo bazen: adresa
// se ucita u pomocni registar, pa se koristi mem-oblik. Posto st nema slobodan
// registar za adresu bez ruzenja podataka, koristimo poseban obrazac nize.
// ----------------------------------------------------------------------------
void Asembler::obradiSt(const std::vector<Token>& tokeni, size_t& i) {
    const std::string naredba = "st";

    // Prvi operand je izvorni registar.
    if (i >= tokeni.size() || tokeni[i].vrsta != VrstaTokena::REGISTAR) {
        throw AsemblerskaGreska{naredba + ": ocekivan izvorni registar"};
    }
    int srcIdx = indeksGpr(tokeni[i].tekst);
    if (srcIdx < 0) {
        throw AsemblerskaGreska{naredba + ": nepoznat izvorni registar"};
    }
    u8 src = static_cast<u8>(srcIdx);
    i++;

    if (i >= tokeni.size() || tokeni[i].vrsta != VrstaTokena::ZAPETA) {
        throw AsemblerskaGreska{naredba + ": ocekivana zapeta posle izvornog registra"};
    }
    i++;

    Operand op = parsirajOperand(tokeni, i, naredba);
    Sekcija* sek = aktivnaSekcija();

    switch (op.vrsta) {
        case VrstaOperanda::MEM_REG:
            // mem[reg] <= src  =>  mem32[gpr[reg] + 0 + 0] <= gpr[src]
            sek->dodajRec(sklopiInstrukciju(OC_ST, 0b0000, op.reg, 0, src, 0));
            return;

        case VrstaOperanda::MEM_REG_LITERAL:
            if (!staje12bita(op.literal)) {
                throw AsemblerskaGreska{naredba + ": pomeraj ne staje u 12 bita"};
            }
            sek->dodajRec(sklopiInstrukciju(OC_ST, 0b0000, op.reg, 0, src, op.literal));
            return;

        case VrstaOperanda::MEM_LITERAL: {
            // mem[literal] <= src. Adresa (literal) ide u bazen; koristimo
            // st mod sa dvostrukom adresom: mem32[mem32[pc + D]] <= src
            // (MOD=0010 iz priloga: mem32[mem32[gpr[A]+gpr[B]+D]] <= gpr[C]).
            size_t idxStavke = dodajLiteralUBazen(static_cast<u32>(op.literal));
            u32 offInstr = sek->lokacija();
            sek->dodajRec(sklopiInstrukciju(OC_ST, 0b0010, REG_PC, 0, src, 0));
            sek->zakrpe.push_back(Zakrpa{offInstr, idxStavke});
            return;
        }

        case VrstaOperanda::MEM_SIMBOL: {
            // mem[simbol] <= src. Adresa simbola u bazen + relokacija; koristi
            // se st mod sa dvostrukom adresom (mem32[mem32[pc + D]] <= src).
            size_t idxStavke = dodajSimbolUBazen(op.simbol);
            u32 offInstr = sek->lokacija();
            sek->dodajRec(sklopiInstrukciju(OC_ST, 0b0010, REG_PC, 0, src, 0));
            sek->zakrpe.push_back(Zakrpa{offInstr, idxStavke});
            return;
        }

        case VrstaOperanda::REG:
            // st %gpr, %reg  ->  reg <= gpr. Ovo je zapravo prebacivanje medju
            // registrima; realizujemo ga kao ld-ekvivalent: gpr[reg] <= gpr[src].
            sek->dodajRec(sklopiInstrukciju(OC_LD, 0b0001, op.reg, src, 0, 0));
            return;

        case VrstaOperanda::IMED_LITERAL:
        case VrstaOperanda::IMED_SIMBOL:
            throw AsemblerskaGreska{naredba + ": neposredna vrednost ($) ne moze biti odrediste"};

        case VrstaOperanda::MEM_REG_SIMBOL:
            throw AsemblerskaGreska{naredba + ": [reg + simbol] jos nije podrzano u ovoj fazi"};
    }
}

// ----------------------------------------------------------------------------
// Skok i poziv: jmp, call, beq, bne, bgt.
//
// Odredisna adresa (literal ili simbol) je 32-bitna, pa ide u bazen literala,
// a instrukcija skace preko memorije: pc <= mem32[pc + D] (mem-oblik skoka).
//
// Sintaksa:
//   jmp  <operand>
//   call <operand>
//   beq/bne/bgt %gpr1, %gpr2, <operand>
// gde je <operand> literal ili simbol (odredisna adresa).
// ----------------------------------------------------------------------------
void Asembler::obradiSkok(const std::vector<Token>& tokeni, size_t& i) {
    // Ime naredbe je token neposredno pre trenutne pozicije (obradiNaredbu je
    // vec pomerio i iza imena naredbe).
    const std::string naredba = tokeni[i - 1].tekst;

    Sekcija* sek = aktivnaSekcija();

    // OC i osnovni MOD (mem-oblik) zavise od naredbe.
    u8 oc, mod;
    bool uslovni = false;
    if (naredba == "jmp") {
        oc = OC_JMP; mod = 0b1000;
    } else if (naredba == "call") {
        oc = OC_CALL; mod = 0b0001;  // call MOD=0001: push pc; pc<=mem32[...]
    } else if (naredba == "beq") {
        oc = OC_JMP; mod = 0b1001; uslovni = true;
    } else if (naredba == "bne") {
        oc = OC_JMP; mod = 0b1010; uslovni = true;
    } else { // bgt
        oc = OC_JMP; mod = 0b1011; uslovni = true;
    }

    // Za uslovne skokove prvo procitaj dva registra za poredjenje: %gpr1, %gpr2.
    u8 regB = 0, regC = 0;
    if (uslovni) {
        if (i >= tokeni.size() || tokeni[i].vrsta != VrstaTokena::REGISTAR) {
            throw AsemblerskaGreska{naredba + ": ocekivan prvi registar za poredjenje"};
        }
        int b = indeksGpr(tokeni[i].tekst);
        if (b < 0) throw AsemblerskaGreska{naredba + ": nepoznat registar"};
        regB = static_cast<u8>(b);
        i++;
        if (i >= tokeni.size() || tokeni[i].vrsta != VrstaTokena::ZAPETA) {
            throw AsemblerskaGreska{naredba + ": ocekivana zapeta"};
        }
        i++;
        if (i >= tokeni.size() || tokeni[i].vrsta != VrstaTokena::REGISTAR) {
            throw AsemblerskaGreska{naredba + ": ocekivan drugi registar za poredjenje"};
        }
        int c = indeksGpr(tokeni[i].tekst);
        if (c < 0) throw AsemblerskaGreska{naredba + ": nepoznat registar"};
        regC = static_cast<u8>(c);
        i++;
        if (i >= tokeni.size() || tokeni[i].vrsta != VrstaTokena::ZAPETA) {
            throw AsemblerskaGreska{naredba + ": ocekivana zapeta pre odredista"};
        }
        i++;
    }

    // Odredisni operand: literal ili simbol.
    if (i >= tokeni.size() || tokeni[i].vrsta == VrstaTokena::KRAJ) {
        throw AsemblerskaGreska{naredba + ": nedostaje odrediste skoka"};
    }

    size_t idxStavke;
    if (tokeni[i].vrsta == VrstaTokena::BROJ) {
        idxStavke = dodajLiteralUBazen(static_cast<u32>(tokeni[i].broj));
        i++;
    } else if (tokeni[i].vrsta == VrstaTokena::IDENT) {
        idxStavke = dodajSimbolUBazen(tokeni[i].tekst);
        i++;
    } else {
        throw AsemblerskaGreska{naredba + ": odrediste mora biti literal ili simbol"};
    }

    // Generisi instrukciju skoka preko memorije: A=pc, cilj = mem32[pc + D],
    // gde D pokazuje na stavku u bazenu (zakrpljuje se na kraju sekcije).
    u32 offInstr = sek->lokacija();
    sek->dodajRec(sklopiInstrukciju(oc, mod, REG_PC, regB, regC, 0));
    sek->zakrpe.push_back(Zakrpa{offInstr, idxStavke});
}