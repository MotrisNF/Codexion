# Codexion — Guía de trabajo

> Generado a partir de `es.subject.pdf` y del estado real del código en `/home/saperez-/Milestone3/Codexion` (repo git = esta misma carpeta, remote `origin` → `git@github.com:MotrisNF/Codexion.git`).
> Esto es una **guía de estructuras y firmas**, no una implementación: te digo qué structs/campos/funciones necesitas y qué debe cumplir cada una, pero la lógica interna (los `if`, los `while`, el orden de las líneas) la escribes tú.

---

## 0. Resumen del subject (referencia rápida)

- `N` personas (`number_of_coders`) = `N` hilos. `N` dongles en mesa circular (si `N==1`, 1 dongle).
- Ciclo por persona: **compilar → depurar → refactorizar → compilar...**
- Compilar exige **ambos dongles** (izq. y der.) a la vez, durante `time_to_compile` ms.
- Al terminar de compilar: libera ambos dongles (entran en `dongle_cooldown` ms de cooldown), pasa a depurar (`time_to_debug` ms), luego a refactorizar (`time_to_refactor` ms), y vuelve a pedir dongles.
- Si no **empieza** a compilar antes de `time_to_burnout` ms desde el inicio de su última compilación (o desde el arranque) → **burned out**, la simulación se para.
- Arbitraje entre solicitantes del mismo dongle, según `scheduler`:
  - `fifo`: orden de llegada de la solicitud.
  - `edf`: menor `deadline = last_compile_start_ms + time_to_burnout` primero.
- Hilo **monitor** separado, detecta agotamiento con precisión ≤ 10 ms.
- Parada también si **todas** las personas llevan `>= number_of_compiles_required` compilaciones.
- Log serializado con mutex (nunca mensajes cortados o mezclados).
- **Prohibidas variables globales** → todo pasa por puntero a un struct raíz creado en `main`.
- Cola de prioridad (heap binario) implementada a mano (C89, sin librería) para el scheduler FIFO/EDF.

