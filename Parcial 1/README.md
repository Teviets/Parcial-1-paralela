# [NOMBRE CREATIVO DE LA CONSULTORA HPC] <!-- TODO: elegir nombre con tu compañera -->

Consultora de optimización de software — Examen Parcial 1: Consultoría HPC usando OpenMP.

## Integrantes

- Teviet (teviet@gmail.com)
- [Nombre completo de tu compañera] <!-- TODO -->

## Problema resuelto

**Problema 1: El Histograma Masivo** — clasificación de mediciones de temperatura en 100 cubetas usando OpenMP.

## Estructura del repositorio

```
/secuencial   -> algoritmo base (sin paralelizar)
/paralelo     -> solución optimizada con OpenMP
/docs         -> documentación, resultados y corridas de prueba
```

## Cómo compilar y correr

Requiere un compilador con soporte OpenMP (en macOS, `gcc` de Homebrew: `brew install gcc`).

```bash
# Secuencial
gcc -O2 -o secuencial/histograma_secuencial secuencial/histograma_secuencial.c
./secuencial/histograma_secuencial 200000000

# Paralelo (segundo argumento = número de threads, opcional)
gcc -O2 -fopenmp -o paralelo/histograma_paralelo paralelo/histograma_paralelo.c
./paralelo/histograma_paralelo 200000000 8
```

Ver `docs/documentacion.md` para el análisis completo (contexto, estrategia de paralelización y resultados).
