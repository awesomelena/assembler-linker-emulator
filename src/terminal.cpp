// ============================================================================
// terminal.cpp
//
// Implementacija terminal periferije.
// ============================================================================

#include "terminal.hpp"
#include <cstdio>

void terminalIspisi(u32 vrednost) {
    // Displej ispisuje znak odredjen ASCII tabelom za upisanu vrednost.
    // Uzimamo donji bajt jer je ASCII kod osmobitni.
    char znak = static_cast<char>(vrednost & 0xFF);
    std::putchar(znak);

    // Odmah praznimo bafer da bi se znak pojavio na ekranu istog trenutka,
    // a ne tek kada se bafer napuni ili program zavrsi. Za interaktivan
    // terminal ovo je neophodno.
    std::fflush(stdout);
}

// ============================================================================
// Ulazna strana terminala (tastatura) - sirovi rezim preko termios.
// ============================================================================

#include <termios.h>
#include <unistd.h>
#include <fcntl.h>

// Originalna podesavanja terminala, da bismo ih vratili na kraju.
static struct termios prvobitnaPodesavanja;
static bool podesavanjaSacuvana = false;

// Da li je citanje sa standardnog ulaza uspesno prebaceno u neblokirajuci
// rezim. Samo tada smemo da citamo - inace bi read blokirao emulaciju.
static bool neblokirajuciPostavljen = false;

void terminalPripremi() {
    // Bez obzira na to da li je standardni ulaz pravi terminal ili je
    // preusmeren (iz datoteke ili pipe-a), citanje mora biti neblokirajuce
    // kako emulacija ne bi stala cekajuci na unos.
    int zastavice = fcntl(STDIN_FILENO, F_GETFL, 0);
    if (zastavice != -1) {
        fcntl(STDIN_FILENO, F_SETFL, zastavice | O_NONBLOCK);
        neblokirajuciPostavljen = true;
    }

    // Sirovi rezim (bez baferovanja po liniji i bez eho prikaza) ima smisla
    // samo ako je standardni ulaz zaista terminal.
    if (!isatty(STDIN_FILENO)) {
        return;
    }

    // Sacuvaj trenutna podesavanja da bismo ih kasnije vratili.
    if (tcgetattr(STDIN_FILENO, &prvobitnaPodesavanja) != 0) {
        return;  // ne mozemo da procitamo podesavanja - radimo bez sirovog rezima
    }
    podesavanjaSacuvana = true;

    struct termios nova = prvobitnaPodesavanja;

    // ICANON iskljucuje baferovanje po liniji: karakter stize odmah,
    // bez cekanja Enter-a. ECHO iskljucuje prikaz otkucanog znaka.
    nova.c_lflag &= static_cast<tcflag_t>(~(ICANON | ECHO));

    // VMIN=0, VTIME=0 znaci: citanje se vraca odmah, cak i ako nema znaka.
    nova.c_cc[VMIN] = 0;
    nova.c_cc[VTIME] = 0;

    tcsetattr(STDIN_FILENO, TCSANOW, &nova);
}

void terminalVrati() {
    // Vrati prvobitna podesavanja samo ako smo ih uspesno sacuvali.
    if (podesavanjaSacuvana) {
        tcsetattr(STDIN_FILENO, TCSANOW, &prvobitnaPodesavanja);
        podesavanjaSacuvana = false;
    }
}

bool terminalProveriTaster(u8& kod) {
    // Citamo samo ako je citanje neblokirajuce; inace bi read zaustavio
    // emulaciju cekajuci na unos.
    if (!neblokirajuciPostavljen) {
        return false;
    }

    char znak;
    // Zahvaljujuci VMIN=0/VTIME=0, read se vraca odmah:
    //   > 0  procitan je znak,
    //   == 0 nema znaka (nista nije pritisnuto),
    //   < 0  greska (npr. EAGAIN kada nema podataka).
    ssize_t procitano = read(STDIN_FILENO, &znak, 1);
    if (procitano > 0) {
        kod = static_cast<u8>(znak);
        return true;
    }
    return false;
}