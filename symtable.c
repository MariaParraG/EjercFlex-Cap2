/* ============================================================
 * Ejercicio 3 - Capítulo 2: Flex & Bison
 * Archivo: symtable.c
 *
 * Problema: La tabla de símbolos de tamaño fijo falla cuando
 * se llena. Implementar ambas técnicas de tamaño variable:
 *
 *   (A) ENCADENAMIENTO  — tabla hash de listas enlazadas
 *   (B) REHASHING       — duplicar el arreglo cuando se llena
 *
 * Pregunta del libro: ¿cuál técnica hace más complicado el
 * programa de referencias cruzadas (cross-referencer)? ¿Por qué?
 *
 * RESPUESTA (ver discusión al final del main):
 *   El REHASHING es más complicado para referencias cruzadas
 *   porque al mover las entradas a nuevas posiciones de memoria,
 *   cualquier puntero externo (ej. listas de números de línea
 *   que apuntan a entradas de la tabla) queda INVÁLIDO tras
 *   cada rehash. Con encadenamiento las entradas NUNCA se mueven,
 *   por lo que los punteros externos siempre permanecen válidos.
 * ============================================================ */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* ── Lista de números de línea (para referencias cruzadas) ── */
struct linelist {
    int              lineno;
    struct linelist *next;
};

/* ── Entrada de símbolo ─────────────────────────────────────── */
struct sym {
    char            *name;   /* nombre de la palabra */
    struct linelist *lines;  /* lista de líneas donde aparece */
    struct sym      *next;   /* siguiente en la cadena (colisiones) */
};

/* ============================================================
 * TÉCNICA A — ENCADENAMIENTO (Chaining)
 *
 * La tabla hash es un arreglo de listas enlazadas.
 * Las cubetas crecen dinámicamente; las entradas NUNCA se mueven.
 * Los punteros externos a struct sym permanecen válidos siempre.
 * ============================================================ */

#define CUBETAS_INICIALES 16   /* se empieza pequeño para demostrar el crecimiento */

typedef struct {
    struct sym **cubetas;   /* arreglo de punteros al inicio de cada cadena */
    int          ncubetas;  /* número actual de cubetas */
    int          nentradas; /* total de entradas insertadas */
} TablaEncadenada;

static TablaEncadenada tabla_enc;

/* Inicializa la tabla encadenada */
static void enc_inicializar(TablaEncadenada *t)
{
    t->ncubetas  = CUBETAS_INICIALES;
    t->nentradas = 0;
    t->cubetas   = calloc(t->ncubetas, sizeof *t->cubetas);
    if (!t->cubetas) { perror("calloc"); exit(1); }
}

/* Función de hash genérica para cadenas */
static unsigned enc_hash(const char *s, int ncubetas)
{
    unsigned h = 0;
    while (*s) h = h * 9 ^ (unsigned char)*s++;
    return h % (unsigned)ncubetas;
}

/*
 * Amplía el arreglo de cubetas cuando el factor de carga supera 2.
 *
 * NOTA IMPORTANTE: aquí sólo se redistribuyen los punteros (struct sym*)
 * entre las nuevas cubetas — los structs sym en sí NO se mueven de lugar
 * en memoria. Por eso los punteros externos siguen siendo válidos.
 */
static void enc_ampliar(TablaEncadenada *t)
{
    int         nuevas_n = t->ncubetas * 2;
    struct sym **nuevas  = calloc(nuevas_n, sizeof *nuevas);
    if (!nuevas) { perror("calloc"); exit(1); }

    /* Redistribuir cada entrada a su nueva cubeta */
    for (int i = 0; i < t->ncubetas; i++) {
        struct sym *sp = t->cubetas[i];
        while (sp) {
            struct sym *siguiente = sp->next;
            unsigned h = enc_hash(sp->name, nuevas_n);
            sp->next  = nuevas[h];
            nuevas[h] = sp;
            sp = siguiente;
        }
    }
    free(t->cubetas);
    t->cubetas  = nuevas;
    t->ncubetas = nuevas_n;
    fprintf(stderr, "[encadenamiento] tabla ampliada a %d cubetas\n", nuevas_n);
}

/* Busca o inserta un símbolo usando encadenamiento */
struct sym *enc_buscar(TablaEncadenada *t, const char *nombre)
{
    unsigned    h  = enc_hash(nombre, t->ncubetas);
    struct sym *sp;

    /* Buscar en la cadena de esta cubeta */
    for (sp = t->cubetas[h]; sp; sp = sp->next)
        if (strcmp(sp->name, nombre) == 0)
            return sp;

    /* Ampliar si el factor de carga supera 2 */
    if (t->nentradas > t->ncubetas * 2)
        enc_ampliar(t);

