/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_lock_set.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/17 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/18 09:17:50 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	add_unique(t_dongle **set, int count, t_dongle *d)
{
	int	i;

	if (!d)
		return (count);
	i = 0;
	while (i < count)
	{
		if (set[i] == d)
			return (count);
		i++;
	}
	set[count] = d;
	return (count + 1);
}

int	build_lock_set(t_sim *sim, t_coder *coder, t_dongle **pair,
	t_dongle **set)
{
	t_coder	*rival_lo;
	t_coder	*rival_hi;
	int		count;

	count = add_unique(set, 0, pair[0]);
	count = add_unique(set, count, pair[1]);
	rival_lo = other_side(sim, pair[0], coder);
	if (rival_lo)
		count = add_unique(set, count, partner_dongle(rival_lo, pair[0]));
	rival_hi = other_side(sim, pair[1], coder);
	if (rival_hi)
		count = add_unique(set, count, partner_dongle(rival_hi, pair[1]));
	return (count);
}

void	sort_by_id(t_dongle **set, int count)
{
	int			i;
	int			j;
	t_dongle	*tmp;

	i = 1;
	while (i < count)
	{
		j = i;
		while (j > 0 && set[j - 1]->id > set[j]->id)
		{
			tmp = set[j - 1];
			set[j - 1] = set[j];
			set[j] = tmp;
			j--;
		}
		i++;
	}
}

void	lock_set(t_dongle **set, int count, int lock)
{
	int	i;

	if (lock)
	{
		i = 0;
		while (i < count)
			pthread_mutex_lock(&set[i++]->mutex);
	}
	else
	{
		i = count - 1;
		while (i >= 0)
			pthread_mutex_unlock(&set[i--]->mutex);
	}
}
