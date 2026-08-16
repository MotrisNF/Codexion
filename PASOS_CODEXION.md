# Codexion — Qué queda por hacer

> Este documento sustituye a la versión anterior. Ya no describe "todo el proyecto desde cero": describe el estado real tras completar los puntos 1-4 (header, `heap.c`, `validate_number.c`, `dongle.c`), las decisiones de diseño ya tomadas en ese código, y todo lo que queda pendiente, fase a fase. Deliberadamente no contiene código: es una guía de responsabilidades y decisiones, la implementación la escribes tú.

---

## 0. Lo que ya está hecho (puntos 1-4)

- `coders/codexion.h`: structs completos (`t_args`, `t_heap_node`, `t_heap`, `t_dongle`, `t_coder`, `t_sim`) y todos los prototipos, incluidos los de funciones que aún no existen (`log_event`).
- `coders/heap.c` + `coders/heap_utils.c`: min-heap binario completo y probado (inserción/extracción en orden correcto). Está partido en dos archivos porque `heap.c` solo (API pública + reordenamiento) tenía 8 funciones y el límite de Norma es 5 por archivo: `heap.c` se quedó con `heap_create/destroy/is_empty/push/pop_min` (la API que usa el resto del proyecto) y `heap_utils.c` con `heap_swap/heap_sift_up/heap_sift_down` (el reordenamiento interno, ya no `static` porque ahora vive en otro archivo).
- `coders/validate_number.c`: `is_valid_number` completo y probado (rechaza vacíos, no numéricos, espacios, overflow de `int`; acepta signo opcional y los límites exactos `INT_MIN`/`INT_MAX`).
- `coders/check_args.c`: ahora valida los 7 argumentos numéricos con `is_valid_number` antes de convertirlos con `atoi`.
- `coders/dongle.c` + `coders/dongle_acquire.c`: mismo motivo de split que el heap (10 funciones no cabían en un archivo). `dongle.c` tiene el ciclo de vida (`dongle_create/destroy/release`, `sim_is_stopped`, `get_now_ms`); `dongle_acquire.c` tiene la lógica de adquisición (`dongle_acquire`, `dongle_acquire_pair`, y sus helpers `compute_key`/`can_take_now`/`order_dongles`, estos sí `static` porque solo se usan ahí dentro).
- Cada `.c` compila individualmente con `-Wall -Wextra -Werror -pthread` sin avisos, y **`norminette coders/` da "OK!" en los 9 archivos**. **El binario todavía no enlaza**: `dongle_acquire.c` llama a `log_event`, que solo está declarada (en el header), no implementada. Eso es lo primero que desbloquea todo lo demás (fase 5).

### Decisiones de diseño ya tomadas (respétalas en el código que falta)

Estas decisiones están tomadas y el código de `dongle.c` depende de ellas. Si algo de esto no encaja con lo que tenías en mente, dímelo antes de seguir para no tener que deshacer trabajo:

1. **Arbitraje por dongle, no global.** Cada `t_dongle` tiene su propio `t_heap *waiting`. No existe un contador de llegada global en `t_sim`: el contador `arrival_counter` vive dentro de cada `t_dongle` y solo cuenta las solicitudes de ese dongle en concreto. Esto es coherente con el subject ("arbitraje entre solicitantes del mismo dongle").
2. **La clave del heap depende del scheduler**: en `fifo` es el contador de llegada del propio dongle; en `edf` es `last_compile_start_ms + time_to_burnout` del coder. Se decide dentro de `dongle_acquire`, comparando `coder->simulation->args->schedule` contra `"edf"`.
3. **Orden anti-deadlock**: siempre se adquiere primero el dongle con `id` más bajo. Es el mismo criterio para todos los coders, así que no puede haber espera circular.
4. **Caso `number_of_coders == 1`**: se asume que `coder->left` y `coder->right` apuntarán al mismo `t_dongle`. `dongle_acquire_pair` ya contempla esto (si `first == second`, solo adquiere una vez) — pero **quien llame a `dongle_release` para liberar el par (la rutina del coder, fase 6) tiene que aplicar el mismo criterio**: una sola llamada a `dongle_release` si son el mismo dongle, dos si son distintos. Si no se respeta esto, se libera dos veces el mismo mutex/cooldown y el comportamiento queda indefinido.
5. **`sim_is_stopped` es la única forma correcta de leer `stop_flag`** desde cualquier hilo que no sea el monitor. No leas `sim->stop_flag` directamente en ningún sitio nuevo: siempre a través de esta función (ya bloquea `mutex_stop_flag` por ti).
6. **El cooldown usa su propio reloj** (`gettimeofday` sin relacionarlo con `start_time_ms`). El timestamp que se loggea sí usa `start_time_ms` como base (eso lo hace `now_ms`, todavía no implementada). Son dos relojes con propósitos distintos, no los mezcles.
7. **`compute_key` para `fifo` incrementa el contador del dongle mientras el mutex de ese dongle ya está bloqueado** (lo hace `dongle_acquire` antes de llamarla). Si en algún sitio nuevo necesitas leer o tocar `arrival_counter`, hazlo siempre con ese mutex bloqueado.

