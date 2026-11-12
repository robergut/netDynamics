# Resultados del artículo

Resultados finales usados en las figuras y tablas del artículo (ver
[ARTICULO.md](../ARTICULO.md)). A diferencia de `results/`, esta carpeta se versiona.

Cada experimento va en su propia carpeta con todo lo necesario para repetirlo:

```
paper/
  fig3_k_sweep/
    config_used.txt     # configuración exacta, incluida la semilla
    attractors.txt
    basins.txt
    phase-hamming.txt
```

Para generar un experimento directamente aquí:

```
./netDynamics config --outDir=paper/fig3_k_sweep --file=paper/fig3_k_sweep/salida --seed=42
```

Notas:

- Anota en el mensaje del commit, o en un README de la carpeta, qué figura o tabla sale de
  cada experimento. El commit también fija la versión del código con la que se generó.
- `salida` y `deg_nodes` (las redes) se pueden regenerar con la semilla de
  `config_used.txt`; no hace falta versionarlas salvo que se quiera conservar
  exactamente las redes analizadas.
- GitHub rechaza archivos de más de 100 MB. `phase-hamming.txt` puede crecer mucho
  (una línea por estado inicial y por red): comprímelo o súbelo a un repositorio de
  datos como Zenodo, que además da un DOI para citarlo.