    /* Recalcular hash tras posible ampliación */
    h  = enc_hash(nombre, t->ncubetas);
    sp = malloc(sizeof *sp);
    if (!sp) { perror("malloc"); exit(1); }

    sp->name     = strdup(nombre);
    sp->lines    = NULL;
    sp->next     = t->cubetas[h];
    t->cubetas[h] = sp;
    t->nentradas++;
    return sp;
}

/* ============================================================
 * TÉCNICA B — REHASHING
 *
 * Arreglo plano con direccionamiento abierto (sondeo lineal).
 * Cuando el arreglo se llena a la mitad, se asigna uno nuevo
 * del doble de tamaño y se reinsertan todas las entradas.
 *
 * ⚠ PROBLEMA PARA REFERENCIAS CRUZADAS:
 *   Tras cada rehash, cada entrada se copia a una nueva posición
 *   en memoria. Cualquier puntero del tipo `struct sym *ref`
 *   guardado fuera de la tabla queda INVÁLIDO (dangling pointer).
 *   Habría que rastrear y actualizar TODOS los punteros externos
 *   después de cada rehash — lo que complica enormemente el
 *   programa de referencias cruzadas.
 * ============================================================ */

typedef struct {
    char            *name;   /* nombre de la palabra */
    struct linelist *lines;  /* lista de líneas donde aparece */
    int              usado;  /* 1 si la ranura está ocupada */
} RanuraRehash;

typedef struct {
    RanuraRehash *ranuras;   /* arreglo plano de ranuras */
    int           nranuras;  /* tamaño actual del arreglo */
    int           nentradas; /* entradas ocupadas */
} TablaRehash;

static TablaRehash tabla_reh;

/* Inicializa la tabla con rehashing */
static void reh_inicializar(TablaRehash *t)
{
    t->nranuras  = CUBETAS_INICIALES;
    t->nentradas = 0;
    t->ranuras   = calloc(t->nranuras, sizeof *t->ranuras);
    if (!t->ranuras) { perror("calloc"); exit(1); }
}

static unsigned reh_hash(const char *s, int n)
{
    unsigned h = 0;
    while (*s) h = h * 9 ^ (unsigned char)*s++;
    return h % (unsigned)n;
}

/*
 * Ejecuta el rehashing: asigna un arreglo del doble de tamaño
 * y reinserta todas las entradas existentes.
 *
 * ⚠ Después de esta llamada, todos los punteros RanuraRehash*
 *   obtenidos previamente apuntan a memoria liberada (inválidos).
 */
static void hacer_rehash(TablaRehash *t)
{
    int           viejo_n = t->nranuras;
    RanuraRehash *viejo_r = t->ranuras;
    int           nuevo_n = viejo_n * 2;

    t->ranuras   = calloc(nuevo_n, sizeof *t->ranuras);
    t->nranuras  = nuevo_n;
    t->nentradas = 0;
    if (!t->ranuras) { perror("calloc"); exit(1); }

    /* Reinsertar cada entrada del arreglo viejo en el nuevo */
    for (int i = 0; i < viejo_n; i++) {
        if (!viejo_r[i].usado) continue;
        unsigned h = reh_hash(viejo_r[i].name, nuevo_n);
        /* Sondeo lineal para encontrar ranura libre */
        while (t->ranuras[h].usado)
            h = (h + 1) % (unsigned)nuevo_n;
        t->ranuras[h] = viejo_r[i];   /* copia del struct — ¡cambia la dirección! */
        t->nentradas++;
    }
    free(viejo_r);
    fprintf(stderr, "[rehashing] tabla ampliada a %d ranuras\n", nuevo_n);
    /* ATENCIÓN: todos los RanuraRehash* anteriores ya son inválidos */
}

/* Busca o inserta un símbolo usando rehashing */
RanuraRehash *reh_buscar(TablaRehash *t, const char *nombre)
{
    /* Ampliar si se supera el 50% de ocupación */
    if (t->nentradas >= t->nranuras / 2)
        hacer_rehash(t);

    unsigned h = reh_hash(nombre, t->nranuras);
    while (t->ranuras[h].usado) {
        if (strcmp(t->ranuras[h].name, nombre) == 0)
            return &t->ranuras[h];
        h = (h + 1) % (unsigned)t->nranuras;   /* sondeo lineal */
    }

    /* Ranura libre encontrada — insertar */
    t->ranuras[h].name  = strdup(nombre);
    t->ranuras[h].lines = NULL;
    t->ranuras[h].usado = 1;
    t->nentradas++;
    return &t->ranuras[h];
}

/* ── Utilidades comunes ─────────────────────────────────────── */

