/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_acquire.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/18 09:17:46 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long	compute_key(t_dongle *dongle, t_coder *coder)
{
	if (strcmp(coder->simulation->args->schedule, "edf") == 0)
		return ((coder->last_compile_start_ms
				+ coder->simulation->args->time_to_burnout) * 1000L
			+ coder->id);
	return (dongle->arrival_counter++);
}

long	dongle_wait_hint(t_dongle *dongle, long now)
{
	long	wait_ms;

	wait_ms = dongle->aviable_at_ms - now;
	if (wait_ms <= 0)
		wait_ms = 5;
	return (wait_ms);
}

int	dongle_acquire_pair(t_coder *coder)
{
	if (strcmp(coder->simulation->args->schedule, "edf") == 0)
		return (dongle_acquire_pair_edf(coder));
	return (dongle_acquire_pair_fifo(coder));
}
