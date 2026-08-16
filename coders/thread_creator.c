/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   thread_creator.c                                   :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 15:21:26 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

//Destruye los dos mutex de "sim" y libera el propio struct. Solo se
//usa en los caminos de error de build_sim (cuando dongles/coders no
//se han podido crear): en el camino feliz, esto lo hace destroy_sim
//(main.c) tras haber unido y liberado tambien dongles y coders.
static void	destroy_sim_mutexes(t_sim *sim)
{
	pthread_mutex_destroy(&sim->mutex_log);
	pthread_mutex_destroy(&sim->mutex_stop_flag);
	free(sim);
}

//Construye el struct raiz: es el unico sitio del programa donde se
//crea, su puntero se pasa despues a todos los hilos (coders y
//monitor) para no necesitar ninguna variable global. Si dongles o
//coders fallan a mitad, deshace exactamente lo que ya se habia
//reservado (incluidos los dongles, si el fallo fue al crear los
//coders) antes de devolver NULL.
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

//Activa el flag de parada sin depender del monitor (el monitor
//puede no haberse llegado a crear). Se usa cuando pthread_create
//falla a mitad de lanzar los hilos: los que ya estan corriendo
//deben enterarse cuanto antes de que tienen que parar.
static void	mark_stop(t_sim *sim)
{
	pthread_mutex_lock(&sim->mutex_stop_flag);
	sim->stop_flag = 1;
	pthread_mutex_unlock(&sim->mutex_stop_flag);
}

//Lanza los N hilos coder y el monitor. Si algun pthread_create
//falla, deja de intentar lanzar mas, marca la parada (para que los
//que ya esten corriendo la detecten pronto: el timedwait de
//dongle_acquire y las esperas troceadas de coder_routine la
//revisan en un plazo acotado) y devuelve 0. join_threads sabe, por
//thread_launched/monitor_launched, cuales hay que unir de verdad.
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

//Solo une los hilos que de verdad se llegaron a lanzar: unir un
//pthread_t que pthread_create nunca inicializo es comportamiento
//indefinido (cuelgue o crash segun la plataforma).
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