/* Agrega un número de línea a la lista, evitando duplicados */
static void agregar_linea(struct linelist **cabeza, int lineno)
{
    for (struct linelist *p = *cabeza; p; p = p->next)
        if (p->lineno == lineno) return;   /* ya estaba en la lista */

    struct linelist *nodo = malloc(sizeof *nodo);
    nodo->lineno = lineno;
    nodo->next   = *cabeza;
    *cabeza      = nodo;
}

/* Imprime los números de línea ordenados */
static void imprimir_lineas(struct linelist *p)
{
    /* Recolectar en un arreglo para ordenar */
    int n = 0;
    for (struct linelist *q = p; q; q = q->next) n++;
    int *arr = malloc(n * sizeof *arr);
    int  i   = 0;
    for (; p; p = p->next) arr[i++] = p->lineno;

    /* Ordenamiento por inserción (arreglos pequeños) */
    for (int a = 1; a < n; a++)
        for (int b = a; b > 0 && arr[b-1] > arr[b]; b--) {
            int t = arr[b]; arr[b] = arr[b-1]; arr[b-1] = t;
        }
    for (int j = 0; j < n; j++) printf(" %d", arr[j]);
    free(arr);
}

/* ── Programa demostrativo (usa ENCADENAMIENTO, la opción recomendada) ── */

static int linea_actual = 1;

/* Verifica si un carácter es letra */
static int es_letra(char c)
{
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

/* Tokenizador simple: divide el texto en palabras y las registra */
static void tokenizar(const char *texto)
{
    const char *p = texto;
    while (*p) {
        if (*p == '\n') { linea_actual++; p++; continue; }
        if (es_letra(*p)) {
            const char *inicio = p;
            while (es_letra(*p)) p++;
            int   largo = (int)(p - inicio);
            char *palabra = malloc(largo + 1);
            memcpy(palabra, inicio, largo);
            palabra[largo] = '\0';

            /* Registrar la palabra y su número de línea */
            struct sym *sp = enc_buscar(&tabla_enc, palabra);
            agregar_linea(&sp->lines, linea_actual);
            free(palabra);
        } else {
            p++;
        }
    }
}

/* Comparador para ordenar la referencia cruzada alfabéticamente */
static int cmp_nombre(const void *a, const void *b)
{
    return strcmp((*(struct sym **)a)->name, (*(struct sym **)b)->name);
}

/* Imprime la tabla de referencias cruzadas ordenada */
static void imprimir_xref(TablaEncadenada *t)
{
    int n = t->nentradas;
    struct sym **arr = malloc(n * sizeof *arr);
    int k = 0;
    for (int i = 0; i < t->ncubetas; i++)
        for (struct sym *sp = t->cubetas[i]; sp; sp = sp->next)
            arr[k++] = sp;
    qsort(arr, n, sizeof *arr, cmp_nombre);

    printf("\n%-20s Líneas\n", "Palabra");
    printf("%-20s ------\n",  "--------------------");
    for (int i = 0; i < n; i++) {
        printf("%-20s", arr[i]->name);
        imprimir_lineas(arr[i]->lines);
        printf("\n");
    }
    free(arr);
}

int main(void)
{
    enc_inicializar(&tabla_enc);
    reh_inicializar(&tabla_reh);

    /* Texto de ejemplo para generar la referencia cruzada */
    const char *texto =
        "el gato se sentó en el tapete\n"
        "el gato se comió la rata\n"
        "la rata huyó del gato\n"
        "un gato no es una rata\n";

    printf("=== Referencias Cruzadas (Encadenamiento) ===\n");
    tokenizar(texto);
    imprimir_xref(&tabla_enc);

    printf("\n=== Discusión: Encadenamiento vs Rehashing para Referencias Cruzadas ===\n");
    printf(
        "El ENCADENAMIENTO es MEJOR para referencias cruzadas porque:\n"
        "  - Las entradas (struct sym) se crean con malloc() y NUNCA se mueven.\n"
        "  - Los punteros externos (ej. sym* en listas de líneas) permanecen\n"
        "    válidos incluso cuando la tabla crece.\n"
        "  - Al ampliar, sólo se redistribuyen punteros entre cubetas; los\n"
        "    structs sym permanecen en la misma dirección de memoria.\n"
        "\n"
        "El REHASHING es MÁS COMPLICADO para referencias cruzadas porque:\n"
        "  - Las entradas se COPIAN a un nuevo arreglo en posiciones distintas.\n"
        "  - Todo puntero sym* guardado fuera de la tabla queda INVÁLIDO\n"
        "    (dangling pointer) después de cada rehash.\n"
        "  - Habría que rastrear y actualizar TODOS los punteros externos\n"
        "    tras cada rehash, lo cual requiere una segunda pasada o una\n"
        "    capa de indirección adicional — complicando mucho el programa.\n"
    );

    return 0;
}
