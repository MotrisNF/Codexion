/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

//Timestamp relativo al arranque de la simulacion, solo para lo que
//se imprime en el log (la logica interna de cooldown/burnout usa
//get_now_ms() directamente, sin pasar por start_time_ms).
long	now_ms(t_sim *sim)
{
	return (get_now_ms() - sim->start_time_ms);
}

//Un color ANSI distinto por coder, ciclando cada 12 (id % 12) si
//number_of_coders > 12. No es variable global: es un array local a
//la funcion, recreado en cada llamada, solo con enlace de solo
//lectura de sus literales de cadena.
static const char	*color_for(int coder_id)
{
	const char	*colors[12];

	colors[0] = "\033[31m";
	colors[1] = "\033[32m";
	colors[2] = "\033[33m";
	colors[3] = "\033[34m";
	colors[4] = "\033[35m";
	colors[5] = "\033[36m";
	colors[6] = "\033[91m";
	colors[7] = "\033[92m";
	colors[8] = "\033[93m";
	colors[9] = "\033[94m";
	colors[10] = "\033[95m";
	colors[11] = "\033[96m";
	return (colors[coder_id % 12]);
}

//Unico punto de printf del proyecto para eventos de la simulacion.
//coder_id se imprime tal cual esta en t_coder->id: 1-based (el
//primer coder creado es el 1), como exige el subject ("cada
//persona tiene un numero que va de 1 a number_of_coders") y como
//usa su propio ejemplo de log. La linea entera (timestamp, id y
//evento incluidos) va envuelta en el color del coder y el reset al
//final, para que el formato exigido por el subject
//("timestamp_in_ms X evento") siga intacto si algo extrae el texto
//ignorando los codigos de color ANSI.
void	log_event(t_sim *sim, int coder_id, const char *event)
{
	pthread_mutex_lock(&sim->mutex_log);
	printf("%s%ld %d %s\033[0m\n", color_for(coder_id), now_ms(sim),
		coder_id, event);
	pthread_mutex_unlock(&sim->mutex_log);
}
