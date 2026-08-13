# Codexion — Hoja de ruta

> Generado a partir de `es.subject.pdf` y del estado actual del código en `/home/saperez-/Milestone3/Codexion`.
> Este archivo es solo un plan de trabajo, **no modifica ningún archivo de código**.

---

## 0. Resumen del proyecto (subject)

Es una variante del problema de los "filósofos comensales" (dining philosophers), pero con **dongles USB** en vez de tenedores:

- Hay `N` personas (`number_of_coders`), cada una es **un hilo**.
- Hay `N` dongles en la mesa, uno entre cada par de personas (circular). Si `N == 1`, solo hay un dongle.
- Cada persona cicla entre 3 estados: **compilar → depurar → refactorizar → (vuelve a compilar)**.
- Para compilar necesita **los dos dongles** (izquierdo y derecho) simultáneamente durante `time_to_compile` ms.
- Al terminar de compilar, libera ambos dongles y pasa a depurar (`time_to_debug` ms), luego a refactorizar (`time_to_refactor` ms), y vuelve a intentar coger dongles para compilar.
- Si una persona no **empieza** a compilar dentro de `time_to_burnout` ms desde el inicio de su última compilación (o desde el inicio de la simulación), **se agota** ("burned out") y la simulación se detiene.
- Un dongle liberado no puede volver a cogerse hasta que pase `dongle_cooldown` ms (**cooldown obligatorio**).
- Cuando varias personas piden el mismo dongle, se arbitra según `scheduler`:
  - `fifo`: se sirve en orden de llegada de la solicitud.
  - `edf`: se sirve a quien tenga el deadline más próximo (`deadline = last_compile_start + time_to_burnout`).
- Debe existir un **hilo monitor separado** que detecte el agotamiento con precisión y pare la simulación.
- La simulación también se detiene si **todas** las personas han compilado al menos `number_of_compiles_required` veces.
- El log debe usar mutex para serializar la salida (nunca mezclar mensajes), y el mensaje de "burned out" debe aparecer como máximo 10 ms después del agotamiento real.
- **Prohibidas las variables globales.**
- Debe implementarse una **cola de prioridad (heap)** a mano para el scheduler FIFO/EDF (no hay librería estándar en C89 para esto).

### Argumentos del programa (los 8, todos obligatorios)

```
./codexion number_of_coders time_to_burnout time_to_compile time_to_debug \
           time_to_refactor number_of_compiles_required dongle_cooldown scheduler
```

### Funciones autorizadas

```
pthread_create, pthread_join, pthread_mutex_init, pthread_mutex_lock,
pthread_mutex_unlock, pthread_mutex_destroy, pthread_cond_init,
pthread_cond_wait, pthread_cond_timedwait, pthread_cond_broadcast,
pthread_cond_destroy, gettimeofday, usleep, write, malloc, free,
printf, fprintf, strcmp, strlen, atoi, memset
```

### Formato de log obligatorio

```
timestamp_in_ms X has taken a dongle
timestamp_in_ms X is compiling
timestamp_in_ms X is debugging
timestamp_in_ms X is refactoring
timestamp_in_ms X burned out
```

### Entrega

- Archivos a entregar: `Makefile`, `*.c`, `*.h` **dentro del directorio `coders/`**.
- `Makefile` con reglas `$(NAME)`, `all`, `clean`, `fclean`, `re` (+ `bonus` si hay bonus), compilado con `cc -Wall -Wextra -Werror -pthread`, sin relink.
- `libft`: **no permitido** en la parte obligatoria.
- `README.md` en la raíz del repo con, como mínimo:
  1. Primera línea en cursiva: *Este proyecto ha sido creado como parte del currículo de 42 por \<login1\>[, \<login2\>...]*.
  2. Sección **Descripción**.
  3. Sección **Instrucciones** (compilación, instalación, ejecución).
  4. Sección **Recursos** (referencias + descripción del uso de IA: para qué se usó y en qué partes).
  5. Específico de este proyecto: sección **"Blocking cases handled"** (deadlocks, condiciones de Coffman, inanición, cooldown, detección de agotamiento, serialización del log).
  6. Específico de este proyecto: sección **"Thread synchronization mechanisms"** (mutex/condvar usados, cómo se evitan race conditions, comunicación thread-safe entre coders y el monitor).