### Argumentos (8, todos obligatorios, en este orden)
```
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

### Funciones autorizadas (única lista permitida)
```
pthread_create, pthread_join, pthread_mutex_init, pthread_mutex_lock,
pthread_mutex_unlock, pthread_mutex_destroy, pthread_cond_init,
pthread_cond_wait, pthread_cond_timedwait, pthread_cond_broadcast,
pthread_cond_destroy, gettimeofday, usleep, write, malloc, free,
printf, fprintf, strcmp, strlen, atoi, memset
```
> No hay `pthread_cond_signal` → tendrás que usar `pthread_cond_broadcast`.
> No hay `qsort`, `sprintf`, `snprintf`, `calloc`, `realloc`, `strtol` → no están disponibles.

### Entrega
- `Makefile`, `*.c`, `*.h` **dentro de `coders/`** (ya existe en la raíz del repo).
- `Makefile`: reglas `$(NAME)`, `all`, `clean`, `fclean`, `re` (+ `bonus` si aplica), compilado con `cc -Wall -Wextra -Werror -pthread`, sin relink innecesario.
- `libft` no permitida en obligatoria.
- `README.md` en la **raíz** del repo (no dentro de `coders/`) con las 6 secciones descritas en §11.
- Solo se evalúa el contenido del repo git (`origin/main`).

---

## 1. Estado real del código (auditado línea a línea)

Repo raíz = `/home/saperez-/Milestone3/Codexion` (ya es el repo git correcto). **Ya no existe una subcarpeta `Github/`** — cualquier referencia a esa ruta en versiones anteriores de este plan está obsoleta.

| Archivo | Estado real | Qué falta |
|---|---|---|
| `coders/codexion.h` | `t_args` completo (8 campos). `t_coder` con 10 campos, pero sin `pthread_t`, sin `id`, sin punteros a dongle (usa `int left_dongle`/`rigth_dongle` en vez de punteros a struct), sin puntero al struct raíz, sin mutex propio. No existen `t_dongle`, `t_sim`, `t_heap`. | Definir los structs de §2 y corregir `t_coder` (incluye el typo `rigth_dongle`). |
| `coders/check_args.c` | `fill_struct` usa `atoi` puro. `check_struct_values` valida `<=0` y el valor de `schedule`. | No detecta no-numéricos (`"12x"`, `"1.5"`, `" 12"`) ni overflow. Falta `is_valid_number()` (§9). |
| `coders/main.c` | Llama a `cheack_args`, imprime "Todo ok.", libera y sale. | No crea `t_sim`, no lanza hilos, no llama a `pthread_join`, no hay limpieza de mutexes. |
| `coders/thread_creator.c` | Una única línea: `pthread_t *ptrhead_matrix_generator()` sin cuerpo ni `;`. **No compila.** | Implementar de cero (§4). |
| `Makefile` | No existe. | Crear con las reglas exigidas (§10). |
| `README.md` | No existe. | Crear en la raíz (§11). |
| `.gitignore` | Existe (`a.out`, `*.o`). | OK, no tocar. |
| Dongles / cooldown / scheduler / heap / monitor / logging | No implementado. | Es el ~90% restante del proyecto. |

**Nota de compilación inmediata**: `thread_creator.c` tal cual **rompe el build** ahora mismo (`pthread_t *ptrhead_matrix_generator()` sin `{}` ni `;` es un error de sintaxis). Arréglalo antes de crear el `Makefile`, o fallará en cuanto exista.

---

## 2. Fase 1 — Structs necesarios (sin variables globales)

Como las variables globales están prohibidas, necesitas un struct raíz creado en `main` cuyo puntero se pase a cada hilo. Campos que te faltan por struct (añádelos a `codexion.h`, y corrige `t_coder`, que hoy usa `int` en vez de punteros):

**`t_dongle`** (uno por posición en la mesa, `N` en total):
- `id` (`int`) — identificador estable (0..N-1), lo necesitarás para el orden de adquisición anti-deadlock.
- estado libre/ocupado (`int`).
- `available_at_ms` (`long`) — instante a partir del cual deja de estar en cooldown.
- `pthread_mutex_t` propio.
- `pthread_cond_t` propio.
- alguna estructura de cola de espera (heap, ver §3) asociada a este dongle.

**`t_coder`** (uno por hilo/persona, corrige el actual):
- `id` (`int`).
- `pthread_t` del hilo.
- puntero a su dongle izquierdo y puntero a su dongle derecho (no `int`, sino `t_dongle *`).
- `last_compile_start_ms` (`long`).
- mutex propio para proteger ese campo (el monitor lo lee, el coder lo escribe).
- `compilations_done` (`int`).
- puntero al struct raíz de simulación.

**`t_sim`** (struct raíz, uno solo, creado en `main`):
- puntero a `t_args`.
- array de `t_dongle`.
- array de `t_coder`.
- `pthread_t` del hilo monitor.
- mutex de log.
- flag de parada (`int`) + mutex propio para ese flag.
- `start_time_ms` (`long`) — base de todos los timestamps del log.

**`t_heap`** (cola de prioridad, ver §3): un array de nodos, cada nodo con una clave (`long`, orden de llegada o deadline según scheduler) y el `id` del coder al que pertenece, más contadores de tamaño/capacidad.

Notas de diseño (para que decidas tú la implementación, no un algoritmo cerrado):
- El dongle izquierdo/derecho de cada coder depende de su posición en la mesa circular; decide tú cómo indexarlo a partir de `id` y `N`.
- Si `N == 1` solo hay un dongle en la mesa — decide cómo representas ese caso especial en los punteros izquierdo/derecho.
- El heap de espera puede ser uno por dongle (recomendado) o un diseño distinto — pero documenta tu elección.

Norma (verifica los límites exactos con el checker de tu campus, pero como referencia): máx. ~25 líneas por función, máx. 4 parámetros, máx. 5 variables declaradas por función → vas a necesitar bastantes funciones pequeñas de init/destroy/getter para cada struct.

---

## 3. Fase 1b — Cola de prioridad para el arbitraje (`coders/heap.c`)

**Para qué sirve, en una frase**: cuando varias personas piden el mismo dongle a la vez, algo tiene que decidir a quién se lo dan primero. Con `fifo` es fácil (el que llegó antes). Con `edf` hay que dar siempre el dongle a quien tenga el **deadline más próximo** (`last_compile_start + time_to_burnout`), y eso cambia según quién va llegando y a quién se le va sirviendo. El subject pide que esto se resuelva con una cola de prioridad (heap) porque en tu lista de funciones autorizadas no hay `qsort` para mantener nada ordenado de otra forma.

**No es una librería genérica**: es solo un array de tamaño fijo — como mucho esperan el mismo dongle `number_of_coders` personas a la vez, así que `capacity = number_of_coders` ya te vale y no necesitas `realloc` (que tampoco está autorizado). Con 3 operaciones te sobra:

```c
t_heap	*heap_create(int capacity);
void	heap_destroy(t_heap *heap);
int		heap_push(t_heap *heap, long key, int coder_id);
int		heap_pop_min(t_heap *heap);   // devuelve el coder_id de la clave mínima
int		heap_is_empty(t_heap *heap);
```
- `key`: para `fifo` usa un contador de orden de llegada; para `edf` usa el deadline (`last_compile_start + time_to_burnout`). Es el mismo heap para los dos schedulers, solo cambia qué metes como `key`.
- La inserción y la extracción reordenan el array (típico "subir"/"bajar" el elemento movido); sepáralo en una función auxiliar por cada una para no pasarte del límite de líneas de Norma.
- Alternativa mínima si un heap "de verdad" te bloquea: un array simple donde `push` añade al final y `pop_min` recorre buscando la clave menor (O(n) en vez de O(log n)). Con `number_of_coders` pequeño el rendimiento no es el problema — pero el subject pide explícitamente la estructura heap, así que documenta en el README si te desvías de eso y por qué.
- Pruébalo aislado antes de integrarlo: un `main` de prueba temporal que empuje varias claves y compruebe que `heap_pop_min` las devuelve en orden ascendente.

---

## 4. Fase 2/3 — Dongles y rutina del coder

### 4.1 Adquisición de dongles (evitar deadlock)

Problema clásico de los filósofos: si todos los hilos cogen su dongle izquierdo primero y esperan el derecho, el sistema se bloquea. Tienes que romper esa simetría con algún criterio (por ejemplo, basado en el `id` de cada dongle, o en un mutex/orden de petición distinto) — decide y justifica tu estrategia en el README (§11).

Funciones que necesitas en `coders/dongle.c` (firma y responsabilidad, sin cuerpo):

```c
int		dongle_acquire(t_dongle *dongle, t_coder *coder);
```
Responsabilidad: bloquear el hilo llamante hasta que el dongle esté libre, haya pasado su cooldown, y le corresponda según el `scheduler` (`fifo`/`edf`) — o hasta que la simulación se pare. Debe registrar la solicitud de alguna forma (heap de espera del dongle) y revalidar la condición completa cada vez que se despierta (para evitar wakeups espurios y evitar que dos coders se lleven el mismo dongle). Al conseguirlo, debe loggear `"has taken a dongle"`. Devuelve un código para distinguir "conseguido" de "la simulación se ha parado mientras esperaba".

```c
int		dongle_acquire_pair(t_coder *coder);
```
Responsabilidad: obtener los dos dongles del coder (izquierdo y derecho) aplicando tu criterio anti-deadlock, gestionando qué hacer si consigue el primero pero no el segundo (o la simulación se para entre medias).

```c
void	dongle_release(t_dongle *dongle, t_coder *coder);
```
Responsabilidad: marcar el dongle como libre, fijar su nuevo `available_at_ms` (ahora + `dongle_cooldown`), y despertar a quien esté esperando por él (`pthread_cond_broadcast`, no hay `signal` disponible).

Ten en cuenta: cada espera con `pthread_cond_wait`/`pthread_cond_timedwait` debe volver a comprobar todas sus condiciones al despertar, incluido el flag de parada de la simulación.

### 4.2 Rutina del hilo coder (`coders/coder_routine.c`)

```c
void	*coder_routine(void *arg);   // arg = t_coder*, firma exigida por pthread_create
```
Debe implementar el ciclo completo de una persona, repitiendo hasta que la simulación se pare:
- comprobar el flag de parada al inicio de cada vuelta,
- registrar el instante en que empieza a intentar compilar (para que el monitor calcule el burnout),
- conseguir ambos dongles (`dongle_acquire_pair`),
- loggear y esperar `time_to_compile` ms,
- liberar ambos dongles,
- contabilizar la compilación,
- loggear y esperar `time_to_debug` ms,
- loggear y esperar `time_to_refactor` ms.

Detalles a decidir tú: cómo trocear las esperas (`usleep`) para poder reaccionar rápido si el monitor activa el flag de parada a mitad de una espera larga; bajo qué mutex proteges la escritura/lectura de `last_compile_start_ms` y `compilations_done` (el monitor los lee desde otro hilo).

---

## 5. Fase 4 — Hilo monitor (`coders/monitor.c`)

```c
void	*monitor_routine(void *arg);   // arg = t_sim*
```
Responsabilidad, en bucle, con un sondeo lo bastante frecuente para cumplir el margen de 10 ms (piensa en el orden de 1 ms, ajusta según pruebas):
- para cada coder, calcular si ha pasado más de `time_to_burnout` ms desde su `last_compile_start_ms` sin haber empezado a compilar → si es así, loggear `"burned out"`, activar el flag de parada y despertar a todos los hilos que puedan estar esperando en algún `cond_wait`.
- comprobar también si todos los coders han alcanzado `number_of_compiles_required` → si es así, activar el flag de parada (sin loggear "burned out").
- decidir la frecuencia de sondeo (`usleep`) que cumpla el margen de precisión sin consumir CPU en exceso.

Necesitarás alguna forma de despertar a los hilos que están en `cond_wait` sobre condvars de dongles distintas cuando el monitor decide parar — piensa cómo vas a alcanzar todas esas condvars desde un único hilo monitor.

---

## 6. Fase 5 — Logging serializado (`coders/log.c`)

```c
long	now_ms(t_sim *sim);   // timestamp actual menos el instante de arranque
void	log_event(t_sim *sim, int coder_id, const char *event);
```
- Un único punto de `printf`/`write` en todo el proyecto, protegido por el mutex de log.
- Formato de línea exigido por el subject: `timestamp_in_ms X <evento>`.
- Eventos literales a usar (coinciden con el subject palabra por palabra): `"has taken a dongle"`, `"is compiling"`, `"is debugging"`, `"is refactoring"`, `"burned out"`.
- Decide si `X` es el `id` 0-based o 1-based y sé consistente en todo el log; documenta tu elección si el subject no lo especifica.
- Verifica manualmente con muchos coders y tiempos cortos que ninguna línea sale cortada o mezclada con otra.

---

## 7. Fase 6 — Condiciones de parada (resumen operativo)

Dos formas de fijar el flag de parada, ambas decididas solo por el **monitor**:
1. Agotamiento de alguien → loggear `"burned out"` primero, luego parar, luego despertar a todos.
2. Éxito global (todos alcanzan `number_of_compiles_required`) → parar directo, sin loggear burnout.

Todas las esperas (`cond_wait`/`timedwait`) deben revisar el flag de parada al despertar para poder salir de verdad. Tras salir de `coder_routine`, cada hilo simplemente termina; `main` debe unir (`pthread_join`) los `N` coders y el monitor antes de liberar cualquier memoria.

---

## 8. Fase 7 — Memoria y errores (checklist de verificación)

- [ ] Cada `pthread_mutex_init`/`pthread_cond_init` tiene su `_destroy` correspondiente.
- [ ] Cada `malloc` tiene su `free`, incluidos los caminos de error (si algo falla a mitad de la inicialización, libera lo ya reservado antes de salir).
- [ ] `pthread_join` se llama para **todos** los hilos creados, sin excepción, antes de liberar memoria.
- [ ] Ejecuta con `valgrind --leak-check=full --show-reachable=yes ./codexion ...` y confirma que no hay leaks.
- [ ] Ejecuta con `-fsanitize=thread` (build de test aparte) o `helgrind` para detectar data races.
- [ ] El subject dice que un segfault/bus error/double free en evaluación implica **nota 0** — trátalo como criterio de bloqueo.

---

## 9. Fase 8 — Validación de argumentos (`check_args.c`)

Función nueva a añadir (en `check_args.c` o en un archivo aparte, p. ej. `validate_number.c`):
```c
int	is_valid_number(const char *str);   // 1 si es un entero válido (con overflow check), 0 si no
```
Qué debe rechazar, exactamente:
- cadena vacía,
- cualquier carácter que no sea dígito (aparte de un signo opcional al principio),
- overflow de `int` (sin usar `strtol`, que no está en la lista de funciones autorizadas — decide tú una estrategia de acumulación/comparación manual).

Debe llamarse antes de `atoi` para los 7 argumentos numéricos (todos menos el scheduler). Mantén el `argc != 9` como primera comprobación (ya está bien en `cheack_args`).

---

## 10. Fase 9 — Makefile

Archivo: `coders/Makefile`. Requisitos exigidos por el subject, no una implementación completa:
- variable `NAME` con el nombre del binario,
- reglas `all`, `$(NAME)`, `clean` (borra `.o`), `fclean` (borra también el binario), `re` (`fclean` + `all`),
- compilador `cc`, flags `-Wall -Wextra -Werror -pthread`,
- compilación incremental (solo recompila los `.c` que cambiaron, no relink completo cada vez — depende de `codexion.h` para invalidar objetos si cambia la cabecera),
- si añades bonus, una regla `bonus` que compile los `*_bonus.c/.h` como archivos separados de la parte obligatoria.

Verifica que `make`, `make clean`, `make fclean`, `make re` funcionan **desde dentro de `coders/`**.

---

## 11. Fase 10 — Norma (checklist de auto-verificación)

Antes de entregar, pasa el checker de Norma de tu campus sobre **todos** los `.c`/`.h` de `coders/` (incluido `heap.c`) y confirma cero errores. Puntos donde este diseño suele generar problemas si no divides bien las funciones:
- las operaciones del heap (inserción/extracción con reordenamiento) tienden a superar el límite de líneas si no separas el reordenamiento en su propia función,
- `dongle_acquire` tiene varias condiciones que comprobar — considera extraer esa comprobación a una función aparte que devuelva `int`,
- revisa el número de variables declaradas por función (los structs grandes tientan a declarar muchas locales de golpe).

---

## 12. Fase 11 — Plan de pruebas (comandos concretos)

```bash
cd coders && make re

