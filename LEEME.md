# Motor de Recomendacion de Contenido (C++17)

**Proyecto universitario de consola para Visual Studio 2022.** Incluye las estructuras de datos de las fases anteriores, una cola FIFO y una pila LIFO implementadas manualmente, un catalogo inicial, sesiones para usuarios normales y administrador, recomendaciones deterministas y persistencia local.

## Como abrirlo en Visual Studio

1. Instala Visual Studio 2022 con la carga de trabajo **Desarrollo para el escritorio con C++** (MSVC v143 y Windows SDK).
2. Abre `MotorRecomendacion.sln` desde Visual Studio (no confundir con Visual Studio Code).
3. En el Explorador de soluciones, haz clic derecho sobre **MotorRecomendacion** -> **Establecer como proyecto de inicio**.
4. Selecciona `Debug` y `x64`, y presiona **Ctrl+F5** para ejecutar o **F5** para depurar.
5. La carpeta de trabajo de la aplicacion esta configurada a `MotorRecomendacion/`. Los cuatro archivos de datos se guardan en `MotorRecomendacion/datos/`. Si se eliminan **los cuatro**, el sistema vuelve a crear el catalogo inicial automaticamente. No elimines solo uno, porque el programa no sobrescribe conjuntos incompletos. Guarda cualquier respaldo personal fuera de `datos/`.
6. Para ejecutar las pruebas, establece **Pruebas** como proyecto de inicio y ejecutalo. Restablece despues **MotorRecomendacion** como proyecto de inicio.

En caso de incompatibilidad con tu version de Visual Studio, revisa en las propiedades del proyecto la plataforma de herramientas (`v143` para Visual Studio 2022).

## Cuentas iniciales

| Rol | ID | Clave |
| --- | --- | --- |
| Administrador | `1` | `admin123` |
| Usuario normal de demostracion | `2` | `demo123` |
| Usuario normal Alex | `3` | `demo123` |

Las claves se almacenan **en texto plano solo con fines academicos**. No uses credenciales personales ni despliegues este ejemplo como un servicio real.

## Que incluye

- **TablaHash<T>**: encadenamiento separado mediante nodos, funcion modulo y redimensionamiento manual.
- **Grafo**: lista de vertices y listas enlazadas de adyacencia; conexiones no dirigidas con peso 0 a 1.
- **ArbolCategorias**: arbol N-ario por representacion *primer hijo / siguiente hermano*; jerarquias con `/`.
- **MatrizDispersa**: listas ortogonales enlazadas simultaneamente por usuario y contenido; solo guarda celdas con interacciones.
- **HeapMaximo**: arreglo dinamico con `subir` y `bajar`; obtiene candidatos en orden de afinidad descendente, con desempate por ID ascendente.
- **Cola<T>**: lista enlazada FIFO con punteros de frente y fin, utilizada por BFS.
- **PilaAcciones<T>**: lista enlazada LIFO para deshacer visualizaciones, notas y favoritos por usuario.
- **Sistema**, **MotorRecomendacion**, **GestorPersistencia**: coordinacion, recomendaciones y almacenamiento en archivos.

Los punteros y arreglos para las estructuras son propios. Se utiliza `std::string` para texto, `std::filesystem` y `fstream` para archivos, `std::unique_ptr<Candidato[]>` para una matriz temporal de resultados de interfaz; ninguno sustituye las estructuras implementadas manualmente.

## Catalogo inicial

Se incluyen **19 contenidos** educativos y de entretenimiento (programacion, videojuegos, ciencia ficcion, arte, musica, fisica y mas). Hay ejemplos de interacciones previas de los usuarios 2 y 3. Puedes agregar, editar y eliminar contenidos como administrador. Esas acciones reconstruyen automaticamente el arbol de categorias y las conexiones del grafo; al eliminar un contenido tambien se retiran sus interacciones.

## Funcion de recomendacion

El motor no es inteligencia artificial: usa una regla determinista sobre datos existentes.

```text
Afinidad = 0.35 * P + 0.25 * H + 0.30 * G + 0.10 * Popularidad
```

- `P`: proporcion de preferencias explicitas del usuario que coinciden, sin distinguir mayusculas ASCII, con titulo, ruta de categoria o etiquetas del contenido. Si no hay preferencias es 0.
- `H`: promedio ponderado de satisfaccion en contenidos anteriormente registrados de la misma categoria (peso `1.0`) o familia principal (peso `0.4`), con 0 si no hay historial relevante.
- `G`: promedio ponderado por los productos de pesos de las rutas BFS de hasta dos saltos. La satisfaccion de relaciones de dos saltos se multiplica por 0.5 antes del promedio; las relaciones directas conservan el comportamiento anterior. Si no hay relaciones relevantes, G = 0.
- `Popularidad`: atributo del contenido dividido entre 100.
- Satisfaccion de una calificacion 1 a 5 = `(calificacion - 1) / 4`. Sin nota, una visualizacion equivale a `0.5`; marcar favorito eleva su satisfaccion como minimo a `0.9`.

