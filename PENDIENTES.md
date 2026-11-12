# Pendientes

Cambios identificados que **no** se han aplicado al código. Las referencias a la tesis
son al capítulo 4 ("Degeneracy in Random Boolean Networks") y al capítulo 5
("Numerical Simulations").

## 1. Nodos que reciben del nodo redundante R sin recibir del nodo duplicado X

**Dónde:** `doubleRule` en `classicrbn.c:352`, llamada desde `addNode` (`classicrbn.c:380`).

**Contexto:** con output degeneracy (`od < 1`), algunas salidas de R se reasignan al
azar a nodos que no tenían a X como entrada. Esos nodos ganan una entrada y su tabla
de búsqueda pasa de `2^K` a `2^(K+1)` entradas.

**Qué hace el código hoy:** duplica la tabla de forma que el valor de R se ignora
(`weight = 0` en `doubleRule`). Esos nodos quedan conectados a R, pero R no influye en
su siguiente estado.

**Qué dice la tesis:** la sección 4.1 solo define el caso en que el receptor tiene a X
y a R como entradas (`f_new(…, X, R, …) = f_old(…, X ∨ R, …)`), que ya está
implementado. Para un receptor que solo tiene a R, la tesis (p. 38) dice:

> In the case that a node gets a controlling element from σ_ired, its coupling
> function is doubled, as a canalizing function.

Esto puede leerse de varias formas:

- **a) Duplicar la tabla e ignorar R** (lo actual). R no tiene efecto en esos nodos.
- **b) R canalizante:** cuando R toma cierto valor (por ejemplo, 1), la salida del
  receptor queda fija; cuando toma el otro, se usa `f_old`.
- **c) Mitad nueva aleatoria:** la mitad de la tabla que corresponde a R=1 se genera
  al azar con probabilidad `p`, igual que en `initRules`.

**Pendiente:** revisar cómo lo implementaba la extensión de RBNLab en Java que se usó
para los experimentos de la tesis y replicarlo en `doubleRule`.

## 2. `get_phase` mide también los nodos redundantes

**Dónde:** `main.c:87`.

```c
reportPhase(out, conf->iterations, conf->maxStates, conf->nm + conf->redNodes, net);
```

**Contexto:** `reportPhase` calcula la distancia de Hamming (`experiments.c:132-133`)
sobre los primeros `n` nodos, y hoy recibe `nm + redNodes`. Por eso cuenta también los
nodos redundantes. `get_attractors` (`main.c:82`) sí compara solo los nodos originales,
porque recibe `conf->nm`.

**Qué dice la tesis (capítulo 5, p. 42):**

> It should be noted that results presented are based on the dynamics of the RBN core
> ensembles, i.e. results do not consider the controlling elements of Ndeg.

**Cambio propuesto:** pasar `conf->nm` en lugar de `conf->nm + conf->redNodes`. La
perturbación inicial ya se hace sobre el nodo 0, que siempre pertenece al núcleo.

## 3. Redes modulares (`m > 1`) con nodos redundantes

**Dónde:** `main.c:38-67` y el final de `addNode` (`classicrbn.c:483`).

**Síntoma:** con `m > 1` y `redNodes > 0` el programa falla con accesos fuera de los
arreglos. AddressSanitizer reporta `heap-buffer-overflow` en `next`, o
`heap-use-after-free` con el código original. Con `m = 1`, que es la configuración que
se usó en la tesis, no ocurre.

**Causa:** `main` y `addNode` no coinciden en el tamaño de la red:

- `initModular` (`main.c:38`) reserva, y `appendRBN` escribe, redes de
  `(nm + redNodes) * m` nodos; es decir, nodos redundantes **por módulo**.
- Antes de agregar los nodos redundantes, `main.c:64` hace `n = nm * m`, y `addNode`
  se llama `redNodes` veces, lo que agrega nodos redundantes **en total**:
  `n = nm * m + redNodes`.
- `addNode` termina con `net->n = net->nm = net->n + 1`, lo que sobrescribe `nm` con
  el tamaño total y borra la estructura de módulos.
- Las siguientes llamadas a `readRBN` usan un `n` que ya no coincide con el tamaño de
  las redes guardadas en el archivo, y los índices se salen de los arreglos.

**Pendiente:** definir si los nodos redundantes se agregan por módulo o en total, y
hacer que la reserva de memoria, `readRBN`/`appendRBN` y `addNode` usen el mismo
tamaño. `addNode` no debería modificar `nm`.