---

## 1. Fase 5 — Logging serializado (`coders/log.c`)

Es el siguiente paso obligatorio: sin esto no enlaza el binario, porque `dongle.c` ya llama a `log_event`.

Necesitas dos funciones, ya declaradas en el header:
- Una que calcule el timestamp actual en milisegundos relativo al arranque de la simulación (la diferencia entre "ahora" y `sim->start_time_ms`).
- Una que imprima una línea de log para un evento de un coder concreto.

Puntos a decidir y respetar:
- Debe ser el **único punto del proyecto** donde se hace `printf`/`write` para los eventos de la simulación (aparte de los mensajes de error de argumentos, que van antes de crear la simulación).
- Tiene que estar protegido por `mutex_log`: lock, imprimir, unlock — nada más dentro de la sección crítica, para no retener el mutex más de lo necesario.
- Formato de línea exigido por el subject: el timestamp en ms, el identificador del coder, y el texto del evento.
- Eventos literales a usar tal cual los pide el subject: `"has taken a dongle"`, `"is compiling"`, `"is debugging"`, `"is refactoring"`, `"burned out"`. `dongle_acquire` ya usa el primero.
- Decide si el identificador del coder que se imprime es `id` 0-based o 1-based, y sé consistente en todo el proyecto (incluida la rutina del coder y el monitor, que loggearán el resto de eventos).
- Pruébalo con varios hilos lanzando `log_event` a la vez antes de seguir, para confirmar que ninguna línea sale cortada o mezclada con otra.

---

## 2. Fase 6 — Rutina del coder (`coders/coder_routine.c`)

Esta es la función que ejecuta cada hilo coder (la firma la exige `pthread_create`: recibe y devuelve `void *`, y el argumento real por dentro es un `t_coder *`).

Tiene que implementar el ciclo completo de una persona, repitiéndolo hasta que la simulación se pare:

1. Comprobar `sim_is_stopped` al principio de cada vuelta del ciclo — si ya está parada, terminar sin más.
2. Registrar el instante en el que empieza a intentar compilar, en `last_compile_start_ms`. **Recuerda**: este campo lo lee el monitor desde otro hilo, así que la escritura va protegida por `coder->mutex` (aunque la propia rutina, al ser el único escritor, no necesite el mutex para leerse a sí misma después).
3. Llamar a `dongle_acquire_pair` para conseguir ambos dongles. Si devuelve que la simulación se ha parado mientras esperaba, salir del ciclo sin loggear nada más de compilación.
4. Loggear `"is compiling"` y esperar `time_to_compile` ms.
5. Liberar los dos dongles con `dongle_release`, respetando el caso especial de `number_of_coders == 1` que se explica en el punto 0.4 de arriba.
6. Incrementar `compilations_done`, protegido también por `coder->mutex` (mismo motivo: el monitor lo lee).
7. Loggear `"is debugging"` y esperar `time_to_debug` ms.
8. Loggear `"is refactoring"` y esperar `time_to_refactor` ms.
9. Volver al paso 1.

Detalle importante a decidir tú: las esperas largas (`time_to_compile`, `time_to_debug`, `time_to_refactor`) probablemente no deberían ser un único `usleep` de golpe, porque si el monitor activa el flag de parada a mitad de esa espera, el hilo no se entera hasta que termine. Trocea la espera en fragmentos pequeños (por ejemplo, de 1 ms) comprobando `sim_is_stopped` entre fragmento y fragmento, para que la simulación pueda parar con rapidez razonable en cualquier momento.

---

## 3. Fase 7 — Hilo monitor (`coders/monitor.c`)

Un único hilo, separado de los coders, que en bucle:

1. Para cada coder, calcula si ha pasado más de `time_to_burnout` ms desde su `last_compile_start_ms` sin que haya **empezado** a compilar de nuevo. Esta lectura de `last_compile_start_ms` sí necesita el `coder->mutex` del coder correspondiente, porque aquí el monitor es un hilo distinto al que escribe el campo.
   - Si algún coder se ha agotado: loggear `"burned out"` para ese coder, **antes** de activar el flag de parada (el orden importa, lo pide el subject).
