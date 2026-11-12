# Cambios aplicados

Correcciones hechas sobre el commit `f039117` ("Adding base code for RBN statistics").
Los cambios que faltan están en [PENDIENTES.md](PENDIENTES.md). Las referencias a la
tesis son al capítulo 4 ("Degeneracy in Random Boolean Networks").

Cada error se confirmó antes de corregirlo, compilando con AddressSanitizer y
UndefinedBehaviorSanitizer (`clang -fsanitize=address,undefined`) o con una prueba
específica. Las mismas pruebas se repitieron después de la corrección.

## 1. Consumo de memoria sin límite en la búsqueda de atractores

**Archivos:** `experiments.c`, `experiments.h`.

Esta era la causa más probable de los fallos de memoria durante la ejecución.

### Problema

`getAttractors` daba `MARGIN` (100) pasos después del transitorio. En cada paso, si el
estado no estaba en ningún atractor guardado, creaba uno nuevo con hasta
`MAX_ATTRACTOR + 1` (3001) estados.

En el régimen caótico (por ejemplo, K=5 y p=0.5) muchos ciclos miden más de 3001
estados. En ese caso el atractor guardado era solo un fragmento del ciclo, el siguiente
paso tampoco lo encontraba, y se creaba **otro** atractor de 3001 estados. Esto podía
repetirse hasta 100 veces por estado inicial.

- Con N=40 y solo 5 estados iniciales se crearon 63 "atractores" con 186,966 estados.
  Con `maxStates: 32768` esto llega a decenas o cientos de GB.
- `reportAttractors` solo liberaba la `List` (`free(attractors)`). Los `Attractor`, los
  `Chain` y los estados nunca se liberaban, así que la memoria se acumulaba de una red
  a la siguiente.
- En redes caóticas, `attractors.txt` contaba fragmentos repetidos de un mismo ciclo
  como atractores distintos.

### Corrección

- `getAttractors` (`experiments.c:224`) detecta el ciclo con el **algoritmo de Brent**,
  comparando el estado completo de la red. Necesita un solo búfer adicional y obtiene
  el periodo exacto.
- Cada atractor se guarda una sola vez, con su periodo real. Si el periodo pasa de
  `MAX_ATTRACTOR`, o no se encuentra ciclo después de `MAX_ATTRACTOR + MARGIN` pasos,
  el estado inicial se cuenta como "sin resolver", se informa en consola y no se
  guarda nada.
- Se mantiene la comparación de estados con `n` nodos, para que `get_attractors` siga
  considerando solo los nodos originales (`conf->nm`).
- Funciones nuevas `freeAttractor` (`experiments.c:21`) y `freeAttractors`
  (`experiments.c:34`). `reportAttractors` libera toda la memoria de cada red
  (`experiments.c:193`).
- El formato de `attractors.txt` no cambió.

### Resultado

Con N=40 caótica y 200 estados iniciales: 1 atractor real por red y 2 MB de memoria.

## 2. Desbordamiento de pila en `reportPhase`

**Archivo:** `experiments.c` (`reportPhase`, línea 85).

### Problema

- `j++` se hacía dentro del `printf`, antes de `deltas[j] = ...`. Con `NET_COUNT`
  redes se escribía en `deltas[NET_COUNT]`, fuera del arreglo. ASan lo reportaba como
  `dynamic-stack-buffer-overflow`.
- Si el archivo de redes tenía más redes que `iterations` (se abre con `"a+"`), la
  escritura se salía todavía más.
- Los promedios se dividían entre `MAX_STATES` (4096), no entre el número de estados
  iniciales (`maxStates`). Con `maxStates: 32768` los valores salían 8 veces más grandes.

### Corrección

- Se procesan como máximo `NET_COUNT` redes (`experiments.c:103`), y el índice se
  incrementa después de usarlo (`deltas[j++]`, `experiments.c:143`).
- Los promedios dividen entre `INITIAL_STATES_COUNT`. El promedio global y la
  desviación estándar usan el número de redes que realmente se leyeron.
- Si no se leyó ninguna red, se informa y se sale sin dividir entre cero.

## 3. Crash en la topología acoplada (`topology: 3`, `m > 1`)

**Archivo:** `classicrbn.c`.

### Problema

En `generateInterModuleConnections` el índice de fila se calculaba como
`rand() * net->nm + modCont`. El producto se desbordaba y el índice salía del arreglo
(`BUS` en ASan). La matriz auxiliar `libres` de `generateCoupledConnectionMatrix`
nunca se liberaba.

### Corrección

- `rand() % net->nm + modCont` (`classicrbn.c:315`).
- `libres` se libera al terminar (`classicrbn.c:292`).

## 4. `addNode`: construcción del nodo redundante y de la degeneración

**Archivo:** `classicrbn.c` (`addNode`, línea 380; `doubleRule`, línea 352).

### Cómo se indexan las reglas

En `step()` y `next()`, la columna conectada con el índice más alto es el bit
**menos** significativo del índice de la regla. El nodo redundante R siempre se agrega
en la última columna (`n`), así que en todo nodo que lo recibe es el bit menos
significativo.

### Problemas

1. **Duplicación de tablas en los receptores.** Con `rules[j + aux] = rules[j]` las dos
   mitades de la tabla quedaban iguales, lo que hace irrelevante el bit **más**
   significativo y no el de R. El receptor dejaba de leer una de sus entradas
   originales y pasaba a depender de R. Aun con degeneración total (fd=id=od=1),
   alrededor del 10% de las transiciones de la red cambiaban.
2. **No se seguía la sección 4.1 de la tesis.** La tesis indica que los nodos que
   reciben de X y de R combinan ambas señales con OR:
   `f_new(…, X, R, …) = f_old(…, X ∨ R, …)`. El código pretendía que R se ignorara, y
   con eso ni `od` ni la aleatoriedad de `id`/`fd` podían afectar al resto de la red.