- Solo se evalúa lo que esté en el **repositorio Git asignado**.

---

## 1. Estado actual de tu código (auditoría)

Ubicación actual: `/home/saperez-/Milestone3/Codexion` (archivos **sueltos**, fuera del repo git).
Repo git real: `/home/saperez-/Milestone3/Codexion/Github` (remote `MotrisNF/Codexion`), y **ese repo solo contiene el PDF del subject** — ningún archivo `.c`/`.h`/`Makefile` está todavía versionado ni commiteado ahí.

| Archivo | Estado | Notas |
|---|---|---|
| `codexion.h` | Parcial | Define `t_args` (argumentos) y un `t_coder` muy incompleto. Faltan: struct de simulación global (sin usar variables globales, así que debe pasarse por puntero a todos los hilos), mutexes de dongles, condvars, id de hilo, `pthread_t`, punteros a dongles vecinos, mutex de log, tiempos de inicio, contador de compilaciones, heap/cola de prioridad, struct de "dongle". |
| `check_args.c` | Bastante avanzado | Parsea y valida los 8 argumentos, comprueba `<=0` y el valor de `scheduler`. **Falta**: rechazar valores no enteros (p. ej. `"12abc"`, `"1.5"`, espacios) — el subject lo pide explícitamente ("Rechaza entradas no válidas como números negativos, valores no enteros o un scheduler diferente de fifo o edf"); `atoi` no detecta esto. También falta comprobar overflow de `atoi`. |
| `main.c` | Muy inicial | Solo llama a `check_args` e imprime "Todo ok." No crea ningún hilo, no arranca simulación. |
| `thread_creator.c` | Vacío | Solo tiene la cabecera de 42, ni una línea de código. |
| `Makefile` | **No existe** | Nada compilado con reglas oficiales; hay un `a.out` suelto de una compilación manual. |
| `README.md` | **No existe** | — |
| Directorio `coders/` | **No existe** | El subject exige que todo el código fuente esté ahí. |
| Dongles / mutex / cooldown / scheduler / heap / monitor / logging | **No implementado** | Es el 90% del proyecto. |

**Conclusión**: solo tienes hecho el *parseo y validación de argumentos* (~5-10% del proyecto). Todo el modelo de concurrencia (hilos, dongles, mutexes, condvars, cooldown, scheduler FIFO/EDF con heap, hilo monitor, logging serializado, condiciones de parada) está por hacer. Además, el código vive fuera del repositorio git que se va a evaluar.

---

## 2. Plan de trabajo detallado

### Fase 0 — Reestructuración del repositorio
- [ ] Mover/copiar `check_args.c`, `codexion.h`, `main.c`, `thread_creator.c` dentro de `Github/coders/` (el subject exige explícitamente que el código esté en un directorio llamado `coders/`).
- [ ] Verificar que `Github/` es realmente el repo que vas a entregar (remote `MotrisNF/Codexion`); si es así, trabaja directamente ahí de ahora en adelante para no tener dos copias divergentes.
- [ ] Añadir `.gitignore` (excluir `a.out`, binarios, `*.o`).
- [ ] Primer commit con la estructura base.

### Fase 1 — Diseño de estructuras de datos (sin variables globales)
Como las variables globales están prohibidas, necesitas un **struct raíz** (p. ej. `t_simulation`) que se cree en `main`, se rellene, y cuyo puntero se pase a cada hilo (coder + monitor). Debe incluir, como mínimo:

- [ ] `t_args` (ya la tienes) — parámetros del programa.
- [ ] Array de `t_dongle` (uno por persona), cada uno con:
  - `pthread_mutex_t mutex` (o gestionado por el mutex/condvar del array),
  - estado: libre / en uso,
  - `long available_at_ms` (timestamp desde el que vuelve a estar disponible, para el cooldown),
  - cola de espera (para FIFO) o referencia al heap global (para EDF).