2. Comprueba también si **todos** los coders han alcanzado `number_of_compiles_required` compilaciones. Si es así, activa el flag de parada directamente, sin loggear "burned out" — es un final por éxito, no por agotamiento.
3. Al activar el flag de parada (por cualquiera de los dos motivos), tiene que despertar a todos los hilos que puedan estar bloqueados en un `pthread_cond_wait` sobre la condvar de **cualquier** dongle — no solo uno. Como cada dongle tiene su propia condvar, esto implica recorrer el array de dongles y hacer `pthread_cond_broadcast` sobre cada uno.
4. El sondeo tiene que ser lo bastante frecuente para detectar el burnout con un margen de 10 ms como mucho — piensa en un `usleep` del orden de 1 ms entre vuelta y vuelta del bucle, y ajusta con pruebas reales si hace falta más o menos precisión.
5. El propio monitor debe dejar de iterar en cuanto el flag de parada esté activo (por su propia comprobación o porque ya lo puso él mismo), para poder terminar y ser unido con `pthread_join`.

Cuidado con el orden de bloqueo de mutex aquí: si necesitas leer varios campos de varios coders en la misma vuelta, hazlo coder por coder (lock, lee, unlock) en vez de mantener varios mutex bloqueados a la vez — no hay ninguna razón para retenerlos simultáneamente y así te evitas cualquier riesgo de deadlock cruzado con los mutex de los dongles.

---

## 4. Fase 8 — `main.c` y `thread_creator.c` reales

Ahora mismo `main.c` solo valida argumentos y sale; `thread_creator.c` solo declara una función sin definirla. Aquí se junta todo:

1. Construir el struct raíz `t_sim`: reservar y rellenar el array de `N` dongles (con `dongle_create`, pasando `capacity = number_of_coders` a cada uno) y el array de `N` coders, inicializar `mutex_log`, `mutex_stop_flag`, `stop_flag = 0`, y `start_time_ms` (con `gettimeofday`, como referencia para `now_ms`).
2. Asignar a cada coder su `left`/`right` según su posición en la mesa circular a partir de su `id` y de `N` — decide tú la fórmula de indexación (típicamente algo como "el dongle a tu izquierda es el que tiene tu mismo índice, el de la derecha es el siguiente módulo N", pero revísalo con cuidado para el caso `N == 1`, que ya vimos que necesita `left == right`).
3. Lanzar los `N` hilos coder con `pthread_create`, pasando un puntero a cada `t_coder` (que a su vez apunta al `t_sim` raíz), y lanzar el hilo monitor con `pthread_create` pasando el `t_sim` raíz.
4. Hacer `pthread_join` de los `N` coders y del monitor, **todos, sin excepción**, antes de liberar nada.
5. Limpiar todo en orden inverso a como se creó: destruir cada dongle (`dongle_destroy`, que ya libera su heap interno), liberar los arrays de punteros, destruir los mutex propios de cada coder, destruir `mutex_log` y `mutex_stop_flag`, liberar el struct `t_sim` y el `t_args`.
6. Gestionar los caminos de error de inicialización: si algo falla a mitad de crear los dongles o los coders (por ejemplo un `malloc` que devuelve `NULL`), libera lo que ya se había reservado antes de salir, no dejes memoria colgada ni un `pthread_create` a medias sin su `join`.

Este es también el momento de poner nombre real a lo que hoy es solo un stub en `thread_creator.c` — decide tú si prefieres tener toda la creación de hilos en una función central ahí, o repartirla entre `main.c` y `thread_creator.c` (por ejemplo, una función que cree solo los coders y otra que cree el monitor, para no pasarte del límite de líneas de Norma en una única función).

---

## 5. Fase 9 — Makefile

No existe todavía. Va dentro de `coders/`, con:
- Una variable `NAME` con el nombre del binario.
- Reglas `all`, `$(NAME)`, `clean` (borra los `.o`), `fclean` (borra también el binario), `re` (`fclean` + `all`).
- Compilador `cc`, flags `-Wall -Wextra -Werror -pthread`.
- Compilación incremental: que un `make` después de tocar un solo `.c` no recompile todos los demás, y que tocar `codexion.h` sí invalide todos los objetos (todos los `.c` dependen de él).
- Si en algún momento añades bonus, una regla `bonus` aparte que compile archivos `*_bonus.c/.h` independientes de la parte obligatoria.

Verifica `make`, `make clean`, `make fclean` y `make re` ejecutados desde dentro de `coders/`.

---

## 6. Fase 10 — Plan de pruebas

Antes de dar nada por terminado, prueba al menos estos escenarios (con `cd coders && make re` primero):

