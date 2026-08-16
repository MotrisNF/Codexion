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

//Decide el orden de adquisicion por id de dongle (el mas pequeno
//primero), igual para todos los coders: rompe la simetria clasica
//del problema de los filosofos y evita el deadlock circular.
static void	order_dongles(t_coder *coder, t_dongle **first, t_dongle **second)
{
	if (coder->left->id < coder->right->id)
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
//hasta que le toque (can_take_now) o la simulacion se pare. Ojo:
//el while, no if, es obligatorio por los wakeups espurios y porque
//solo hay broadcast (no signal) disponible: se despierta a todos
//los que esperan ese dongle y cada uno tiene que revalidar si le
//toca de verdad a el.
int	dongle_acquire(t_dongle *dongle, t_coder *coder)
{
	long	key;

	pthread_mutex_lock(&dongle->mutex);
	key = compute_key(dongle, coder);
	heap_push(dongle->waiting, key, coder->id);
	while (!can_take_now(dongle, coder, get_now_ms())
		&& !sim_is_stopped(coder->simulation))
		pthread_cond_wait(&dongle->cond, &dongle->mutex);
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
