# Plan para un artículo a partir de la tesis

Plan para reproducir los escenarios del capítulo 5 de la tesis con el código actual y
extenderlos para un artículo. Las referencias de página son las del PDF de la tesis
(UNAM, 2013), capítulos 4 y 5 y apéndice A.

Documentos relacionados: [CAMBIOS.md](CAMBIOS.md) (correcciones y optimización),
[PENDIENTES.md](PENDIENTES.md) (decisiones abiertas del modelo) y
[MEJORAS.md](MEJORAS.md) (opciones de rendimiento).

## 1. Escenarios de la tesis

| Experimento | Redes | Tratamientos | Medidas |
|---|---|---|---|
| Propiedades estadísticas (§5.1) | N = 15 + 5 y N = 80 + 20, K = {1, 2, 3, 5}, 1000 redes | Simple, Red (φ = ι = ω = 1), FD, ID, OD | A, L̄, %SIA |
| Sensibilidad a condiciones iniciales (§5.2) | N = 20 con 1000 redes; N = 100 y N = 800 con 100 redes | Los mismos | ΔH |
| Redundancia más degeneración (§5.2.1) | N = 15 + 5 + 5 con 1000 redes; N = 720 y N = 880 con 100 redes | Un nodo duplicado dos veces: una copia redundante y otra degenerada | ΔH |
| Medidas de complejidad (§5.3) | N = 20 + 5, K = 5 | Los mismos | Emergencia, autoorganización, complejidad, homeostasis |

Parámetros comunes: p = 0.5, topología homogénea, actualización síncrona,
t_max = 10,000 pasos para ΔH, y los resultados se calculan sobre los nodos del núcleo
(p. 42).

Nomenclatura de los tratamientos (p. 42-43): **Simple** es la red sin nodos agregados,
**Red** tiene nodos redundantes, y **FD**, **ID** y **OD** tienen nodos con
degeneración de función, de entradas y de salidas. En el apéndice A, cada tratamiento
aparece como una combinación (φ, ι, ω), y un coeficiente en 0 indica el tipo de
degeneración aplicado:

| (φ, ι, ω) | Tratamiento |
|---|---|
| (0, 1, 1) | FD |
| (1, 0, 1) | ID |
| (1, 1, 0) | OD |
| (1, 1, 1) | Red |
| (0, 0, 0) | Las tres degeneraciones a la vez; no hay una fila explícita para Simple (por verificar) |

## 2. Estado del código

### Listo

- Generación de redes con los cinco tratamientos: `fd`, `id` y `od` en `config`, y
  `addNode` corregido según la §4.1 (OR entre X y R en los receptores).
- A, número de estados en atractores y tamaño de cada cuenca: `attractors.txt` y
  `basins.txt`. De ahí, L̄ = estados en atractores / A y %SIA = estados / 2^N.
- ΔH para N = 20, exacto, con búsqueda exhaustiva (`exhaustiveMaxN: 24`): unos 2
  minutos para 1000 redes.
- Semillas registradas en los archivos de salida.

### Falta implementar

1. **Redes de más de 64 nodos en el motor rápido.** N = 80, 100, 720, 800 y 880 usan
   hoy la implementación lenta. Hay que extender `FastNet` a un estado de varias
   palabras de 64 bits. Con N grande se muestrea; no hay búsqueda exhaustiva.
2. **Redundancia más degeneración (§5.2.1).** Hoy cada nodo nuevo duplica un nodo
   elegido al azar. Falta una opción para duplicar el mismo nodo dos veces: una copia
   redundante (φ = ι = ω = 1) y otra con un tipo de degeneración.
3. **Medidas de complejidad (§5.3).** No están en el código C: `getInfReport` y
   `getShannonInf` (`experiments.c`) están incompletas, y `getShannonInf` suma
   `log2` del valor en lugar de usar probabilidades. Hay que implementar información
   de Shannon, emergencia, autoorganización, complejidad y homeostasis según [GF12].
4. **Script de experimentos.** Debe recorrer la matriz N × K × tratamiento ×
   repeticiones con semillas registradas y escribir un CSV con una fila por red, con
   los parámetros, la semilla, A, L̄, %SIA, ΔH y las medidas de complejidad.
5. **Análisis en R.** Debe generar las figuras 5.1 a 5.14 y las tablas A.1 a A.4 a
   partir del CSV.

### Decisiones pendientes que afectan los resultados

Están descritas en [PENDIENTES.md](PENDIENTES.md):

