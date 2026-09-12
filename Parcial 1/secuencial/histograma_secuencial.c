/*
 * Problema 1: El Histograma Masivo (version secuencial)
 *
 * Clasifica un arreglo de N mediciones de temperatura (punto flotante)
 * en NUM_CUBETAS rangos (buckets) y cuenta cuantas mediciones caen en
 * cada rango.
 *
 * Uso: ./histograma_secuencial [N]
 *   N = cantidad de mediciones a generar (default 100,000,000)
 */

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define NUM_CUBETAS 100
#define TEMP_MIN -50.0
#define TEMP_MAX 50.0

static double rango_por_cubeta(void) {
    return (TEMP_MAX - TEMP_MIN) / NUM_CUBETAS;
}

static int cubeta_para(float valor, double ancho) {
    int idx = (int)((valor - TEMP_MIN) / ancho);
    if (idx < 0) idx = 0;
    if (idx >= NUM_CUBETAS) idx = NUM_CUBETAS - 1;
    return idx;
}

static float *generar_datos(long n, unsigned int seed) {
    float *datos = malloc((size_t)n * sizeof(float));
    if (!datos) {
        fprintf(stderr, "No se pudo reservar memoria para %ld elementos\n", n);
        exit(1);
    }
    srand(seed);
    for (long i = 0; i < n; i++) {
        /* Temperatura simulada entre TEMP_MIN y TEMP_MAX */
        double r = (double)rand() / (double)RAND_MAX;
        datos[i] = (float)(TEMP_MIN + r * (TEMP_MAX - TEMP_MIN));
    }
    return datos;
}

int main(int argc, char *argv[]) {
    long n = 100000000L; /* 100 millones por defecto */
    if (argc > 1) {
        n = atol(argv[1]);
    }

    printf("Generando %ld mediciones...\n", n);
    float *datos = generar_datos(n, 42);

    long histograma[NUM_CUBETAS] = {0};
    double ancho = rango_por_cubeta();

    clock_t inicio = clock();

    for (long i = 0; i < n; i++) {
        int b = cubeta_para(datos[i], ancho);
        histograma[b]++;
    }

    clock_t fin = clock();
    double tiempo = (double)(fin - inicio) / CLOCKS_PER_SEC;

    long total = 0;
    for (int i = 0; i < NUM_CUBETAS; i++) total += histograma[i];

    printf("Tiempo secuencial: %.6f segundos\n", tiempo);
    printf("Total clasificado: %ld (debe ser igual a N=%ld)\n", total, n);

    printf("\nPrimeras 10 cubetas:\n");
    for (int i = 0; i < 10; i++) {
        double lo = TEMP_MIN + i * ancho;
        double hi = lo + ancho;
        printf("  [%6.2f, %6.2f) -> %ld\n", lo, hi, histograma[i]);
    }

    free(datos);
    return 0;
}
