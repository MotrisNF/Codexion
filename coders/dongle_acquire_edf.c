/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_acquire_edf.c                               :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/17 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

/*
** EDF: the subject requires that nobody burns out under EDF while the
** parameters stay viable. Both dongles of a pair are grabbed together:
** either both are genuinely free right now or neither is taken, so
** nobody holds a dongle hostage while stuck on the other one.
*/
static void	dongle_enqueue(t_dongle *dongle, t_coder *coder)
{
	long	key;

	pthread_mutex_lock(&dongle->mutex);
	key = compute_key(dongle, coder);
	heap_push(dongle->waiting, key, coder->id);
	pthread_mutex_unlock(&dongle->mutex);
}

/*
** `dongle` is ready for `coder` if it's physically free AND no rival
** with strictly better priority is both waiting for it and genuinely
** able to use it right now. A rival that merely arrived first but is
** still stuck on its own other dongle does not get to sit on this one
** unused - that was wasting real throughput.
*/
static int	edf_dongle_ready(t_sim *sim, t_dongle *dongle, t_coder *coder,
	long now)
{
	t_coder		*rival;
	t_dongle	*rival_other;
	long		my_key;
	long		rival_key;

	if (dongle->taked || now < dongle->aviable_at_ms)
		return (0);
	if (!heap_find(dongle->waiting, coder->id, &my_key))
		return (0);
	rival = other_side(sim, dongle, coder);
	if (!rival || !heap_find(dongle->waiting, rival->id, &rival_key))
		return (1);
	if (rival_key >= my_key)
		return (1);
	rival_other = partner_dongle(rival, dongle);
	return (rival_other->taked || now < rival_other->aviable_at_ms);
}

/*
** A release on either lo or hi broadcasts sim->cond_progress (see
** dongle_release). Waiting on that shared signal instead of picking
** just one of the two dongle-specific conds means we never have to
** guess which one to listen to - we just wake up on any relevant
** change and re-check both.
*/
static void	wait_for_change(t_sim *sim, t_dongle *lo, t_dongle *hi)
{
	struct timespec	deadline;
	long			now;
	long			wait_ms;

	now = get_now_ms();
	wait_ms = dongle_wait_hint(lo, now);
	if (dongle_wait_hint(hi, now) < wait_ms)
		wait_ms = dongle_wait_hint(hi, now);
	pthread_mutex_lock(&sim->mutex_progress);
	deadline_in_ms(wait_ms, &deadline);
	pthread_cond_timedwait(&sim->cond_progress, &sim->mutex_progress,
		&deadline);
	pthread_mutex_unlock(&sim->mutex_progress);
}

static int	try_take_pair(t_sim *sim, t_coder *coder, t_dongle *lo,
	t_dongle *hi)
{
	t_dongle	*pair[2];
	t_dongle	*set[4];
	int			count;
	int			ok;

	pair[0] = lo;
	pair[1] = hi;
	count = build_lock_set(sim, coder, pair, set);
	sort_by_id(set, count);
	lock_set(set, count, 1);
	ok = edf_dongle_ready(sim, lo, coder, get_now_ms())
		&& edf_dongle_ready(sim, hi, coder, get_now_ms());
	if (ok)
	{
		heap_remove_id(lo->waiting, coder->id);
		heap_remove_id(hi->waiting, coder->id);
		lo->taked = 1;
		hi->taked = 1;
	}
	lock_set(set, count, 0);
	return (ok);
}

int	dongle_acquire_pair_edf(t_coder *coder)
{
	t_sim		*sim;
	t_dongle	*lo;
	t_dongle	*hi;

	sim = coder->simulation;
	order_dongles(coder, &lo, &hi);
	if (lo == hi)
	{
		while (!sim_is_stopped(sim))
			usleep(1000);
		return (0);
	}
	dongle_enqueue(lo, coder);
	dongle_enqueue(hi, coder);
	while (!sim_is_stopped(sim))
	{
		if (try_take_pair(sim, coder, lo, hi))
		{
			log_event(sim, coder->id, "has taken a dongle");
			log_event(sim, coder->id, "has taken a dongle");
			return (1);
		}
		wait_for_change(sim, lo, hi);
	}
	return (0);
}