# Caso trivial N=1 (debe agotarse, documenta por qué en el README)
./codexion 1 2000 200 200 200 3 100 fifo

# Caso viable pequeño, FIFO
./codexion 5 800 200 200 200 5 50 fifo

# Mismo caso, EDF (no debería agotarse nadie si los parámetros son viables)
./codexion 5 800 200 200 200 5 50 edf

# Caso no viable (burnout muy ajustado) -> comprobar log "burned out" y timestamp
./codexion 5 250 200 200 200 5 50 fifo

# Memoria y race conditions
valgrind --leak-check=full --show-reachable=yes ./codexion 5 800 200 200 200 5 50 fifo
```
- Para el caso EDF "viable", verifica que ningún coder recibe `"burned out"`.
- Para el caso "no viable", comprueba que el log de `"burned out"` aparece dentro de los 10 ms del momento real en que debería haberse detectado.
- Prueba también con `number_of_coders` alto (10-20) y tiempos muy cortos para forzar contención real sobre los dongles.
- Build de test con TSan (no se entrega, solo para depurar):
```bash
cc -Wall -Wextra -pthread -fsanitize=thread *.c -o codexion_tsan
./codexion_tsan 5 800 200 200 200 5 50 edf
```

---

## 13. Fase 12 — README.md (raíz del repo, estructura exigida)

Secciones obligatorias, en este orden:
1. Primera línea en cursiva: `*Este proyecto ha sido creado como parte del currículo de 42 por saperez-*`.
2. **Descripción**.
3. **Instrucciones** (compilación: `cd coders && make`; ejecución con ejemplo de comando).
4. **Recursos** (documentación consultada + descripción honesta de en qué partes usaste IA y para qué).
5. **Blocking cases handled** (cómo evitas deadlock, starvation, cómo gestionas el cooldown, cómo detectas el agotamiento con precisión, cómo serializas el log).
6. **Thread synchronization mechanisms** (qué mutex/condvar usas y para qué, cómo evitas race conditions, cómo se comunican coders y monitor de forma thread-safe).

---

## 14. Fase 13 — Entrega final (checklist literal)

- [ ] `coders/` contiene exactamente: `Makefile`, `*.c`, `*.h` (+ `*_bonus.{c,h}` si hay bonus, en archivos separados de la obligatoria).
- [ ] `README.md` está en la **raíz** del repo (no dentro de `coders/`).
- [ ] `cd coders && make re` compila limpio, sin warnings, con `-Wall -Wextra -Werror -pthread`.
- [ ] Norma sin errores en todos los archivos.
- [ ] `git status` limpio, todo commiteado y pusheado a `origin/main`.
- [ ] Repasa el capítulo "Recode instructions" del subject: en la defensa te pueden pedir modificar algo en vivo — asegúrate de poder explicar y tocar cualquier parte de tu propio código sin depender de este plan.

---

## 15. Orden de implementación recomendado (con dependencias)

1. Arreglar `thread_creator.c` (ahora mismo no compila) — aunque sea con un stub válido, para no bloquear `make`.
2. `codexion.h`: añadir `t_dongle`, `t_heap`, `t_sim`, corregir `t_coder` (§2).
3. `heap.c` aislado y probado con un `main` temporal (§3).
4. `validate_number.c` + integrarlo en `check_args.c` (§9) — independiente del resto, buena victoria rápida.
5. `dongle.c`: `dongle_acquire`, `dongle_release`, `dongle_acquire_pair` (§4.1).
6. `log.c`: `now_ms`, `log_event` (§6) — lo necesitas antes de probar nada con salida legible.
7. `coder_routine.c` (§4.2).
8. `monitor.c` (§5).
9. `main.c` + `thread_creator.c` reales: creación del struct raíz, `pthread_create` de N coders + monitor, `pthread_join` de todos, limpieza final (§1, §8).
10. Pruebas con los comandos de §12 (empieza con FIFO simple antes de EDF).
11. Norma (§11).
12. `README.md` (§13).
13. Entrega (§14).
