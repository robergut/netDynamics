# Mejoras posibles

Opciones para hacer el análisis más eficiente o más preciso, todavía sin decidir. La
implementación actual está descrita en [CAMBIOS.md](CAMBIOS.md) (sección
"Optimización") y los errores pendientes están en [PENDIENTES.md](PENDIENTES.md).

## Situación actual

Con la configuración de la tesis (N = 15 + 5 nodos redundantes, K = 5, 32,768 estados
iniciales) cada red tarda 0.12 s con la búsqueda exhaustiva (`exhaustiveMaxN: 24`).
Las 1000 redes del capítulo 5 tardan unos 2 minutos.

A ese tamaño, la implementación ya está cerca del límite: el tiempo que queda se va en
recorrer tablas de 2^N estados en memoria. La conveniencia de cada mejora depende del
tamaño de red y del régimen dinámico.

## 1. Fase exacta sobre todos los estados (N ≤ 26)

**Qué:** hoy `get_phase` promedia `maxStates` estados iniciales al azar. Como
`buildStateSpace` ya etiqueta los 2^N estados con su ciclo, se puede promediar sobre
**todos** los estados iniciales, sin muestreo.

**Costo:** para N = 20 es un millón de consultas a `stateAfter`, unos milisegundos por
red. Es un cambio chico en `phaseBatchExhaustive` y `reportPhaseFast` (`fastexp.c`).

**Beneficio:** el promedio de δ por red sería exacto, sin ruido de muestreo. Con eso
harían falta menos redes para separar los tratamientos. El capítulo 5 de la tesis dice
que, con coeficientes de degeneración entre 0 y 1, las diferencias entre tratamientos
no fueron estadísticamente significativas.

**A decidir:** si `phase-hamming.txt` debe tener una línea por cada estado (2^N líneas
por red) o solo los promedios por red.

## 2. Reducción de la red (N de 30 a unos cientos, régimen ordenado o crítico, K ≤ 2)

**Qué:** en estas redes, la mayoría de los nodos quedan congelados (valor constante
después del transitorio) o son irrelevantes (sus valores no afectan a ningún nodo que
influya en la dinámica).

- **Reducción a nodos relevantes:** quitar los nodos constantes, los que no tienen
  salidas y las entradas que la regla ignora, y repetir hasta que no cambie nada. El
  conjunto de atractores se conserva. En redes críticas, el núcleo que queda suele ser
  pequeño, del orden de √N nodos. Sobre ese núcleo se aplica la búsqueda exhaustiva
  actual.
- **Descomposición en componentes fuertemente conexos:** construir los atractores
  componente por componente, en orden topológico, en lugar de explorar toda la red a
  la vez.

**Beneficio:** permite la búsqueda exhaustiva, con resultados exactos, en redes mucho
más grandes que 26 nodos.

**Limitación:** no ayuda en el régimen caótico (K = 5), donde casi ningún nodo se
congela.

## 3. Régimen caótico con N grande (K = 5): solo factores constantes

En este régimen los periodos crecen exponencialmente con N, del orden de 2^(N/2), y
casi ningún nodo se congela. Ningún algoritmo evita eso, así que muestrear con un
límite de periodo es lo único viable. Las mejoras posibles son de factor constante:

| Opción | Ganancia estimada | Comentario |
|---|---|---|
| Paralelismo en varios núcleos | 8 a 10 veces en un Mac de 8 a 10 núcleos | Ver punto 4 |
| SIMD (NEON o AVX2) | 2 a 4 veces | 128 a 256 trayectorias por instrucción en lugar de las 64 de `fastNextSliced` |
| GPU (CUDA o Metal) | 10 a 50 veces | La tabla de transiciones y la fase son masivamente paralelas; "saltar T pasos" para todos los estados se puede hacer con duplicación de punteros, unas 14 rondas para T = 10,002. Para los tamaños de la tesis no compensa. |

## 4. Paralelismo

`make OPENMP=1` ya paraleliza las redes (atractores) y los estados iniciales (fase),
pero no se pudo probar porque el compilador de macOS no incluye OpenMP. Hay dos formas
de habilitarlo:

- **OpenMP en macOS:** `brew install libomp` y ajustar el `makefile` con
  `-Xpreprocessor -fopenmp`, el include de libomp y `-lomp`. Es el camino más corto.
- **Rust con `rayon`:** ver punto 5.

Los resultados no deberían depender del número de hilos: cada red y cada grupo de 64
estados iniciales usa su propio flujo de números aleatorios (`rngStream`). Habría que
comprobarlo comparando una corrida de un hilo con una de varios.

## 5. Reescribir en Rust

**Velocidad: casi igual.** Rust y C (con clang) compilan con el mismo backend, LLVM, y
para este tipo de código (enteros, bits, arreglos, tablas hash) generan casi las mismas
instrucciones. La diferencia suele ser de ±10%. Las mejoras de la sección
"Optimización" de CAMBIOS.md vinieron del algoritmo, no del lenguaje.

**Ventajas:**

- **Seguridad de memoria.** La mayoría de los errores encontrados (índices fuera de
  rango en `deltas` y `generateInterModuleConnections`, fugas en la búsqueda de
  atractores, el `use-after-free` con `m > 1`) no compilarían, o terminarían con un
  error claro en el punto exacto.
- **Paralelismo con `rayon`.** Cambiar `.iter()` por `.par_iter()`, sin instalar nada
  en macOS, y con garantía del compilador de que no hay condiciones de carrera.
- **Herramientas.** Pruebas integradas (`cargo test`), bibliotecas probadas para
  números aleatorios y tablas hash, benchmarks con `criterion`.

**Costos:**

- Reescribir unas 2,000 líneas y volver a verificar la equivalencia con la versión en C.
- Cuidar que el modelo siga siendo el mismo: el orden de los bits en las reglas, el OR
  de la sección 4.1 de la tesis y los tres tipos de degeneración.
- Pérdida de continuidad con el código de la tesis.

**Paso intermedio:** portar solo el motor rápido (`fastrbn` y `fastexp`) a Rust y
llamarlo desde el C actual.

**Recomendación:** si solo se busca velocidad, no vale la pena (OpenMP da la misma
ganancia). Conviene si el proyecto va a seguir creciendo: nuevos tipos de degeneración,
más experimentos o redes más grandes.

## 6. Validación con herramientas externas

- **BoolNet** (paquete de R): búsqueda exhaustiva de atractores síncronos, implementada
  en C. Como los resultados ya se analizan en R, sirve para comparar los atractores y
  cuencas de algunas redes exportadas desde `salida` o `deg_nodes`.
- **BNS** (Dubrova y Teslenko): búsqueda de atractores síncronos con SAT, para redes de
  cientos de nodos.

## Resumen

| Objetivo | Opción | Esfuerzo |
|---|---|---|
| Mejores resultados para la tesis (N = 20) | 1. Fase exacta sobre todos los estados | Bajo |
| Redes más grandes con K ≤ 2 | 2. Reducción a nodos relevantes y búsqueda exhaustiva | Medio |
| Más velocidad en general | 4. OpenMP con libomp | Bajo |
| Más velocidad, seguridad y pruebas | 5. Rust con `rayon` | Alto |
| Más velocidad en redes caóticas grandes | 3. SIMD o GPU | Medio a alto |
| Confianza en los resultados | 6. Comparar contra BoolNet | Bajo |