- **Caso trivial `N=1`**: debería agotarse, documenta en el README por qué es el comportamiento esperado.
- **Caso pequeño viable en `fifo`**: comprobar que todos llegan a `number_of_compiles_required` sin que nadie se queme.
- **El mismo caso en `edf`**: comparar comportamiento contra `fifo`, no debería quemarse nadie si los parámetros son viables.
- **Caso no viable** (burnout muy ajustado frente al resto de tiempos): comprobar que aparece `"burned out"` en el log, y que el timestamp de ese evento está dentro del margen de 10 ms respecto al momento real en que debería haberse detectado.
- **Contención real**: `number_of_coders` alto (10-20) con tiempos muy cortos, para forzar que varios coders compitan de verdad por los mismos dongles.
- **Memoria**: `valgrind --leak-check=full --show-reachable=yes` sobre el caso viable — cero leaks, cero errores.
- **Concurrencia**: una build aparte con `-fsanitize=thread` (no se entrega, es solo para depurar) sobre el caso de contención alta — cero data races reportadas. `helgrind` es una alternativa si prefieres Valgrind también para esto.

---

## 7. Fase 11 — Norma

Antes de entregar, pasa el checker de Norma de tu campus sobre todos los `.c`/`.h` de `coders/` y confirma cero errores. Con el código ya escrito hasta ahora no debería haber sorpresas (se ha ido comprobando línea/parámetros/variables al escribirlo), pero presta atención especial en el código que aún falta a:
- Funciones del monitor y de `coder_routine`, que tienden a acumular muchas comprobaciones seguidas — extrae condiciones complejas a funciones auxiliares que devuelvan `int`, como ya se hizo en `dongle.c` con `can_take_now`.
- El número de variables declaradas por función en `main.c`/`thread_creator.c`, donde la tentación de declarar muchos punteros locales de golpe es alta.

---

## 8. Fase 12 — README.md (raíz del repo)

No existe todavía. Tiene que estar en la **raíz** del repo (no dentro de `coders/`), con estas secciones en este orden:

1. Primera línea en cursiva: la atribución exigida por el subject sobre el currículo de 42.
2. **Descripción** del proyecto.
3. **Instrucciones**: cómo compilar (`cd coders && make`) y cómo ejecutar, con un ejemplo de comando real.
4. **Recursos**: qué documentación has consultado, y una descripción honesta de en qué partes has usado asistencia de IA y para qué (esta misma sesión de trabajo con Claude Code entra aquí — sé específico sobre qué se generó con ayuda y qué no).
5. **Blocking cases handled**: cómo evitas deadlock (el criterio de orden por `id` ya implementado en `dongle.c`), starvation, cómo gestionas el cooldown, cómo detectas el agotamiento con precisión, cómo serializas el log.
6. **Thread synchronization mechanisms**: qué mutex/condvar usas y para qué (puedes basarte literalmente en la tabla de la sección 0 de este documento), cómo evitas race conditions, cómo se comunican coders y monitor de forma thread-safe.

---

## 9. Fase 13 — Entrega final (checklist)

- [ ] `coders/` contiene exactamente `Makefile`, `*.c`, `*.h` (más `*_bonus.{c,h}` si hay bonus, en archivos separados de la parte obligatoria).
- [ ] `README.md` en la raíz, no dentro de `coders/`.
- [ ] `cd coders && make re` compila limpio, sin warnings, con `-Wall -Wextra -Werror -pthread`.
- [ ] Norma sin errores en todos los archivos.
- [ ] `valgrind` sin leaks, TSan/helgrind sin data races.
- [ ] `git status` limpio, todo commiteado y pusheado a `origin/main`.
- [ ] Repasa el capítulo "Recode instructions" del subject: en la defensa te pueden pedir modificar algo en vivo — asegúrate de poder explicar y tocar cualquier parte del código (incluido lo que se ha escrito con ayuda en esta sesión) sin depender de este documento.

---

## 10. Orden recomendado a partir de aquí

1. `log.c` (fase 1 de este documento) — desbloquea el enlazado del binario.
2. `coder_routine.c` (fase 2).
3. `monitor.c` (fase 3).
4. `main.c` + `thread_creator.c` reales (fase 4).
5. Primeras pruebas manuales con los comandos de la fase 6, empezando por `fifo` antes que `edf`.
6. `Makefile` (fase 5) si no lo has ido necesitando ya para las pruebas anteriores (probablemente sí, muévelo antes si te hace falta compilar según trabajas).
7. Pruebas completas (fase 6), incluidas valgrind y TSan.
8. Norma (fase 7).
9. README.md (fase 8).
10. Entrega (fase 9).
