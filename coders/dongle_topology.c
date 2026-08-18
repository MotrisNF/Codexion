/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   dongle_topology.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/17 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/18 09:17:58 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

t_coder	*other_side(t_sim *sim, t_dongle *dongle, t_coder *coder)
{
	int	i;

	i = 0;
	while (i < sim->args->number_of_coders)
	{
		if (sim->coders[i] != coder && (sim->coders[i]->left == dongle
				|| sim->coders[i]->right == dongle))
			return (sim->coders[i]);
		i++;
	}
	return (NULL);
}

t_dongle	*partner_dongle(t_coder *coder, t_dongle *shared)
{
	if (coder->left == shared)
		return (coder->right);
	return (coder->left);
}

void	order_dongles(t_coder *coder, t_dongle **lo, t_dongle **hi)
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
