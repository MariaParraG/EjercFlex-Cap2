/* ============================================================
 * Ejercicio 2 - Capítulo 2: Flex & Bison
 * Archivo: concordance.l
 *
 * Problema: El programa de concordancia trata mayúsculas y
 * minúsculas como palabras distintas. Modificarlo para que
 * las trate como iguales SIN hacer copias extra de las palabras.
 *
 * SOLUCIÓN:
 *   - symhash() convierte cada carácter con tolower() AL VUELO
 *     dentro del cálculo del hash (sin reservar memoria extra).
 *   - La comparación usa strcasecmp() sobre el nombre original
 *     almacenado, sin necesidad de una copia en minúsculas.
 * ============================================================ */

%option noyywrap
%option yylineno

%{
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

/* ── Tabla de símbolos ─────────────────────────────────────── */
#define NHASH 9997   /* número de cubetas (primo para mejor distribución) */

struct sym {
    char        *name;    /* ortografía original (tal como aparece la primera vez) */
    int          refcnt;  /* cuántas veces se ha visto la palabra */
    struct sym  *next;    /* siguiente entrada en la cadena (colisiones) */
};

static struct sym *symtab[NHASH];

/*
 * Función de hash: opera sobre la versión EN MINÚSCULAS de la clave,
 * de modo que "Hola", "hola" y "HOLA" caen en la misma cubeta.
 * NO se reserva memoria extra: se convierte cada carácter con tolower()
 * dentro del propio cálculo.
 */
static unsigned symhash(const char *s)
{
    unsigned h = 0;
    while (*s)
        h = h * 9 ^ (unsigned)tolower((unsigned char)*s++);
    return h % NHASH;
}

/*
 * Busca (o inserta) una palabra en la tabla.
 * La comparación usa strcasecmp() para que "Hola" == "HOLA" == "hola"
 * sin necesitar una copia adicional en minúsculas.
 */
struct sym *lookup(const char *s)
{
    unsigned    h  = symhash(s);
    struct sym *sp = symtab[h];

    /* Recorre la cadena con comparación sin distinción de mayúsculas */
    for (; sp; sp = sp->next)
        if (strcasecmp(sp->name, s) == 0)
            return sp;

    /* No se encontró — insertar nueva entrada */
    sp = malloc(sizeof *sp);
    if (!sp) { perror("malloc"); exit(1); }

    sp->name   = strdup(s);   /* guarda la ortografía original */
    sp->refcnt = 0;
    sp->next   = symtab[h];
    symtab[h]  = sp;
    return sp;
}

/* Comparador para ordenar la concordancia alfabéticamente */
static int sym_cmp(const void *a, const void *b)
{
    const struct sym *sa = *(const struct sym **)a;
    const struct sym *sb = *(const struct sym **)b;
    return strcasecmp(sa->name, sb->name);
}

/* Imprime la tabla de concordancia ordenada por nombre */
void imprimir_concordancia(void)
{
    /* Contar entradas totales */
    int n = 0;
    for (int i = 0; i < NHASH; i++)
        for (struct sym *sp = symtab[i]; sp; sp = sp->next)
            n++;

    /* Copiar punteros a un arreglo para ordenarlos */
    struct sym **arr = malloc(n * sizeof *arr);
    if (!arr) { perror("malloc"); exit(1); }

    int k = 0;
    for (int i = 0; i < NHASH; i++)
        for (struct sym *sp = symtab[i]; sp; sp = sp->next)
            arr[k++] = sp;

    qsort(arr, n, sizeof *arr, sym_cmp);

    printf("\n%-30s %s\n", "Palabra", "Frecuencia");
    printf("%-30s %s\n",   "------------------------------", "----------");
    for (int i = 0; i < n; i++)
        printf("%-30s %d\n", arr[i]->name, arr[i]->refcnt);

    free(arr);
}
%}

%%

[a-zA-Z]+   {
                /*
                 * Toda secuencia de letras es una "palabra".
                 * lookup() usa tolower() en el hash y strcasecmp()
                 * en la comparación — no se hace copia extra.
                 */
                struct sym *sp = lookup(yytext);
                sp->refcnt++;
            }

[^a-zA-Z]+  {
                /* Ignorar caracteres no alfabéticos (espacios, puntuación, etc.) */
            }

%%

int main(int argc, char **argv)
{
    if (argc > 1) {
        FILE *f = fopen(argv[1], "r");
        if (!f) { perror(argv[1]); return 1; }
        yyin = f;
    }

    yylex();
    imprimir_concordancia();
    return 0;
}
