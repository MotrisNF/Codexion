/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_acquire.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 09:16:41 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static long	compute_key(t_dongle *dongle, t_coder *coder)
{
	if (strcmp(coder->simulation->args->schedule, "edf") == 0)
		return (coder->last_compile_start_ms
			+ coder->simulation->args->time_to_burnout);
	return (dongle->arrival_counter++);
}

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

int	dongle_acquire_pair(t_coder *coder)
{
	t_dongle	*first;
	t_dongle	*second;

	order_dongles(coder, &first, &second);
	if (!dongle_acquire(first, coder))
		return (0);
	if (!dongle_acquire(second, coder))
	{
		dongle_release(first, coder);
		return (0);
	}
	return (1);
}
