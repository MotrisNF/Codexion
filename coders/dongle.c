/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle.c                                           :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 09:17:07 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

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
