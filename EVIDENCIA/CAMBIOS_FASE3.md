# Cambios técnicos de fase 3

- Cola y pila propias: `Cola.h`, `PilaAcciones.h`, integradas al grafo y a las acciones de cada usuario.
- BFS de distancia mínima y peso máximo entre rutas de igual distancia: `Grafo.h/.cpp`; tabla hash de vértices y de visitados.
- Motor: mantiene pesos 35/25/30/10, añade evidencia indirecta con penalización 0.5 y muestra aportes por criterio.
- Matriz dispersa: elimina celdas vacías, permite restaurar estados, valida desbordamiento de visualizaciones.
- Sistema/árbol/listas: validación de rutas y textos, límite de 1000 elementos, rollback de modificaciones si falla guardar.
- Persistencia: staging de cuatro archivos, recuperación de respaldo y validación más estricta al cargar. No es una base de datos ACID.
- Menú: opción 15 deshacer, opción 16 explorar por BFS, controles de rol conservados.
- Proyectos y filtros de Visual Studio: nuevos encabezados incluidos y entradas inexistentes retiradas.
- Pruebas: comprobaciones activas aun con NDEBUG; casos normales, límite, errores y medición reproducible.

## Evidencia de verificación

Linux, GCC 14.2.0, C++17: compilación de aplicación y pruebas Release con `-O2 -DNDEBUG -Wall -Wextra -Wpedantic -Werror`. Pruebas adicionales con `-fsanitize=address,undefined`. Ver `resultados.txt` y `rendimiento.csv`.

La medición usa una cadena sintética, no representa un grafo denso ni mide I/O o reconstrucción completa. No se ejecutó MSVC/Visual Studio en este entorno. Validar en Windows antes de la entrega y conservar el historial de Git del equipo al integrar los archivos. Esta nota técnica no sustituye el documento final, la bitácora histórica ni la defensa.