Los cuatro componentes permanecen en `[0,1]`, y los pesos suman `1.0`, por lo que la puntuacion final esta en `[0,1]`. El heap ordena por puntuacion descendente y, en caso de empate, por ID ascendente. **Se excluye cualquier contenido con interaccion efectiva registrada**, incluso si solo esta marcado como favorito. Al quitar el unico favorito, la celda vacia se elimina y el contenido vuelve a ser elegible. El menu muestra los aportes de P, H, G y popularidad en puntos porcentuales.

Ejemplo numerico: `P=0.50`, `H=1.00`, `G=1.00`, `Popularidad=0.80`:

`0.35(0.50) + 0.25(1.00) + 0.30(1.00) + 0.10(0.80) = 0.805` -> afinidad de **80.5 %**.

### Calculo de similitudes del grafo

Las conexiones se generan de forma determinista al registrar, modificar o eliminar contenidos:
- Misma ruta completa de categoria: `+0.55`.
- Misma categoria raiz, distinta subcategoria: `+0.20`.
- Etiquetas compartidas: `+0.45 * (interseccion / union)` (coeficiente de Jaccard).
- Se aplica maximo `1.0` y solo se guarda una arista si el peso es al menos `0.20`.

Se guarda una sola copia de cada relacion en `similitudes.txt` y al cargarla se reconstruyen las dos direcciones de la arista.

### BFS y deshacer (fase 3)

- Opcion 16 del menu: explora contenidos relacionados a uno o dos saltos de un ID. BFS usa la cola FIFO y una tabla hash de visitados. Prioriza menor numero de saltos; entre caminos de igual longitud conserva el mayor producto de pesos. No es Dijkstra ni busca la ruta de mayor peso sin limite de saltos.
- Opcion 15: deshace la ultima visualizacion, calificacion o cambio de favorito del usuario activo. Cada usuario tiene su propia pila en memoria. El resultado de deshacer se guarda, pero la pila se reinicia al cerrar el programa. Eliminar un contenido descarta las acciones correspondientes de todas las pilas.
- Para ver una relacion indirecta en el catalogo demo: como administrador agrega ID `201`, titulo `Puente de ciencia ficcion`, categoria `Tecnologia/Programacion`, popularidad `80`, etiquetas `espacio,aventura,ciencia ficcion`. En opcion 16 busca `101`; `107` aparecera a dos saltos mediante `201`. Elimina `201` al finalizar si quieres conservar el catalogo original.

## Persistencia

Archivos en `MotorRecomendacion/datos/`:

- `usuarios.txt`: ID, nombre, clave, rol, preferencias.
- `contenidos.txt`: ID, popularidad, titulo, categoria, etiquetas.
- `interacciones.txt`: ID usuario, ID contenido, vistas, calificacion, favorito.
- `similitudes.txt`: pares de ID de contenidos y pesos.

Los campos de texto se serializan con `std::quoted`. El sistema valida rutas, cantidades (maximo 1000 preferencias o etiquetas), pesos finitos, duplicados y registros incompletos. Cada cambio se guarda inmediatamente.

En fase 3 se escribe primero el conjunto de cuatro archivos en la carpeta hermana `datos.fase3.tmp`. Solo cuando todos terminan correctamente se renombra `datos` a `datos.fase3.bak` y se publica el nuevo conjunto. Si falla la publicacion, se intenta restaurar el respaldo. Las mutaciones ordinarias se revierten en memoria cuando guardar devuelve falso. Al reiniciar, si falta `datos` y existe el respaldo, se recupera; si ambos existen, se valida el conjunto nuevo antes de descartar el respaldo.

No es una transaccion ACID ni se garantiza durabilidad ante un corte electrico; no ejecutar dos instancias sobre la misma carpeta. Si queda una carpeta `.fase3.tmp` por una interrupcion, el guardado se bloquea deliberadamente: cierra el programa, conserva una copia fuera de la carpeta de trabajo y retira el temporal tras verificar los datos. Si aparece un error de restauracion, conserva tanto los datos como los respaldos y revisalos antes de continuar. La carpeta `datos` debe contener exclusivamente los cuatro archivos del sistema.