3. **Regla de R tomada después de modificarla.** Si X recibía de R, la tabla de X ya se
   había duplicado cuando se copiaba a R, y R recibía valores revueltos.
4. **Autoconexión.** Si X tenía autoconexión, `i % aux` usaba el bit equivocado para la
   entrada de R hacia sí mismo.
5. **Function degeneracy.** Las entradas elegidas se invertían (`!rule`). La sección 4.3
   indica que se reasignan con valores aleatorios con probabilidad `p`. Con φ=0, la
   inversión daba `NOT f`, una función completamente determinada, en lugar de una
   aleatoria. Además, solo se recorría la primera mitad de la tabla cuando R tenía
   autoconexión.

### Corrección

- La tabla original de X se guarda antes de modificar cualquier regla.
- Función nueva `doubleRule` (`classicrbn.c:352`). Duplica la tabla de un nodo que
  recibe a R:
  - Si el nodo también recibe a X, calcula el peso de X en el índice de la regla y,
    cuando R=1, enciende ese bit. El resultado es `f_old(…, X ∨ R, …)`.
  - Si no recibe a X, R se ignora. Este caso está documentado en
    [PENDIENTES.md](PENDIENTES.md), punto 1.
- R copia la tabla original de X. Si tiene autoconexión, porque X la tenía, su tabla
  también se extiende con `doubleRule`. Así lo hace `addRedNode` en `redrbn`
  (RBNLab, C. Gershenson).
- La function degeneracy reasigna `⌊2^K' · (1 − fd)⌋` entradas, elegidas al azar en
  toda la tabla de R, con valores aleatorios con probabilidad `net->p`
  (`classicrbn.c:478`).
- Input degeneracy y output degeneracy no cambiaron: ya reasignaban `⌊K · (1 − id)⌋`
  entradas y `⌊K_out · (1 − od)⌋` salidas, como en las secciones 4.4 y 4.5.

### Verificación

Prueba sobre 3000 redes aleatorias (N=12, K=3, la mitad con autoconexiones) para cada
combinación de grados. Para cada estado aleatorio, con R también aleatorio:

| id / od / fd | Diferencias en nodos originales | R igual a X | Tabla de R igual a la de X |
|---|---|---|---|
| 1 / 1 / 1 | 0 de 288,000 | 0 diferencias | 100% |
| 1 / 0 / 1 | 0 de 288,000 | 0 diferencias | 100% |
| 0.5 / 0.5 / 0.5 | 0 de 288,000 | — | 75% |
| 0 / 0 / 0 | 0 de 288,000 | — | 50% |
| 1 / 1 / 0 | 0 de 288,000 | — | 50% |

"Diferencias en nodos originales" compara el siguiente estado con lo esperado: un nodo
que recibe de X y de R debe dar `f_old(X ∨ R)`, y cualquier otro debe dar `f_old`.
Con fd=0.5 se reasigna la mitad de la tabla y la mitad de lo reasignado coincide por
azar, lo que da un 75%. Con fd=0 la coincidencia es la del azar, un 50%; antes era 0%.

## 5. Valor de `p` usado al crear los nodos redundantes

**Archivo:** `main.c:39`.

`net->p = conf->p` solo se asignaba cuando se creaba el archivo de redes. Si `salida`
ya existía, la function degeneracy usaba el 0.5 que pone `initModular`. Ahora se
asigna siempre, justo después de `initModular`.

## 6. Liberación de memoria al terminar

**Archivos:** `classicrbn.c`, `classicrbn.h`, `main.c`.

- Función nueva `freeRBN(RBN*, int)` (`classicrbn.c:34`). Libera `conn`, `rules`,
  `links`, `state`, `nextState` y la estructura. Recibe el número de nodos reservados
  por `initModular`, porque al final `net->n` puede ser menor.
- `main` libera `net` y `conf` al terminar (`main.c:90-91`), y también en las salidas
  tempranas, cuando no existe `config` o no se puede abrir el archivo de salida.

Con `leaks --atExit` (macOS) el programa termina con 0 fugas en las configuraciones
probadas: N=15 con y sin 5 nodos redundantes, y topología acoplada con m=2.

## 7. Correcciones menores

- **`readConf` sin archivo `config`** (`rbnio.c:75`): devolvía una estructura sin
  inicializar, y `main` la usaba con basura. Ahora devuelve `NULL`, y `main` termina
  (`main.c:25`).
- **Lectura de cadenas del config** (`rbnio.c:89-90`): `%s` sin límite sobre arreglos
  de 65 bytes. Ahora es `%64s`.
- **Limpieza de la red entre iteraciones** (`main.c:46`): `initM(net, 0, net->nm)` solo
  limpiaba `nm` filas. Con `m > 1` quedaban conexiones de la red anterior, y en la
  topología homogénea se acumulaban. Ahora se limpia toda la red con `net->n`.

## Efecto en los resultados

Los resultados generados con la versión anterior no son comparables con los de esta
versión:

- **Atractores:** en redes caóticas, los conteos anteriores estaban inflados por los
  fragmentos repetidos de ciclos largos. Ahora salen menos atractores, y los ciclos de
  periodo mayor a 3000 se reportan aparte en consola.
- **Fase:** los promedios anteriores eran `maxStates / 4096` veces más grandes, o sea,
  8 veces con la configuración de `config`.
- **Nodos redundantes (`redNodes > 0`):** las redes anteriores tenían nodos que
  alteraban la dinámica original aun con degeneración total, y la function degeneracy
  con `fd = 0` generaba la función negada en lugar de una aleatoria. Los experimentos
  de degeneración tendrían que volver a correrse.