- **Punto 1:** cómo se comporta un nodo que recibe de R sin recibir de X. La tesis
  dice "doubled, as a canalizing function" (p. 38), y eso admite varias lecturas.
- **Punto 2:** sobre qué nodos se mide ΔH. La p. 42 dice que los resultados no
  consideran los nodos agregados, pero la p. 50 define H_i = 1/N con N = 20. Hoy
  `get_phase` mide sobre todos los nodos.
- **Punto 3:** redes modulares (`m > 1`) con nodos redundantes. No afecta los
  escenarios de la tesis, que usan m = 1.

## 3. Replicación antes de escribir

El capítulo 5 (p. 41) dice que los experimentos se hicieron con la extensión de
**RBNLab en Java**, no con este código C. En el código C se encontraron errores que
cambian los resultados (ver [CAMBIOS.md](CAMBIOS.md)):

- `addNode` trataba al nodo nuevo como el bit más significativo de la regla; con eso
  los receptores dejaban de leer una de sus entradas originales.
- `fd` negaba la función en lugar de reasignar valores aleatorios.
- La búsqueda de atractores inflaba los conteos en redes caóticas.

Si la versión Java tenía los mismos errores, los números de la tesis no se pueden usar
tal cual. Si no los tenía, los resultados con el código corregido deberían parecerse a
los del apéndice A.

**Primer experimento:** correr los escenarios de las tablas A.1 a A.4 y comparar. Si
hay diferencias, explicarlas antes de escribir. El código Java original sería la mejor
referencia.

### Valores de referencia (apéndice A, N_core = 15, N_deg = 5, 1000 redes)

**K = 1 (tabla A.1)**

| φ | ι | ω | Ā | L̄_e | %SIA | σ_A | σ_Le | σ_%SIA |
|---|---|---|---|---|---|---|---|---|
| 0 | 0 | 0 | 1.667 | 2.893 | 2.76E-4 | 1.446 | 5.579 | 5.32E-4 |
| 0 | 1 | 1 | 1.547 | 2.605 | 2.48E-4 | 1.249 | 4.896 | 4.67E-4 |
| 1 | 0 | 1 | 1.613 | 2.770 | 2.64E-4 | 1.922 | 8.066 | 7.69E-4 |
| 1 | 1 | 0 | 1.515 | 2.446 | 2.33E-4 | 1.148 | 4.380 | 4.18E-4 |
| 1 | 1 | 1 | 1.717 | 3.131 | 2.99E-4 | 2.563 | 10.577 | 10.09E-4 |

**K = 2 (tabla A.2)**

| φ | ι | ω | Ā | L̄_e | %SIA | σ_A | σ_Le | σ_%SIA |
|---|---|---|---|---|---|---|---|---|
| 0 | 0 | 0 | 3.357 | 11.741 | 11.2E-4 | 2.963 | 15.321 | 1.46E-3 |
| 0 | 1 | 1 | 3.092 | 10.183 | 9.71E-4 | 3.079 | 14.961 | 1.43E-3 |
| 1 | 0 | 1 | 3.064 | 9.095 | 8.67E-4 | 2.843 | 10.970 | 1.05E-3 |
| 1 | 1 | 0 | 3.823 | 8.733 | 8.33E-4 | 2.347 | 12.313 | 1.17E-3 |
| 1 | 1 | 1 | 3.317 | 11.085 | 10.6E-4 | 3.216 | 17.037 | 1.62E-3 |

**K = 3 (tabla A.3)**

| φ | ι | ω | Ā | L̄_e | %SIA | σ_A | σ_Le | σ_%SIA |
|---|---|---|---|---|---|---|---|---|
| 0 | 0 | 0 | 4.698 | 27.830 | 2.65E-3 | 3.402 | 25.171 | 2.40E-3 |
| 0 | 1 | 1 | 4.078 | 17.981 | 1.71E-3 | 3.055 | 14.663 | 1.40E-3 |
| 1 | 0 | 1 | 3.942 | 18.382 | 1.75E-3 | 2.721 | 14.398 | 1.37E-3 |
| 1 | 1 | 0 | 3.808 | 18.328 | 1.75E-3 | 2.413 | 14.396 | 1.37E-3 |
| 1 | 1 | 1 | 4.173 | 20.045 | 1.91E-3 | 2.876 | 15.123 | 1.44E-3 |

**K = 5 (tabla A.4)**

