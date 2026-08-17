/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 13:12:17 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 09:18:33 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	destroy_sim(t_sim *sim)
{
	int	i;
	int	n;

	n = sim->args->number_of_coders;
	i = 0;
	while (i < n)
	{
		pthread_mutex_destroy(&sim->coders[i]->mutex);
		free(sim->coders[i]);
		dongle_destroy(sim->dongles[i]);
		i++;
	}
	free(sim->coders);
	free(sim->dongles);
	pthread_mutex_destroy(&sim->mutex_log);
	pthread_mutex_destroy(&sim->mutex_stop_flag);
	free(sim);
}

int	main(int argc, char **argv)
{
	t_args	*args;
	t_sim	*sim;
	int		launched_ok;

	args = cheack_args(argv, argc);
	if (!args)
		return (printf("Use the correct format.\n"), 1);
	sim = build_sim(args);
	if (!sim)
		return (free(args), printf("Allocation failed.\n"), 1);
	launched_ok = launch_threads(sim);
	join_threads(sim);
	destroy_sim(sim);
	free(args);
	if (!launched_ok)
		return (printf("Thread creation failed.\n"), 1);
	return (0);
}
