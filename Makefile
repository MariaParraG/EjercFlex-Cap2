# Makefile — Ejercicios Capítulo 2 (Flex & Bison)
# Requisitos: flex, gcc

CC     = gcc
CFLAGS = -Wall -Wextra -g
LEX    = flex

.PHONY: all ej1 ej2 ej3 limpiar probar

all: ej1 ej2 ej3

# ── Ejercicio 1: Match línea por línea ───────────────────────
ej1: ex1/linematch

ex1/lex.yy.c: ex1/linematch.l
	$(LEX) -o $@ $<

ex1/linematch: ex1/lex.yy.c
	$(CC) $(CFLAGS) -o $@ $< -lfl 2>/dev/null || $(CC) $(CFLAGS) -o $@ $<

# ── Ejercicio 2: Concordancia sin distinción de mayúsculas ───
ej2: ex2/concordance

ex2/lex.yy.c: ex2/concordance.l
	$(LEX) -o $@ $<

ex2/concordance: ex2/lex.yy.c
	$(CC) $(CFLAGS) -o $@ $< -lfl 2>/dev/null || $(CC) $(CFLAGS) -o $@ $<

# ── Ejercicio 3: Tabla de símbolos de tamaño variable ────────
ej3: ex3/symtable

ex3/symtable: ex3/symtable.c
	$(CC) $(CFLAGS) -o $@ $<

# ── Pruebas rápidas ───────────────────────────────────────────
probar: all
	@echo "=== Ejercicio 1: Match por línea ==="
	@printf "Hola Mundo\nSegunda Línea\nTercera Línea\n" | ./ex1/linematch

	@echo ""
	@echo "=== Ejercicio 2: Concordancia sin distinción de mayúsculas ==="
	@printf "El gato se sentó. EL GATO se fue. el Gato volvió." | ./ex2/concordance

	@echo ""
	@echo "=== Ejercicio 3: Tabla de símbolos variable ==="
	@./ex3/symtable

limpiar:
	rm -f ex1/lex.yy.c ex1/linematch \
	      ex2/lex.yy.c ex2/concordance \
	      ex3/symtable
