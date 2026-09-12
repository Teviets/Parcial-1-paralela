# Documentación — Problema 1: El Histograma Masivo

## 1. Contexto y Datos

### Descripción del problema

Se tiene un arreglo unidimensional de `N` mediciones de temperatura en punto
flotante. Se necesita clasificarlas en 100 rangos (cubetas) de igual ancho,
contando cuántas mediciones caen en cada rango. El reto no es solo repartir
el arreglo entre trabajadores, sino garantizar que el conteo final sea
**exacto** (sin perder ni duplicar mediciones por condiciones de carrera).

### Propuesta secuencial

El algoritmo base (`secuencial/histograma_secuencial.c`) recorre el arreglo
una sola vez con un ciclo `for`. Para cada elemento calcula el índice de su
cubeta con una resta y una división:

```c
int idx = (int)((valor - TEMP_MIN) / ancho_cubeta);
histograma[idx]++;
```

Este cálculo es O(1) por elemento, así que el algoritmo completo es O(N).
No hay dependencias entre iteraciones: el resultado de clasificar el
elemento `i` no depende del resultado de clasificar el elemento `i-1`. Esto
es justo lo que hace que el problema sea "embarazosamente paralelo"
(*embarrassingly parallel*): se puede dividir el arreglo en bloques
independientes sin ningún reordenamiento de datos.

### Justificación de los datos de prueba

- **Tamaño de la muestra**: se probó con `N = 200,000,000` (200 millones)
  de mediciones. Es un tamaño representativo de "miles de millones" a
  escala reducida para que las corridas de prueba tomen segundos y no
  minutos/horas en una laptop, sin perder el comportamiento asintótico del
  algoritmo (el enunciado original habla de miles de millones; el código
  acepta cualquier `N` por parámetro, incluyendo valores de esa magnitud si
  se cuenta con suficiente RAM: `N` mediciones de `float` ocupan `4*N`
  bytes).
- **Origen de los datos**: generados sintéticamente con `rand()` y una
  semilla fija (`42`) para que las corridas sean reproducibles y
  comparables entre la versión secuencial y la paralela (mismos datos,
  mismo resultado esperado).
- **Rango simulado**: temperaturas uniformes entre -50.0°C y 50.0°C,
  divididas en 100 cubetas de 1°C de ancho cada una.
- **Estructura en memoria**: un arreglo contiguo `float datos[N]` (mismo
  layout que tendría el arreglo real del enunciado) y un arreglo de
  contadores `long histograma[100]`. Se usó `float` (no `double`) para
  modelar mediciones de sensores reales, que normalmente no requieren
  doble precisión, y para reducir el uso de memoria a la mitad frente a
  `double` cuando `N` es del orden de miles de millones.

## 2. Estrategia de Paralelización

### Directivas de OpenMP utilizadas

```c
#pragma omp parallel
{
    long local[NUM_CUBETAS] = {0};   // histograma privado por thread

    #pragma omp for schedule(static)
    for (long i = 0; i < n; i++) {
        int b = cubeta_para(datos[i], ancho);
        local[b]++;
    }

    #pragma omp critical
    {
        for (int b = 0; b < NUM_CUBETAS; b++)
            histograma[b] += local[b];
    }
}
```

- **`#pragma omp parallel`**: crea el equipo de threads una sola vez; cada
  thread declara su propio arreglo `local[100]` en la pila, que es
  automáticamente privado (no se comparte entre threads).
- **`#pragma omp for schedule(static)`**: reparte el rango `[0, N)` en
  bloques contiguos de tamaño aproximadamente igual, uno por thread. Se
  eligió `static` (y no `dynamic` o `guided`) porque **el costo de procesar
  cada elemento es idéntico** (una resta, una división, un incremento): no
  hay desbalance de carga posible entre cubetas o posiciones del arreglo,
  así que `static` minimiza el overhead de scheduling (no hay que pedir
  trabajo en cada iteración, como sí ocurre con `dynamic`).
- **`#pragma omp critical`**: cada thread combina su histograma local en el
  histograma global exactamente una vez (al terminar su bloque). Es una
  sección crítica muy corta (100 sumas), ejecutada `N_threads` veces en
  total, así que su costo es insignificante comparado con las `N`
  iteraciones del ciclo principal.

### Cómo se evitaron las condiciones de carrera

La condición de carrera obvia sería que todos los threads incrementen
directamente `histograma[b]` sobre el arreglo global: dos threads podrían
leer el mismo contador, sumarle 1 cada uno, y escribir el mismo valor
final, perdiendo un conteo (*race condition* clásica de lectura-suma-escritura
no atómica).

Se evitó con el patrón **"histograma local + reducción manual al final"**:

