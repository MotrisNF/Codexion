/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor_utils.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/17 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	coder_burned_out(t_coder *coder, int burnout, long now)
{
	int	timed_out;

	pthread_mutex_lock(&coder->mutex);
	timed_out = ((now - coder->last_compile_start_ms) >= burnout);
	pthread_mutex_unlock(&coder->mutex);
	return (timed_out);
}

int	coder_finished(t_coder *coder, int required)
{
	int	done;

	pthread_mutex_lock(&coder->mutex);
	done = (coder->compilations_done >= required);
	pthread_mutex_unlock(&coder->mutex);
	return (done);
}

long	next_burnout_deadline(t_sim *sim, long now)
{
	int		i;
	long	deadline;
	long	best;

	i = 0;
	best = now;
	while (i < sim->args->number_of_coders)
	{
		pthread_mutex_lock(&sim->coders[i]->mutex);
		deadline = sim->coders[i]->last_compile_start_ms
			+ sim->args->time_to_burnout;
		pthread_mutex_unlock(&sim->coders[i]->mutex);
		if (i == 0 || deadline < best)
			best = deadline;
		i++;
	}
	if (best < now)
		best = now;
	return (best);
}
