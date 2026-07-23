# ============================================================================
# Makefile za projekat iz Sistemskog softvera
#
# Gradi tri izvrsna programa: asembler, linker i emulator.
# Svaki od njih ima svoj .cpp fajl sa main funkcijom u src/ direktorijumu,
# a zajednicke module (koje dodajemo kasnije) delice preko src/ i inc/.
# ============================================================================

# --- Alati i opcije prevodjenja -------------------------------------------

# Koji kompajler koristimo.
CXX := g++

# Opcije prevodjenja:
#   -std=c++17  koristimo C++17 standard (dovoljno moderan, siroko podrzan)
#   -Wall -Wextra  ukljucuju vecinu korisnih upozorenja - pomazu da rano
#                  uhvatimo greske (nekoriscene promenljive, sumnjiva poredjenja...)
#   -g          ubacuje debug informacije da bismo mogli da koristimo gdb
#   -Iinc       kaze kompajleru da zaglavlja trazi i u inc/ direktorijumu
CXXFLAGS := -std=c++17 -Wall -Wextra -g -Iinc

# --- Direktorijumi ---------------------------------------------------------

SRC_DIR := src
INC_DIR := inc

# --- Ciljevi ---------------------------------------------------------------

# Podrazumevani cilj: kada se pokrene samo "make", gradi sva tri programa.
# Podrazumevani cilj gradi sve alate. Pored naziva "asembler" pravi se i
# "assembler" (englesko ime) jer ga zvanicni testovi predmeta koriste.
all: asembler assembler linker emulator

# Svaki program se zasad sastoji od jednog .cpp fajla. Kada budemo dodavali
# zajednicke module, ovde cemo prosiriti spiskove zavisnosti.

# Asembler se sastoji od vise modula: ulazna tacka (asembler.cpp), logika
# parsiranja (parser.cpp), lekser i tabela simbola.
ASM_SRC := $(SRC_DIR)/asembler.cpp \
           $(SRC_DIR)/parser.cpp \
           $(SRC_DIR)/naredbe.cpp \
           $(SRC_DIR)/operandi.cpp \
           $(SRC_DIR)/bazen.cpp \
           $(SRC_DIR)/izlaz.cpp \
           $(SRC_DIR)/lekser.cpp \
           $(SRC_DIR)/registri.cpp \
           $(SRC_DIR)/tabela_simbola.cpp

asembler: $(ASM_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Linker se sastoji od ulazne tacke i modula za citanje predmetnih programa.
LINK_SRC := $(SRC_DIR)/linker.cpp \
            $(SRC_DIR)/jezgro.cpp \
            $(SRC_DIR)/citac.cpp \
            $(SRC_DIR)/hex_izlaz.cpp \
            $(SRC_DIR)/izlaz.cpp \
            $(SRC_DIR)/tabela_simbola.cpp

linker: $(LINK_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

# Emulator: ulazna tacka i moduli za memoriju i ucitavanje.
EMU_SRC := $(SRC_DIR)/emulator.cpp \
           $(SRC_DIR)/emulacija.cpp \
           $(SRC_DIR)/terminal.cpp \
           $(SRC_DIR)/tajmer.cpp \
           $(SRC_DIR)/ucitavanje.cpp

emulator: $(EMU_SRC)
	$(CXX) $(CXXFLAGS) -o $@ $^

# --- Pomocni ciljevi -------------------------------------------------------

# "make clean" brise sve izgenerisan fajlove i vraca projekat u cisto stanje.
# Kopija asemblera pod engleskim nazivom, radi kompatibilnosti sa testovima.
assembler: asembler
	cp asembler assembler

clean:
	rm -f asembler assembler linker emulator

# .PHONY kaze make-u da "all" i "clean" nisu nazivi fajlova nego imena akcija,
# pa ih uvek izvrsava (a ne proverava da li fajl "clean" postoji i da li je star).
.PHONY: all clean