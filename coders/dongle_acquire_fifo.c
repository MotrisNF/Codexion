/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_acquire_fifo.c                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/17 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/*
** FIFO: each dongle is acquired independently, first-come-first-served
** on that dongle. Simple sequential acquisition is enough here, since
** FIFO fairness is defined per-resource and does not need cross-resource
** priority visibility.
*/
static int	dongle_ready(t_dongle *dongle, t_coder *coder, long now)
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

static void	compute_wait_deadline(t_dongle *dongle, struct timespec *out)
{
	deadline_in_ms(dongle_wait_hint(dongle, get_now_ms()), out);
}

static int	dongle_acquire_one(t_dongle *dongle, t_coder *coder)
{
	long			key;
	struct timespec	deadline;

	pthread_mutex_lock(&dongle->mutex);
	key = compute_key(dongle, coder);
	heap_push(dongle->waiting, key, coder->id);
	while (!dongle_ready(dongle, coder, get_now_ms())
		&& !sim_is_stopped(coder->simulation))
	{
		compute_wait_deadline(dongle, &deadline);
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

/*
** The acquisition order alternates by parity (classic dining-philosophers
** fix): if everybody grabbed their left dongle first, the whole ring
** could deadlock with each coder holding one dongle and waiting on the
** next one's.
*/
static void	order_dongles_fifo(t_coder *coder, t_dongle **first,
	t_dongle **second)
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

int	dongle_acquire_pair_fifo(t_coder *coder)
{
	t_dongle	*first;
	t_dongle	*second;

	order_dongles_fifo(coder, &first, &second);
	if (!dongle_acquire_one(first, coder))
		return (0);
	if (!dongle_acquire_one(second, coder))
	{
		dongle_release(first, coder);
		return (0);
	}
	return (1);
}
