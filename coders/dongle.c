/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

//Ciclo de vida del dongle y utilidades basicas. La logica de
//adquisicion (dongle_acquire/dongle_acquire_pair) vive en
//dongle_acquire.c, por limite de funciones por archivo de la Norma.

//Reloj propio para el cooldown del dongle, independiente del
//start_time_ms de la simulacion (ese es solo para los timestamps
//del log). gettimeofday es la unica funcion de tiempo autorizada.
//No static: tambien la usa dongle_acquire.c.
long	get_now_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long)tv.tv_sec * 1000 + (long)tv.tv_usec / 1000);
}

//Lectura protegida del flag de parada: la escribe el monitor, la
//leen los coders (incluido dentro de dongle_acquire) desde otro
//hilo, asi que siempre pasa por mutex_stop_flag.
int	sim_is_stopped(t_sim *sim)
{
	int	stopped;

	pthread_mutex_lock(&sim->mutex_stop_flag);
	stopped = sim->stop_flag;
	pthread_mutex_unlock(&sim->mutex_stop_flag);
	return (stopped);
}

t_dongle	*dongle_create(int id, int capacity)
{
	t_dongle	*dongle;

	dongle = malloc(sizeof(t_dongle));
	if (!dongle)
		return (NULL);
	dongle->waiting = heap_create(capacity);
	if (!dongle->waiting)
		return (free(dongle), NULL);
	dongle->id = id;
	dongle->taked = 0;
	dongle->aviable_at_ms = 0;
	dongle->arrival_counter = 0;
	pthread_mutex_init(&dongle->mutex, NULL);
	pthread_cond_init(&dongle->cond, NULL);
	return (dongle);
}

void	dongle_destroy(t_dongle *dongle)
{
	if (!dongle)
		return ;
	heap_destroy(dongle->waiting);
	pthread_mutex_destroy(&dongle->mutex);
	pthread_cond_destroy(&dongle->cond);
	free(dongle);
}

//Libera el dongle, fija el cooldown y despierta (broadcast, no hay
//signal) a todo el que estuviera esperando en su condvar. El
//broadcast va fuera del lock: el estado ya quedo actualizado antes
//de soltar el mutex, y asi no se despierta a nadie contra un mutex
//que seguimos reteniendo nosotros mismos.
void	dongle_release(t_dongle *dongle, t_coder *coder)
{
	t_args	*args;

	args = coder->simulation->args;
	pthread_mutex_lock(&dongle->mutex);
	dongle->taked = 0;
	dongle->aviable_at_ms = get_now_ms() + args->dongle_cooldown;
	pthread_mutex_unlock(&dongle->mutex);
	pthread_cond_broadcast(&dongle->cond);
}