1. Cada thread solo escribe en su copia privada `local[100]`, así que no
   hay nada que sincronizar durante el ciclo principal (que es la parte
   costosa, con `N` iteraciones).
2. Solo al final, cada thread suma su copia privada al histograma global
   dentro de un `critical` — aquí sí hay sincronización, pero es sobre 100
   sumas por thread, no sobre `N` incrementos.

Esta estrategia también evita **false sharing**: si en cambio hubiéramos
usado un histograma global de tamaño `N_threads x 100` con cada thread
escribiendo en su propia fila, filas contiguas en memoria podrían caer en
la misma línea de caché y degradar el rendimiento por invalidaciones de
caché constantes. Al usar un arreglo `local[100]` como variable de pila
(no compartida), cada thread tiene su propia región de memoria
completamente aislada, sin ese problema.

### Balanceo de carga (scheduling)

Como se explicó arriba, `schedule(static)` es la elección correcta aquí
porque **todas las iteraciones cuestan lo mismo**. No aplica ningún
mecanismo de balanceo dinámico, ya que agregaría overhead sin ningún
beneficio (a diferencia, por ejemplo, del Problema 4 de grafos, donde el
trabajo por nodo es muy desigual y sí justificaría `dynamic`/`guided` o una
cola de tareas).

## 3. Resultados y Métricas

### Entorno de prueba (corrida base, generada automáticamente)

- CPU: 10 núcleos lógicos (`sysctl -n hw.ncpu` = 10)
- Compilador: GCC 15.1.0 (Homebrew), flags `-O2 -fopenmp`
- `N = 200,000,000` mediciones
- Verificación de exactitud: en todas las corridas (1, 2, 4, 8 y 10
  threads) el histograma resultante fue **idéntico** al secuencial —
  mismo total (`200,000,000`) y mismo conteo por cubeta. Esto confirma
  que no hubo condiciones de carrera. Ver `docs/corridas/salida.txt` para
  la salida completa de cada ejecución.

### Tabla de Speedup y Eficiencia (corrida de referencia)

| Threads | Tiempo (s) | Speedup (T_secuencial / T_paralelo) | Eficiencia (Speedup / Threads) |
|--------:|-----------:|-------------------------------------:|--------------------------------:|
| 1       | 0.152525   | 1.042                                 | 1.042                           |
| 2       | 0.080602   | 1.971                                 | 0.985                           |
| 4       | 0.045668   | 3.479                                 | 0.870                           |
| 8       | 0.027657   | 5.744                                 | 0.718                           |
| 10      | 0.026213   | 6.060                                 | 0.606                           |

Tiempo secuencial de referencia: **0.158861 s** (N=200,000,000).

### Análisis

- El **speedup crece de forma casi lineal hasta 4 threads** (2 threads →
  ~2x, 4 threads → ~3.5x), lo cual es esperable para un algoritmo
  embarazosamente paralelo con overhead de sincronización mínimo.
- A partir de 8-10 threads la eficiencia empieza a bajar (de 0.87 a 0.61)
  porque el problema está limitado por **ancho de banda de memoria**
  (*memory-bound*, no *compute-bound*): cada iteración hace muy poco
  cómputo (una resta, una división, un incremento) comparado con el costo
  de traer `datos[i]` desde memoria RAM. Al agregar más threads, todos
  compiten por el mismo bus de memoria, y llega un punto en que agregar
  núcleos ya no acelera la lectura del arreglo.
- Esto confirma la premisa del enunciado: la paralelización no es
  "usar más threads" a ciegas, sino entender que este problema en
  particular tiene un techo de rendimiento fijado por el hardware de
  memoria, no por el número de núcleos disponibles.

### Mediciones individuales por integrante (requisito del examen)

> **Cada integrante debe correr ambos programas en su propia máquina** y
> agregar aquí su tabla de speedup/eficiencia junto con evidencia
> (screenshot o video de la ejecución), para demostrar participación
> equitativa.

#### Integrante 1 — Teviet

- Máquina / CPU: (completar)
- Tiempo secuencial (N = ___): (completar)

| Threads | Tiempo (s) | Speedup | Eficiencia |
|--------:|-----------:|--------:|-----------:|
|         |            |         |            |

- Evidencia (screenshot/video): (adjuntar en `docs/corridas/`)

#### Integrante 2 — [Nombre de tu compañera] <!-- TODO -->

- Máquina / CPU: (completar)
- Tiempo secuencial (N = ___): (completar)

| Threads | Tiempo (s) | Speedup | Eficiencia |
|--------:|-----------:|--------:|-----------:|
|         |            |         |            |

- Evidencia (screenshot/video): (adjuntar en `docs/corridas/`)
