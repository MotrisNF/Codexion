/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_builders.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 09:19:13 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	free_dongles(t_dongle **dongles, int count)
{
	int	i;

	i = 0;
	while (i < count)
	{
		dongle_destroy(dongles[i]);
		i++;
	}
	free(dongles);
}

t_dongle	**create_dongles(int n)
{
	t_dongle	**dongles;
	int			i;

	dongles = malloc(sizeof(t_dongle *) * n);
	if (!dongles)
		return (NULL);
	i = 0;
	while (i < n)
	{
		dongles[i] = dongle_create(i, n);
		if (!dongles[i])
			return (free_dongles(dongles, i), NULL);
		i++;
	}
	return (dongles);
}

void	free_coders(t_coder **coders, int count)
{
	int	i;

	i = 0;
	while (i < count)
	{
		pthread_mutex_destroy(&coders[i]->mutex);
		free(coders[i]);
		i++;
	}
	free(coders);
}

t_coder	**create_coders(t_sim *sim)
{
	t_coder	**coders;
	int		n;
	int		i;

	n = sim->args->number_of_coders;
	coders = malloc(sizeof(t_coder *) * n);
	if (!coders)
		return (NULL);
	i = 0;
	while (i < n)
	{
		coders[i] = malloc(sizeof(t_coder));
		if (!coders[i])
			return (free_coders(coders, i), NULL);
		coders[i]->id = i + 1;
		coders[i]->thread_launched = 0;
		coders[i]->left = sim->dongles[i];
		coders[i]->right = sim->dongles[(i + 1) % n];
		coders[i]->last_compile_start_ms = get_now_ms();
		coders[i]->compilations_done = 0;
		coders[i]->simulation = sim;
		pthread_mutex_init(&coders[i]->mutex, NULL);
		i++;
	}
	return (coders);
}
