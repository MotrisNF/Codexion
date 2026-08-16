/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   monitor.c                                          :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

//Lee last_compile_start_ms bajo el mutex del propio coder (aqui es
//el hilo monitor quien lo lee, distinto del hilo que lo escribe en
//coder_routine.c, asi que hace falta el mutex de verdad).
static int	coder_burned_out(t_coder *coder, int burnout, long now)
{
	int	timed_out;

	pthread_mutex_lock(&coder->mutex);
	timed_out = ((now - coder->last_compile_start_ms) >= burnout);
	pthread_mutex_unlock(&coder->mutex);
	return (timed_out);
}

static int	coder_finished(t_coder *coder, int required)
{
	int	done;

	pthread_mutex_lock(&coder->mutex);
	done = (coder->compilations_done >= required);
	pthread_mutex_unlock(&coder->mutex);
	return (done);
}

//Activa el flag de parada y despierta a todos los hilos que puedan
//estar esperando en pthread_cond_wait sobre la condvar de
//cualquier dongle (cada dongle tiene la suya, hay que recorrerlos
//todos desde este unico hilo monitor).
static void	stop_simulation(t_sim *sim)
{
	int	i;

	pthread_mutex_lock(&sim->mutex_stop_flag);
	sim->stop_flag = 1;
	pthread_mutex_unlock(&sim->mutex_stop_flag);
	i = 0;
	while (i < sim->args->number_of_coders)
	{
		pthread_cond_broadcast(&sim->dongles[i]->cond);
		i++;
	}
}

//Recorre todos los coders una vez. Si alguno se ha agotado, loggea
//"burned out" (antes de que el llamante active el flag, como pide
//el subject) y corta ahi mismo. Si nadie se ha agotado, devuelve si
//todos han llegado ya al numero de compilaciones requerido.
static int	check_coders(t_sim *sim, long now)
{
	t_args	*args;
	int		i;
	int		all_done;

	args = sim->args;
	i = 0;
	all_done = 1;
	while (i < args->number_of_coders)
	{
		if (coder_burned_out(sim->coders[i], args->time_to_burnout, now))
		{
			log_event(sim, sim->coders[i]->id, "burned out");
			return (1);
		}
		if (!coder_finished(sim->coders[i], args->number_of_compiles_required))
			all_done = 0;
		i++;
	}
	return (all_done);
}

//Hilo monitor (firma exigida por pthread_create). Sondeo cada 1 ms:
//suficiente margen para detectar el burnout dentro de los 10 ms que
//exige el subject, sin consumir CPU en exceso.
void	*monitor_routine(void *arg)
{
	t_sim	*sim;

	sim = (t_sim *)arg;
	while (!sim_is_stopped(sim))
	{
		if (check_coders(sim, get_now_ms()))
		{
			stop_simulation(sim);
			break ;
		}
		usleep(1000);
	}
	return (NULL);
}
