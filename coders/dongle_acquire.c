/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_acquire.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

//Clave para el heap de espera de ESTE dongle. En fifo es un
//contador de llegada propio del dongle (arbitraje es por dongle,
//no global); en edf es el deadline del coder. Se llama con el
//mutex del dongle ya bloqueado, asi que el ++ es seguro.
static long	compute_key(t_dongle *dongle, t_coder *coder)
{
	if (strcmp(coder->simulation->args->schedule, "edf") == 0)
		return (coder->last_compile_start_ms
			+ coder->simulation->args->time_to_burnout);
	return (dongle->arrival_counter++);
}

//Condicion completa que hay que revalidar cada vez que el hilo se
//despierta de pthread_cond_wait: libre, cooldown pasado, y que el
//propio coder sea la clave minima del heap de espera (su turno).
static int	can_take_now(t_dongle *dongle, t_coder *coder, long now)
{
	if (dongle->taked)
		return (0);
	if (now < dongle->aviable_at_ms)
		return (0);
	if (heap_is_empty(dongle->waiting))
		return (0);
	if (dongle->waiting->nodes[0].coder_id != coder->id)
		return (0);
	return (1);
}

//Solucion "asimetrica" clasica del problema de los filosofos:
//coders de id par piden primero su dongle izquierdo, los de id
//impar piden primero el derecho. Esto evita el deadlock circular
//igual que "siempre el id de dongle mas bajo primero", pero reparte
//mejor la contencion: con esa otra regla, casi todos acaban pidiendo
//primero su propio dongle izquierdo (porque coincide con ser el id
//mas bajo) y solo compiten de verdad por el segundo, dejando que
//solo 1 de cada ronda consiga el par completo en vez de los ~N/2
//que caben en la mesa. Con paridad, cada dongle de indice par pasa
//a ser una contienda real entre dos coders, lo que permite que se
//formen varias parejas a la vez con mas frecuencia.
//Por que sigue sin haber deadlock: para que los N coders se
//bloqueasen en circulo, los N tendrian que sostener cada uno
//exactamente su primer dongle a la vez sin que nadie mas lo
//reclamara. Con esta regla, todo dongle de indice par SIEMPRE tiene
//dos coders queriendolo como primera opcion (su dueño par, que lo
//pide como izquierdo, y su vecino impar, que lo pide como derecho),
//asi que nunca pueden los dos sostenerlo en el mismo instante: al
//menos uno de ellos se queda sin conseguir siquiera su primer
//dongle, así que la mesa nunca puede terminar con los N dongles
//repartidos uno-a-uno y todos esperando el segundo.
static void	order_dongles(t_coder *coder, t_dongle **first, t_dongle **second)
{
	if (coder->id % 2 == 0)
	{
		*first = coder->left;
		*second = coder->right;
	}
	else
	{
		*first = coder->right;
		*second = coder->left;
	}
}

//Se registra en el heap de espera del dongle y duerme en la condvar
//hasta que le toque (can_take_now) o la simulacion se pare. Usa
//timedwait (no un cond_wait indefinido) porque el fin del cooldown
//no dispara ningun broadcast por si mismo: sin el limite de tiempo,
//un coder que se despierta y ve que el cooldown no ha pasado podria
//quedarse dormido para siempre. El while (no if) revalida siempre
//la condicion completa, tanto por el timeout como por los wakeups
//espurios y porque solo hay broadcast (no signal): se despierta a
//todos los que esperan ese dongle y cada uno comprueba si le toca.
int	dongle_acquire(t_dongle *dongle, t_coder *coder)
{
	long			key;
	struct timespec	deadline;

	pthread_mutex_lock(&dongle->mutex);
	key = compute_key(dongle, coder);
	heap_push(dongle->waiting, key, coder->id);
	while (!can_take_now(dongle, coder, get_now_ms())
		&& !sim_is_stopped(coder->simulation))
	{
		deadline_in_ms(5, &deadline);
		pthread_cond_timedwait(&dongle->cond, &dongle->mutex, &deadline);
	}
	if (sim_is_stopped(coder->simulation))
	{
		pthread_mutex_unlock(&dongle->mutex);
		return (0);
	}
	heap_pop_min(dongle->waiting);
	dongle->taked = 1;
	pthread_mutex_unlock(&dongle->mutex);
	log_event(coder->simulation, coder->id, "has taken a dongle");
	return (1);
}

//Caso number_of_coders == 1: left y right apuntan al mismo dongle,
//asi que "first == second" y solo hace falta adquirirlo una vez.
//Quien llame a esta funcion (coder_routine, siguiente paso) tiene
//que aplicar el mismo criterio al liberar: una sola llamada a
//dongle_release si first == second, dos si son distintos.
int	dongle_acquire_pair(t_coder *coder)
{
	t_dongle	*first;
	t_dongle	*second;

	order_dongles(coder, &first, &second);
	if (!dongle_acquire(first, coder))
		return (0);
	if (first == second)
		return (1);
	if (!dongle_acquire(second, coder))
	{
		dongle_release(first, coder);
		return (0);
	}
	return (1);
}
