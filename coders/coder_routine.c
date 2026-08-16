/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   coder_routine.c                                    :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

//Espera "ms" milisegundos en tramos de como mucho 10 ms cada uno,
//comprobando el flag de parada entre tramo y tramo (para reaccionar
//rapido si el monitor para la simulacion a mitad de una espera
//larga) sin trocear tan fino como para que la suma de syscalls y
//locks de sim_is_stopped introduzca un overhead de tiempo medible
//frente a los "ms" pedidos.
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

//Loggea el evento y espera el tiempo de esa fase: comun a las tres
//fases (compilar, depurar, refactorizar), solo cambia el texto y
//la duracion.
static void	log_and_wait(t_coder *coder, const char *event, int ms)
{
	log_event(coder->simulation, coder->id, event);
	sleep_chunked(coder->simulation, ms);
}

//Libera el par de dongles del coder. Si number_of_coders == 1,
//left y right apuntan al mismo dongle (ver thread_creator.c): en
//ese caso solo se libera una vez, o se liberaria dos veces el mismo
//mutex/cooldown.
static void	release_pair(t_coder *coder)
{
	dongle_release(coder->left, coder);
	if (coder->left != coder->right)
		dongle_release(coder->right, coder);
}

//Un intento de compilacion completo: pide el par de dongles y, solo
//si lo consigue, registra el instante en que EMPIEZA A COMPILAR DE
//VERDAD. El subject define el burnout como "time_to_burnout ms
//desde el inicio de SU ULTIMA COMPILACION" (no desde que empieza a
//pedir los dongles): el tiempo esperando contencion no debe restar
//del margen de burnout, solo cuenta el tiempo entre compilaciones
//reales. Devuelve 0 si la simulacion se paro mientras esperaba.
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

//Rutina del hilo coder (firma exigida por pthread_create). Repite
//el ciclo compilar -> depurar -> refactorizar hasta que la
//simulacion se pare, comprobando el flag al principio de cada
//vuelta y entre fase y fase (para no loggear una fase de mas si la
//simulacion se para justo despues de compilar o de depurar).
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
