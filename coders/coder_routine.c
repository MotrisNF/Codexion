/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_routine.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 09:29:41 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static void	sleep_chunked(t_sim *sim, int ms)
{
	int	elapsed;
	int	step;

	elapsed = 0;
	while (elapsed < ms && !sim_is_stopped(sim))
	{
		step = ms - elapsed;
		if (step > 10)
			step = 10;
		usleep(step * 1000);
		elapsed += step;
	}
}

static void	log_and_wait(t_coder *coder, const char *event, int ms)
{
	log_event(coder->simulation, coder->id, event);
	sleep_chunked(coder->simulation, ms);
}

static void	release_pair(t_coder *coder)
{
	dongle_release(coder->left, coder);
	if (coder->left != coder->right)
		dongle_release(coder->right, coder);
}

static int	do_compile(t_coder *coder)
{
	int	compile_ms;

	compile_ms = coder->simulation->args->time_to_compile;
	if (!dongle_acquire_pair(coder))
		return (0);
	pthread_mutex_lock(&coder->mutex);
	coder->last_compile_start_ms = get_now_ms();
	pthread_mutex_unlock(&coder->mutex);
	log_and_wait(coder, "is compiling", compile_ms);
	release_pair(coder);
	pthread_mutex_lock(&coder->mutex);
	coder->compilations_done++;
	pthread_mutex_unlock(&coder->mutex);
	return (1);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;
	t_args	*args;

	coder = (t_coder *)arg;
	args = coder->simulation->args;
	while (!sim_is_stopped(coder->simulation))
	{
		if (!do_compile(coder))
			break ;
		if (sim_is_stopped(coder->simulation))
			break ;
		log_and_wait(coder, "is debugging", args->time_to_debug);
		if (sim_is_stopped(coder->simulation))
			break ;
		log_and_wait(coder, "is refactoring", args->time_to_refactor);
	}
	return (NULL);
}