- [ ] Array de `t_coder`, cada uno con:
  - `id` (1..N),
  - `pthread_t thread`,
  - punteros/índices a su dongle izquierdo y derecho,
  - `long last_compile_start_ms`,
  - `int compilations_done`,
  - puntero al struct raíz de simulación (para acceder a mutexes compartidos, log, tiempo de inicio, flag de parada, etc.).
- [ ] Un mutex global de log (`pthread_mutex_t log_mutex`) para serializar `printf`.
- [ ] Un flag de parada de simulación (`int stop`, protegido por su propio mutex) que todos los hilos consultan en cada iteración de su bucle.
- [ ] `long start_time_ms` (timestamp de arranque de la simulación, usado como base para todos los timestamps de log).
- [ ] Estructura de **cola de prioridad (heap binario)** para el scheduler: necesitas implementarla tú mismo (insertar, extraer-mínimo, comparador FIFO por orden de llegada / EDF por deadline). Esto es una función/fichero aparte (p. ej. `heap.c`).

> Nota de Norma: cada función debe hacer una sola cosa, máx. ~25 líneas, máx. 4 parámetros — vas a necesitar bastantes structs y funciones pequeñas auxiliares (init, destroy, getters) para cumplirlo.

### Fase 2 — Ciclo de vida de una persona (coder thread)
- [ ] Implementar `thread_creator.c` (o renombrar/organizar como prefieras) con la función que lanza `pthread_create` para cada coder y para el monitor.
- [ ] Implementar la rutina de cada hilo coder (bucle):
  1. Comprobar si debe parar (flag de stop).
  2. Solicitar dongle izquierdo y derecho (según el `scheduler`), esperando con `pthread_cond_wait`/`pthread_cond_timedwait` si no están libres o están en cooldown.
  3. Al obtener cada uno: loggear `"X has taken a dongle"`.
  4. Loggear `"X is compiling"`, dormir/esperar `time_to_compile` ms (con `usleep`, revisando periódicamente el flag de stop si quieres poder abortar rápido).
  5. Liberar ambos dongles (marcar cooldown = `now + dongle_cooldown`), señalizar con `pthread_cond_broadcast` a quien esté esperando.
  6. Incrementar `compilations_done`, actualizar `last_compile_start_ms`.
  7. Loggear `"X is debugging"`, esperar `time_to_debug` ms.
  8. Loggear `"X is refactoring"`, esperar `time_to_refactor` ms.
  9. Volver a 1.
- [ ] Actualizar el `last_compile_start_ms` de forma que el hilo **monitor** pueda leerlo de forma segura (mutex por coder, o un mutex general si prefieres simplicidad antes que rendimiento).

### Fase 3 — Dongles: mutex + cooldown + arbitraje justo
- [ ] Función para "solicitar dongle" que:
  - bloquea el mutex del dongle,
  - comprueba si está libre y si ya pasó el cooldown,
  - si no, encola la solicitud (heap/lista) y espera en condvar,
  - al despertar, revalida condición (evitar *wakeups espurios* y evitar que dos personas se lleven el mismo dongle).
- [ ] Función para "liberar dongle" que marca el timestamp de disponibilidad futura (`now + dongle_cooldown`) y notifica a quien espera.
- [ ] Implementar el arbitraje:
  - **FIFO**: cola simple de solicitudes por orden de llegada.
  - **EDF**: heap de prioridad por `last_compile_start + time_to_burnout` (deadline más próximo primero). Recalcular/comparar deadlines dinámicamente.
- [ ] Cuidado especial con **interbloqueo (deadlock)**: si todos cogen su dongle izquierdo primero y esperan el derecho, se bloquea todo el sistema (el clásico problema de los filósofos). Debes romper la simetría (p. ej. adquirir ambos dongles de forma atómica bajo un mismo mutex de "petición", o forzar un orden de adquisición, o pedir los dos a la vez y esperar juntos con una única espera condicional).
- [ ] Caso especial `number_of_coders == 1`: solo hay un dongle en la mesa, así que esa persona **nunca podrá compilar** (necesita dos) y se agotará inevitablemente — confirma en el subject/hoja de evaluación si esto se considera comportamiento esperado o si hay que tratarlo aparte.

