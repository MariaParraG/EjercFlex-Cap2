# Capítulo 2 — Ejercicios de Flex & Bison

Soluciones implementadas de los tres ejercicios del Capítulo 2 del libro *Flex & Bison* de John Levine (O'Reilly).

---

## Estructura del proyecto

```
ch2_exercises/
├── README.md          ← este archivo
├── Makefile           ← compila y prueba todos los ejercicios
├── ex1/
│   └── linematch.l    ← Ejercicio 1: match línea por línea
├── ex2/
│   └── concordance.l  ← Ejercicio 2: concordancia sin distinción de mayúsculas
└── ex3/
    └── symtable.c     ← Ejercicio 3: tabla de símbolos de tamaño variable
```

---

## Requisitos

| Herramienta | Versión mínima | Propósito |
|-------------|---------------|-----------|
| `flex`      | 2.6+          | Generar el analizador léxico para ej1 y ej2 |
| `gcc`       | 9+            | Compilar todos los archivos |
| `make`      | 4+            | Automatizar la compilación |

Instalación en sistemas Debian/Ubuntu:
```bash
sudo apt install flex gcc make
```

---

## Compilar y probar

```bash
# Compilar todo
make all

# Ejecutar pruebas rápidas de los tres ejercicios
make probar

# Compilar ejercicio individual
make ej1   # o ej2, ej3

# Limpiar archivos generados
make limpiar
```

---

## Ejercicio 1 — Match línea por línea

**Archivo:** `ex1/linematch.l`

### Pregunta del libro

El ejemplo 2-3 hace match de los caracteres uno a la vez. ¿Por qué no funciona el patrón `^.*\n` para hacer match de una línea completa? Propone un patrón o combinación de patrones que sí funcione.

### Respuesta

El problema con `^.*\n` es que **en flex el punto (`.`) no hace match con `\n` por defecto**. El patrón intenta coincidir con:

- `^` → ancla de inicio de línea
- `.*` → cero o más caracteres que **no sean** `\n`
- `\n` → el salto de línea

Esto falla por dos razones principales:

1. **Contexto del ancla `^`**: al trabajar carácter por carácter como en el ejemplo 2-3, el estado de "inicio de línea" puede perderse o entrar en conflicto con el diseño de estados del escáner.
2. **Última línea sin `\n`**: si el archivo no termina en salto de línea, `^.*\n` nunca haría match con el último renglón y ese contenido quedaría sin procesar.

### Solución implementada

| Patrón | Descripción |
|--------|-------------|
| `^[^\n]*\n` | **Recomendado**: coincide con todos los caracteres que no son `\n`, seguidos del `\n` final |
| `^.*\n` | Funciona sólo con `%option dotall` (hace que `.` también coincida con `\n`) |
| `<<EOF>>` | Maneja la última línea si no tiene `\n` al final |

```flex
^[^\n]*\n    { printf("Línea %d: %s", yylineno - 1, yytext); }

<<EOF>>      {
                if (yyleng > 0)
                    printf("Línea %d (sin \\n): %s\n", yylineno, yytext);
                yyterminate();
             }
```

### Prueba manual

```bash
echo -e "Primera línea\nSegunda línea\nTercera línea" | ./ex1/linematch
```

Salida esperada:
```
Línea 1: Primera línea
Línea 2: Segunda línea
Línea 3: Tercera línea
```

---

## Ejercicio 2 — Concordancia sin distinción de mayúsculas

**Archivo:** `ex2/concordance.l`

### Pregunta del libro

El programa de concordancia trata las mayúsculas y minúsculas como palabras distintas. Modifícalo para tratarlas como iguales **sin hacer copias extra** de las palabras. Usa `tolower()` en el hash y `strcasecmp()` para comparar.

### Solución implementada

La clave es **no almacenar una copia en minúsculas**. En su lugar:

1. **`symhash()`** convierte cada carácter con `tolower()` *al vuelo*, dentro del propio cálculo aritmético del hash, sin reservar memoria adicional.
2. **`strcasecmp()`** compara el nombre original almacenado con la nueva palabra de forma insensible a mayúsculas.

```c
/* Hash sobre minúsculas, sin copia extra */
static unsigned symhash(const char *s)
{
    unsigned h = 0;
    while (*s)
        h = h * 9 ^ (unsigned)tolower((unsigned char)*s++);
    return h % NHASH;
}

/* Comparación insensible a mayúsculas */
struct sym *lookup(const char *s)
{
    unsigned h = symhash(s);
    for (struct sym *sp = symtab[h]; sp; sp = sp->next)
        if (strcasecmp(sp->name, s) == 0)   /* ← sin copia */
            return sp;
    /* ... insertar nueva entrada ... */
}
```

Así, `"Hola"`, `"hola"` y `"HOLA"` caen en la misma cubeta y se reconocen como la misma palabra.

### Prueba manual

```bash
echo "El gato se sentó. EL GATO volvió. el Gato durmió." | ./ex2/concordance
```

Salida esperada:
```
Palabra                        Frecuencia
------------------------------ ----------
El                             3
gato                           3
se                             2
sentó                          1
volvió                         1
durmió                         1
```

*(El conteo de "El/el/EL" y "gato/GATO/Gato" se unifica correctamente.)*

---

## Ejercicio 3 — Tabla de símbolos de tamaño variable

**Archivo:** `ex3/symtable.c`

### Pregunta del libro

La rutina de tabla de símbolos usa una tabla de tamaño fijo y falla si se llena. Modifícala para que no pase eso. Las dos técnicas estándar son **encadenamiento** (*chaining*) y **rehashing**. ¿Cuál hace más complicado el programa de referencias cruzadas? ¿Por qué?

### Técnica A — Encadenamiento (*Chaining*)

La tabla hash es un **arreglo de listas enlazadas**. Cada cubeta apunta al inicio de una cadena de entradas.

```
cubeta[0] → sym("el") → sym("se") → NULL
cubeta[1] → sym("gato") → NULL
cubeta[2] → NULL
   ...
```

**Crecimiento**: cuando el factor de carga supera 2, se duplica el arreglo de cubetas y se redistribuyen los punteros. Los `struct sym` **nunca se mueven** en memoria.

```c
static void enc_ampliar(TablaEncadenada *t)
{
    /* Nuevo arreglo de cubetas, el doble de grande */
    struct sym **nuevas = calloc(t->ncubetas * 2, sizeof *nuevas);

    /* Redistribuir sólo los punteros — los structs no se tocan */
    for (int i = 0; i < t->ncubetas; i++) {
        struct sym *sp = t->cubetas[i];
        while (sp) {
            struct sym *sig = sp->next;
            unsigned h = enc_hash(sp->name, t->ncubetas * 2);
            sp->next  = nuevas[h];
            nuevas[h] = sp;
            sp = sig;
        }
    }
    free(t->cubetas);
    t->cubetas  = nuevas;
    t->ncubetas *= 2;
}
```

### Técnica B — Rehashing

Arreglo plano con **direccionamiento abierto** (sondeo lineal). Cuando el arreglo llega al 50% de ocupación, se asigna uno nuevo del doble de tamaño y se reinsertan todas las entradas.

```c
static void hacer_rehash(TablaRehash *t)
{
    RanuraRehash *viejo = t->ranuras;
    int viejo_n = t->nranuras;

    t->ranuras  = calloc(viejo_n * 2, sizeof *t->ranuras);
    t->nranuras = viejo_n * 2;

    for (int i = 0; i < viejo_n; i++) {
        if (!viejo[i].usado) continue;
        /* La entrada se COPIA a una nueva posición de memoria */
        unsigned h = reh_hash(viejo[i].name, t->nranuras);
        while (t->ranuras[h].usado) h = (h+1) % t->nranuras;
        t->ranuras[h] = viejo[i];  /* ← struct copy, nueva dirección */
    }
    free(viejo);
    /* ⚠ Todos los punteros anteriores a ranuras son ahora inválidos */
}
```

### ¿Cuál es más complicada para referencias cruzadas?

**El rehashing es más complicado**, y la razón es la validez de los punteros:

| Aspecto | Encadenamiento | Rehashing |
|---------|---------------|-----------|
| ¿Las entradas se mueven? | **No** — `malloc()` fija su dirección | **Sí** — se copian a nuevas posiciones |
| Punteros externos (`sym*`) | **Siempre válidos** | **Inválidos** tras cada rehash |
| Complejidad extra | Ninguna | Hay que rastrear y actualizar todos los `sym*` externos |

En un programa de referencias cruzadas, las listas de números de línea guardan punteros `sym*` a entradas de la tabla. Con rehashing, cada vez que la tabla crece esos punteros apuntan a memoria liberada (**dangling pointers**), lo que obliga a:

- Hacer una segunda pasada para actualizar todos los punteros externos, o
- Usar una capa de indirección adicional (ej. índice entero en vez de puntero).

Con **encadenamiento**, los `struct sym` nunca cambian de dirección, por lo que cualquier `sym*` guardado externamente sigue siendo válido indefinidamente. Es la técnica recomendada para referencias cruzadas.

### Prueba manual

```bash
./ex3/symtable
```

Salida esperada:
```
=== Referencias Cruzadas (Encadenamiento) ===

Palabra              Líneas
-------------------- ------
del                   3
el                    1 2 3
en                    1
es                    4
gato                  1 2 3 4
huyó                  3
la                    2 3
no                    4
rata                  2 3 4
se                    1
sentó                 1
un                    4
una                   4

=== Discusión: Encadenamiento vs Rehashing ...
```

---

## Referencias

- Levine, John. *Flex & Bison*. O'Reilly Media, 2009.
- Documentación oficial de GNU flex: https://github.com/westes/flex
