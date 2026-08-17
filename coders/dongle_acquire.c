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

static long	dongle_wait_hint(t_dongle *dongle, long now)
{
	long	wait_ms;

	wait_ms = dongle->aviable_at_ms - now;
	if (wait_ms <= 0)
		wait_ms = 5;
	return (wait_ms);
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

static int	dongle_acquire_pair_fifo(t_coder *coder)
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

/*
** EDF: the subject requires that nobody burns out under EDF while the
** parameters stay viable. Acquiring the two dongles sequentially would
** let a coder hold priority on one dongle while stuck waiting on the
** other, wasting it for everybody else. So both dongles are grabbed
** atomically: either both are free and we take both, or we take
** neither and wait. Locking is always done in a fixed global order
** (lower dongle id first) to avoid deadlock.
*/
static void	order_dongles(t_coder *coder, t_dongle **lo, t_dongle **hi)
{
	if (coder->left->id < coder->right->id)
	{
		*lo = coder->left;
		*hi = coder->right;
	}
	else
	{
		*lo = coder->right;
		*hi = coder->left;
	}
}

static void	dongle_enqueue(t_dongle *dongle, t_coder *coder)
{
	long	key;

	pthread_mutex_lock(&dongle->mutex);
	key = compute_key(dongle, coder);
	heap_push(dongle->waiting, key, coder->id);
	pthread_mutex_unlock(&dongle->mutex);
}

static void	wait_for_change(t_dongle *lo, t_dongle *hi)
{
	struct timespec	deadline;
	t_dongle		*target;
	long			now;

	now = get_now_ms();
	target = lo;
	if (dongle_wait_hint(hi, now) < dongle_wait_hint(lo, now))
		target = hi;
	pthread_mutex_lock(&target->mutex);
	deadline_in_ms(dongle_wait_hint(target, get_now_ms()), &deadline);
	pthread_cond_timedwait(&target->cond, &target->mutex, &deadline);
	pthread_mutex_unlock(&target->mutex);
}

static int	dongle_acquire_pair_edf(t_coder *coder)
{
	t_dongle	*lo;
	t_dongle	*hi;

	order_dongles(coder, &lo, &hi);
	if (lo == hi)
	{
		while (!sim_is_stopped(coder->simulation))
			usleep(1000);
		return (0);
	}
	dongle_enqueue(lo, coder);
	dongle_enqueue(hi, coder);
	while (!sim_is_stopped(coder->simulation))
	{
		pthread_mutex_lock(&lo->mutex);
		pthread_mutex_lock(&hi->mutex);
		if (dongle_ready(lo, coder, get_now_ms())
			&& dongle_ready(hi, coder, get_now_ms()))
		{
			heap_pop_min(lo->waiting);
			heap_pop_min(hi->waiting);
			lo->taked = 1;
			hi->taked = 1;
			pthread_mutex_unlock(&hi->mutex);
			pthread_mutex_unlock(&lo->mutex);
			log_event(coder->simulation, coder->id, "has taken a dongle");
			log_event(coder->simulation, coder->id, "has taken a dongle");
			return (1);
		}
		pthread_mutex_unlock(&hi->mutex);
		pthread_mutex_unlock(&lo->mutex);
		wait_for_change(lo, hi);
	}
	return (0);
}

int	dongle_acquire_pair(t_coder *coder)
{
	if (strcmp(coder->simulation->args->schedule, "edf") == 0)
		return (dongle_acquire_pair_edf(coder));
	return (dongle_acquire_pair_fifo(coder));
}