### Fase 4 — Hilo monitor
- [ ] Crear un hilo dedicado (`pthread_create`) que, en bucle:
  - recorre a todos los coders,
  - para cada uno calcula si `now - last_compile_start_ms > time_to_burnout` → si es así, loggea `"X burned out"` **dentro de los 10 ms** posteriores al momento real, activa el flag de stop y despierta a todos los hilos (broadcast) para que terminen.
  - también comprueba la condición de parada por éxito: todos los coders con `compilations_done >= number_of_compiles_required`.
- [ ] Ajustar la frecuencia de sondeo del monitor (p. ej. `usleep` de 1 ms o menos) para cumplir el margen de precisión de 10 ms sin consumir CPU en exceso.
- [ ] Al terminar, unir todos los hilos (`pthread_join`) — coders + monitor.

### Fase 5 — Logging serializado
- [ ] Toda escritura de log pasa por una única función (`log_event(sim, coder_id, "is compiling")`) que:
  - calcula el timestamp en ms desde el inicio (`gettimeofday` - `start_time_ms`),
  - bloquea el mutex de log,
  - imprime con `printf`/`write` en el formato exacto pedido,
  - desbloquea el mutex.
- [ ] Verificar que ningún mensaje se corta o se entrelaza con otro (probar con muchos coders y tiempos cortos).

### Fase 6 — Condiciones de parada
- [ ] Parar por agotamiento (alguien se agota) → el monitor lo detecta y detiene todo.
- [ ] Parar por éxito (todos alcanzan `number_of_compiles_required`) → el monitor (u otro mecanismo compartido) lo detecta y detiene todo limpiamente, sin marcar a nadie como agotado.
- [ ] Todos los hilos deben salir de sus esperas (`pthread_cond_wait`/`timedwait`) al activarse el flag de stop — usar `pthread_cond_broadcast` y revisar el flag tras cada espera.

### Fase 7 — Gestión de memoria y errores
- [ ] Todo `malloc` debe tener su `free` correspondiente en todos los caminos (incluidos los de error).
- [ ] Todos los mutex/condvar deben inicializarse (`pthread_*_init`) y destruirse (`pthread_*_destroy`) correctamente.
- [ ] Revisar que ningún hilo quede "colgado" sin `pthread_join` (fugas de hilos) ni haya *use-after-free*.
- [ ] Nunca debe haber segfault/bus error/double free — el subject dice que si ocurre, la nota es 0 directamente.

### Fase 8 — Validación de argumentos (mejorar lo ya hecho)
- [ ] En `check_args.c`, reforzar la validación para rechazar explícitamente:
  - cadenas no numéricas o con caracteres extra (`"12x"`, `"1.5"`, `" 12"`, `"-5"` ya cubierto por `<=0` pero conviene detectarlo antes de `atoi` para dar mejor mensaje),
  - overflow (valores que no caben en `int`),
  - argumentos vacíos.
  - Esto se puede hacer con una función `is_valid_number(char *str)` que recorra dígito a dígito antes de llamar a `atoi`.
- [ ] Mantener el mensaje de uso/error claro cuando `argc != 9`.

### Fase 9 — Makefile
- [ ] Crear `Github/coders/Makefile` con:
  - `NAME = codexion`
  - reglas `all`, `clean`, `fclean`, `re`, `$(NAME)`
  - flags: `-Wall -Wextra -Werror -pthread`
  - compilador `cc`
  - sin relink (recompilar solo lo que cambió).
  - (si hay bonus) regla `bonus` que compile también los `*_bonus.c/.h` en archivos aparte.

