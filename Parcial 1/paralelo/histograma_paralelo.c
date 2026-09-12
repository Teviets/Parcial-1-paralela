/*
 * Problema 1: El Histograma Masivo (version paralela con OpenMP)
 *
 * Estrategia:
 *   - Cada thread mantiene su PROPIO histograma local de 100 cubetas
 *     (evita condiciones de carrera sin necesidad de locks/atomics y
 *     evita false sharing porque cada arreglo local vive en la pila
 *     de su propio thread).
 *   - El arreglo de entrada se reparte con "schedule(static)" porque
 *     cada elemento cuesta exactamente lo mismo de procesar (una
 *     resta, una division y un incremento), asi que no hay
 *     desbalance de carga entre trabajadores: dividir el arreglo en
 *     bloques contiguos iguales es optimo y minimiza overhead.
 *   - Al final, se combinan (reduccion manual) los histogramas
 *     locales en el histograma global. Esta combinacion es O(N_threads
 *     * 100), insignificante frente a los N elementos originales.
 *
 * Uso: ./histograma_paralelo [N] [num_threads]
 */

#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

#define NUM_CUBETAS 100
#define TEMP_MIN -50.0
#define TEMP_MAX 50.0

static double rango_por_cubeta(void) {
    return (TEMP_MAX - TEMP_MIN) / NUM_CUBETAS;
}

static inline int cubeta_para(float valor, double ancho) {
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
        double r = (double)rand() / (double)RAND_MAX;
        datos[i] = (float)(TEMP_MIN + r * (TEMP_MAX - TEMP_MIN));
    }
    return datos;
}

int main(int argc, char *argv[]) {
    long n = 100000000L;
    int num_threads = 0; /* 0 = usar el default de OpenMP */

    if (argc > 1) n = atol(argv[1]);
    if (argc > 2) num_threads = atoi(argv[2]);
    if (num_threads > 0) omp_set_num_threads(num_threads);

    printf("Generando %ld mediciones...\n", n);
    float *datos = generar_datos(n, 42);

    long histograma[NUM_CUBETAS] = {0};
    double ancho = rango_por_cubeta();

    double inicio = omp_get_wtime();

    int hilos_usados = 0;

    #pragma omp parallel
    {
        long local[NUM_CUBETAS] = {0};

        #pragma omp single
        {
            hilos_usados = omp_get_num_threads();
        }

        #pragma omp for schedule(static)
        for (long i = 0; i < n; i++) {
            int b = cubeta_para(datos[i], ancho);
            local[b]++;
        }

        /* Combinar histograma local en el global: seccion critica
           breve (100 sumas), ejecutada una sola vez por thread. */
        #pragma omp critical
        {
            for (int b = 0; b < NUM_CUBETAS; b++) {
                histograma[b] += local[b];
            }
        }
    }

    double fin = omp_get_wtime();
    double tiempo = fin - inicio;

    long total = 0;
    for (int i = 0; i < NUM_CUBETAS; i++) total += histograma[i];

    printf("Threads usados: %d\n", hilos_usados);
    printf("Tiempo paralelo: %.6f segundos\n", tiempo);
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
