/* ============================================================
 * Ejercicio 1 - Capítulo 2: Flex & Bison
 * Archivo: linematch.l
 *
 * Problema: El ejemplo 2-3 hace match carácter por carácter.
 * ¿Por qué no funciona un patrón como ^.*\n para hacer match
 * línea por línea? Propone patrones alternativos.
 *
 * RESPUESTA:
 * ^.*\n no funciona porque en flex el punto (.) NO hace match
 * con \n por defecto. Entonces ^.*\n intenta coincidir con:
 *   - inicio de línea (^)
 *   - cero o más caracteres que NO sean \n (.*)
 *   - un salto de línea (\n)
 *
 * El problema real es que al trabajar carácter por carácter
 * como en el ejemplo 2-3, el ancla ^ pierde su contexto de
 * "inicio de línea". Además, si la última línea del archivo
 * no tiene \n al final, ^.*\n no haría match con ese contenido,
 * dejándolo sin procesar.
 *
 * SOLUCIÓN: Usar los patrones que se muestran a continuación.
 * ============================================================ */

%option noyywrap
%option yylineno   /* flex lleva el conteo de líneas automáticamente */

%%

^[^\n]*\n    {
                /*
                 * Patrón recomendado: hace match de una línea completa.
                 *   ^      = ancla de inicio de línea
                 *   [^\n]* = cero o más caracteres que NO sean salto de línea
                 *   \n     = el salto de línea que termina la línea
                 *
                 * Es más explícito que ^.*\n y funciona siempre,
                 * sin necesidad de opciones especiales de flex.
                 */
                printf("Línea %d: %s", yylineno - 1, yytext);
             }

<<EOF>>      {
                /*
                 * Maneja el caso especial en que la última línea
                 * del archivo no tiene \n al final.
                 */
                if (yyleng > 0)
                    printf("Línea %d (sin \\n al final): %s\n", yylineno, yytext);
                yyterminate();
             }

%%

int main(int argc, char **argv)
{
    /* Si se pasa un archivo como argumento, lo abre; si no, lee de stdin */
    if (argc > 1) {
        FILE *f = fopen(argv[1], "r");
        if (!f) { perror(argv[1]); return 1; }
        yyin = f;
    }
    yylex();
    return 0;
}