### Fase 10 — Norma
- [ ] Pasar el checker de Norma sobre **todos** los `.c`/`.h` del proyecto (incluidos los auxiliares como el heap) — un solo error de Norma en cualquier archivo pone la nota a 0.
- [ ] Revisar nombres de funciones/variables, longitud de funciones, número de variables por función, ausencia de `for`/`while` múltiples anidados más allá de lo permitido, etc.

### Fase 11 — Pruebas
- [ ] Probar con `number_of_coders = 1, 2, 3, 5, 10...`.
- [ ] Probar `scheduler = fifo` y `scheduler = edf` por separado.
- [ ] Probar parámetros "viables" donde nadie debería agotarse bajo `edf` (el subject exige que el programa lo garantice) — validar que efectivamente no se agota nadie.
- [ ] Probar parámetros "no viables" (p. ej. `time_to_burnout` muy bajo) y comprobar que el log de "burned out" aparece dentro de los 10 ms del momento real.
- [ ] Probar que la simulación se detiene correctamente al alcanzar `number_of_compiles_required` para todos.
- [ ] Ejecutar con `valgrind --leak-check=full --show-reachable=yes` (o `helgrind`/`ThreadSanitizer` para condiciones de carrera) para verificar ausencia de leaks y race conditions.
- [ ] Ejecutar con `-fsanitize=thread` en una build de pruebas para detectar data races reales.
- [ ] (Recordatorio: estos programas de prueba no se entregan ni evalúan, pero está permitido usarlos durante tu propia evaluación entre pares).

### Fase 12 — README.md
- [ ] Primera línea en cursiva: `*Este proyecto ha sido creado como parte del currículo de 42 por <tu_login>*`.
- [ ] Sección **Descripción**.
- [ ] Sección **Instrucciones** (cómo compilar con `make`, cómo ejecutar, ejemplo de comando).
- [ ] Sección **Recursos** (documentación sobre pthreads/mutex/condvar que hayas usado, artículos sobre el problema de los filósofos comensales, EDF scheduling, etc. + descripción honesta de en qué partes usaste IA y para qué).
- [ ] Sección **Blocking cases handled** (cómo evitas deadlock, condiciones de Coffman, inanición, cómo gestionas el cooldown, cómo detectas el agotamiento con precisión, cómo serializas el log).
- [ ] Sección **Thread synchronization mechanisms** (qué mutex/condvar usas y para qué, cómo evitas race conditions, cómo se comunican coders y monitor de forma thread-safe).

### Fase 13 — Entrega final
- [ ] Confirmar que `coders/` contiene exactamente `Makefile`, `*.c`, `*.h` (y `*_bonus.{c,h}` si hay bonus).
- [ ] Confirmar que `README.md` está en la **raíz** del repo.
- [ ] `git add`, commit y `push` al repo asignado (`MotrisNF/Codexion`), verificando que compila limpio desde cero (`make re`) y pasa Norma justo antes de entregar.
- [ ] Revisar el enunciado de "Recode instructions" (Cap. VIII): durante la evaluación te pueden pedir una pequeña modificación en vivo — asegúrate de entender bien tu propio código (no solo que compile) para poder justificarlo y modificarlo rápido en la defensa.

---

## 3. Prioridad sugerida (si quieres ir paso a paso)

1. Reestructurar repo → `coders/` dentro de `Github/` (Fase 0).
2. Diseñar structs (Fase 1) — es la base de todo lo demás.
3. Implementar heap/cola de prioridad aislado y testeable (parte de Fase 1/3).
4. Implementar dongles + mutex + cooldown + arbitraje FIFO (Fase 3, versión simple primero).
5. Implementar rutina de coder + logging (Fases 2 y 5).
6. Añadir scheduler EDF (Fase 3, versión completa).
7. Añadir hilo monitor y condiciones de parada (Fases 4 y 6).
8. Endurecer validación de argumentos (Fase 8).
9. Makefile + Norma (Fases 9 y 10).
10. Pruebas exhaustivas con valgrind/tsan (Fase 11).
11. README.md (Fase 12).
12. Entrega (Fase 13).
