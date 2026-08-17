/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   thread_creator.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 15:21:26 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 09:19:51 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	destroy_sim_mutexes(t_sim *sim)
{
	pthread_mutex_destroy(&sim->mutex_log);
	pthread_mutex_destroy(&sim->mutex_stop_flag);
	free(sim);
}

t_sim	*build_sim(t_args *args)
{
	t_sim	*sim;

	sim = malloc(sizeof(t_sim));
	if (!sim)
		return (NULL);
	sim->args = args;
	sim->stop_flag = 0;
	sim->monitor_launched = 0;
	sim->start_time_ms = get_now_ms();
	pthread_mutex_init(&sim->mutex_log, NULL);
	pthread_mutex_init(&sim->mutex_stop_flag, NULL);
	sim->dongles = create_dongles(args->number_of_coders);
	if (!sim->dongles)
		return (destroy_sim_mutexes(sim), NULL);
	sim->coders = create_coders(sim);
	if (!sim->coders)
	{
		free_dongles(sim->dongles, args->number_of_coders);
		return (destroy_sim_mutexes(sim), NULL);
	}
	return (sim);
}

static void	mark_stop(t_sim *sim)
{
	pthread_mutex_lock(&sim->mutex_stop_flag);
	sim->stop_flag = 1;
	pthread_mutex_unlock(&sim->mutex_stop_flag);
}

int	launch_threads(t_sim *sim)
{
	int	i;
	int	n;

	n = sim->args->number_of_coders;
	i = 0;
	while (i < n)
	{
		if (pthread_create(&sim->coders[i]->thread, NULL, coder_routine,
				sim->coders[i]) != 0)
			return (mark_stop(sim), 0);
		sim->coders[i]->thread_launched = 1;
		i++;
	}
	if (pthread_create(&sim->thread, NULL, monitor_routine, sim) != 0)
		return (mark_stop(sim), 0);
	sim->monitor_launched = 1;
	return (1);
}

void	join_threads(t_sim *sim)
{
	int	i;
	int	n;

	n = sim->args->number_of_coders;
	i = 0;
	while (i < n)
	{
		if (sim->coders[i]->thread_launched)
			pthread_join(sim->coders[i]->thread, NULL);
		i++;
	}
	if (sim->monitor_launched)
		pthread_join(sim->thread, NULL);
}