| φ | ι | ω | Ā | L̄_e | %SIA | σ_A | σ_Le | σ_%SIA |
|---|---|---|---|---|---|---|---|---|
| 0 | 0 | 0 | 5 | 188.489 | 17.9E-3 | 2.120 | 135.635 | 12.93E-3 |
| 0 | 1 | 1 | 4 | 72.592 | 6.92E-3 | 2.145 | 50.513 | 4.82E-3 |
| 1 | 0 | 1 | 4 | 67.793 | 6.47E-3 | 2.058 | 43.555 | 4.15E-3 |
| 1 | 1 | 0 | 4 | 87.519 | 8.35E-3 | 2.022 | 55.231 | 5.27E-3 |
| 1 | 1 | 1 | 4 | 85.129 | 8.12E-3 | 2.018 | 50.447 | 4.81E-3 |
| 1 | 1 | 1 | 4 | 85.129 | 6.87E-3 | 2.496 | 47.294 | 4.51E-3 |

### Inconsistencias en el apéndice A por revisar

- La tabla A.4 tiene dos filas (1, 1, 1) con el mismo Ā y L̄_e, pero distinto %SIA y
  distintas desviaciones. Puede que una de ellas sea el tratamiento Simple.
- No hay una fila explícita para Simple. Hay que confirmar si (0, 0, 0) es Simple o
  "las tres degeneraciones a la vez".
- La normalización de %SIA no coincide de forma directa con la ecuación 5.1
  (%SIA = 100 · A · L_e / 2^N). Por ejemplo, con A = 4 y L̄_e = 85.129, da 0.0325 con
  N = 20 y 1.04 con N = 15, y la tabla dice 8.12E-3. Puede deberse a que se promedió el
  cociente de cada red, a que el resultado es una fracción y no un porcentaje, o a que
  se dividió entre otro N. Hay que revisarlo con los datos o el código original.
- La §5.2 (p. 49) dice que se analizaron 2^15 = 32,768 estados, "el número total de
  estados" de redes con N = 20, que tienen 2^20.

## 4. Posibles aportes del artículo respecto a la tesis

- **Resultados exactos para N = 20.** Recorrer los 2^20 estados: atractores completos,
  cuencas exactas y ΔH promediado sobre todo el espacio de estados, en lugar de 32,768
  muestras (ver [MEJORAS.md](MEJORAS.md), punto 1).
- **Barridos de φ, ι y ω entre 0 y 1.** La tesis dice que los efectos con valores
  intermedios "no fueron estadísticamente significativos" (p. 42). Con valores exactos
  y 10,000 redes o más se puede ver si hay una tendencia o si de verdad no hay efecto.
  Cualquiera de los dos resultados es publicable.
- **Pruebas estadísticas formales.** Kruskal-Wallis o Mann-Whitney, con tamaños de
  efecto e intervalos de confianza, en lugar de solo diagramas de caja.
- **Aditividad de redundancia y degeneración.** La tesis la presenta como resultado
  preliminar (p. 62).
- **Reproducibilidad.** Publicar el código, las semillas y los scripts de análisis con
  el artículo.

## 5. Pasos

| # | Paso | Depende de |
|---|---|---|
| 1 | Revisar este plan con el tutor | — |
| 2 | Resolver los puntos 1 y 2 de [PENDIENTES.md](PENDIENTES.md) | 1 |
| 3 | Extender el motor rápido a más de 64 nodos | — |
| 4 | Agregar la opción de redundancia más degeneración sobre el mismo nodo | 2 |
| 5 | Implementar las medidas de complejidad de la §5.3 | — |
| 6 | Escribir el script de experimentos y el análisis en R | 3, 4, 5 |
| 7 | Replicar las tablas A.1 a A.4 y comparar | 2, 6 |
| 8 | Explicar las diferencias con la tesis, si las hay | 7 |
| 9 | Correr los experimentos nuevos (exactos, barridos y aditividad) | 7 |
| 10 | Revisar la literatura sobre degeneración en redes booleanas posterior a 2013 | — |
| 11 | Escribir el artículo | 8, 9, 10 |

Los pasos 3, 5 y 10 se pueden hacer en paralelo con los demás.

## 6. Preguntas para el tutor

- ¿Se conserva el código Java de RBNLab usado en la tesis, o los datos crudos de los
  experimentos?
- ¿ΔH debe medirse solo sobre el núcleo o sobre todos los nodos?
- ¿Qué significa "doubled, as a canalizing function" para los nodos que reciben del
  nodo redundante sin recibir del original?
- ¿Qué revista o congreso sería el destino? (por ejemplo, *Artificial Life*, *Complexity*,
  o un congreso ALIFE).
- ¿El artículo debe ser una replicación con extensiones o centrarse solo en los
  resultados nuevos?
