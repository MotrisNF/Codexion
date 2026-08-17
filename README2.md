## Elegir parámetros viables

El enunciado garantiza que, bajo el planificador `edf`, nadie debe agotarse
*"siempre que los parámetros sean viables"*. Esta sección explica qué
significa "viable" en números concretos, para poder elegir (o generar)
argumentos de prueba con la certeza de que el programa debería completarlos
sin agotamientos.

### 1. Cota dura: el mínimo para que todo el mundo compile una vez

Con `number_of_coders` (`n`) personas sentadas en el hub circular, solo
`floor(n / 2)` pueden compilar a la vez (cada compilación necesita 2 dongles
adyacentes, y dos personas solo pueden compilar en paralelo si no comparten
ninguno). El tiempo mínimo posible, con el mejor reparto imaginable, para que
**todas** las personas compilen al menos una vez es:

```
tiempo_mínimo = ceil(n / floor(n / 2)) × time_to_compile
```

Si `time_to_burnout` es menor o igual que este valor, el agotamiento de
alguien es matemáticamente inevitable, sin importar cómo de bueno sea el
planificador. Por debajo de esta cota no hay forma de que el programa evite
el agotamiento — no es un fallo de implementación, es un límite del propio
problema.

### 2. Sostenibilidad a largo plazo

La cota anterior solo garantiza la **primera** ronda de compilaciones. Si la
simulación tiene que sostenerse durante muchas rondas (`number_of_compiles_required`
alto), también hay que evitar que el sistema esté saturado a largo plazo. Sin
contención, cada persona querría compilar una fracción
`time_to_compile / (time_to_compile + time_to_debug + time_to_refactor)`
del tiempo total. Esa fracción tiene que caber en la capacidad real
disponible:

```
time_to_compile / (time_to_compile + time_to_debug + time_to_refactor)  ≤  floor(n / 2) / n
```

Si esta desigualdad no se cumple, la demanda de compilación supera lo que el
sistema puede servir de forma sostenida: la cola de espera crece sin límite
con el tiempo, y ningún valor de `time_to_burnout`, por grande que sea,
evita el agotamiento eventual en una simulación suficientemente larga.

### Margen recomendado

Cumplir ambas condiciones al límite exacto (margen 0) es frágil: cualquier
`dongle_cooldown` no nulo, o el jitter normal de la planificación del
sistema operativo, puede bastar para tumbarlo. Se recomienda un
`time_to_burnout` con al menos un 50% de margen sobre la cota dura del
punto 1, además de cumplir la desigualdad de sostenibilidad del punto 2.

### Ejemplos con los valores mínimos que acepta el programa

El programa exige `number_of_coders ≥ 1`, `number_of_compiles_required ≥ 1`,
y el resto de tiempos `≥ 0` (ver `check_args.c`).

**Ejemplo 1 — mínimo absoluto (`n = 1`):**

```
./codexion 1 0 0 0 0 1 0 fifo
```

Con una sola persona, `floor(1 / 2) = 0`: no hay compilación posible en
ningún caso (hacen falta 2 dongles simultáneos y solo hay 1 sobre la mesa).
La fórmula del punto 1 no se puede aplicar (división por `floor(n/2)=0`) —
es un caso degenerado a propósito: la persona se agota siempre,
inmediatamente, sin llegar nunca a compilar. Salida real:

```
1 1 has taken a dongle
1 1 burned out
```

**Ejemplo 2 — el `n` más pequeño en el que sí se puede compilar (`n = 2`):**

Con `n = 2`, `floor(2/2) = 1` y la cota dura es
`ceil(2/1) × time_to_compile = 2 × time_to_compile`.

Con `time_to_compile = 10`, el mínimo teórico es `20`. Usar exactamente ese
valor como `time_to_burnout` (margen 0) falla siempre:

```
./codexion 2 20 10 0 0 3 0 fifo
# 10/10 tiradas terminan en "burned out"
```

Añadiendo el margen recomendado (`time_to_burnout = 30`, un 50% por encima
del mínimo) la simulación se completa siempre sin agotamientos:

```
./codexion 2 30 10 0 0 3 0 fifo
# 0/10 tiradas terminan en "burned out"
```

### Límites para otros valores de `n`

Aplicando la misma fórmula (`ceil(n / floor(n/2))` para la cota dura,
`floor(n/2) / n` para la fracción de sostenibilidad):

| `n` | `floor(n/2)` | cota dura                  | fracción de sostenibilidad |
|-----|--------------|-----------------------------|-----------------------------|
| 1   | 0            | no aplica (nunca compila)   | 0                           |
| 2   | 1            | `2 × time_to_compile`       | `1/2 = 0.50`                |
| 3   | 1            | `3 × time_to_compile`       | `1/3 ≈ 0.33`                |
| 5   | 2            | `3 × time_to_compile`       | `2/5 = 0.40`                |