## Estructura principal y complejidad

Notacion: C = contenidos; V = vertices; E = aristas; F = filas de matriz; r = interacciones del usuario; Q = columnas; q = interacciones de una columna; P = preferencias; T = maximo de etiquetas por contenido; D = grado maximo; B = capacidad de la tabla del catalogo; K = resultados pedidos. Se consideran comparaciones de texto de costo unitario para simplificar; el costo real depende de su longitud.

| Operacion | Complejidad |
| --- | --- |
| Hash buscar/eliminar | promedio O(1), peor O(n) por colisiones |
| Hash insertar | promedio amortizado O(1), peor O(n) por colisiones o redimensionamiento |
| Grafo localizar vertice | promedio O(1), peor O(V), mediante el indice hash nuevo |
| Grafo consultar arista | promedio O(1 + D), peor O(V + D) |
| BFS | promedio O(V + E) para recorrido completo; peor O(V(V + E)) si todas las claves colisionan; memoria O(V). Con limite de saltos solo visita el subgrafo alcanzable. |
| Cola y pila | insertar y retirar O(1), memoria O(n); purgar pila O(n) |
| Arbol N-ario | proporcional a hermanos recorridos por nivel y longitud de la ruta |
| Matriz consultar | O(F + r); mejor O(1) |
| Matriz eliminar celda | O(F + r + Q + q) por encabezados, enlaces y limpieza |
| Heap insertar | O(log n) amortizado, peor O(n) al ampliar; mejor O(1) sin ampliar |
| Heap extraer | peor O(log n), mejor O(1) |
| Reconstruir indices | compara O(C^2) pares; con hash promedio y listas de aristas, O(C^2 (1 + T^2 + D)) mas construccion del arbol; puede llegar a orden cubico en grafo denso aun con etiquetas acotadas. |
| Recomendacion | promedio O(B + C(F + r + P(1 + T) + V + E) + C log C + K log C), cota conservadora con BFS por candidato; memoria auxiliar O(C + V). |

El BFS usa hash para evitar buscar cada vertice en una lista lineal; marcar visitados al encolar impide repeticiones en ciclos. La matriz dispersa evita reservar una matriz completa usuarios x contenidos. El heap evita ordenar toda la salida por un algoritmo cuadratico. El arbol N-ario representa categorias con cualquier numero de hijos, a diferencia de un AVL (util para busqueda ordenada de claves, no para representar directamente la jerarquia). El limite de dos saltos reduce relaciones lejanas poco relevantes. El motor aun reserva arreglos de V elementos por candidato; se puede optimizar reutilizando un buffer o recorriendo desde el historial una sola vez por semilla.

## Pruebas

El proyecto `Pruebas` ejecuta 11 grupos: hash, grafo, arbol, matriz, heap, sistema/persistencia, cola/pila, BFS, afinidad, casos limite/deshacer y persistencia robusta. Las comprobaciones no dependen de `assert` ni de NDEBUG. Cada prueba de persistencia usa una carpeta temporal propia y nunca modifica el catalogo real.

Para medir rendimiento en Visual Studio: propiedades del proyecto Pruebas -> Depuracion -> Argumentos del comando: `--rendimiento`. Usa Release x64 y ejecuta. Obtendras un CSV en consola. Las mediciones son del motor aislado, no del guardado ni de la reconstruccion de indices. El grafo sintetico es una cadena de C vertices y C-1 aristas con cinco interacciones; se promedian 100 BFS completos y 3 recomendaciones top 10 de dos saltos. En Linux tambien puedes compilar:

```bash
g++ -std=c++17 -O2 -DNDEBUG Pruebas/Pruebas.cpp MotorRecomendacion/{ArbolCategorias,Grafo,GestorPersistencia,MatrizDispersa,MotorRecomendacion,Sistema}.cpp -o pruebas
./pruebas
./pruebas --rendimiento
```

Resultados de una ejecucion de referencia: `EVIDENCIA/rendimiento.csv`. Repetir en el equipo de la defensa; no son garantias de velocidad.

## Limitaciones deliberadas

Prototipo de consola, no servicio multiusuario. Claves en texto plano, control de rol en el menu, sin recuperacion de claves ni seguridad contra edicion directa de archivos. Comparacion de texto ASCII, sin normalizacion Unicode. La pila de deshacer es temporal. El guardado por intercambio de carpetas no ofrece garantias ACID. El grafo se reconstruye completamente cuando cambia el catalogo. El algoritmo se basa en similitud de contenidos y preferencias, no implementa filtrado colaborativo ni aprendizaje automatico.
