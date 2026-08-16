/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   time_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long	get_now_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long)tv.tv_sec * 1000 + (long)tv.tv_usec / 1000);
}

//Fecha limite absoluta, "ms" milisegundos a partir de ahora, para
//pthread_cond_timedwait. Se usa para que un coder esperando un
//dongle en cooldown se despierte solo por si mismo a revisar la
//condicion, en vez de depender de que otro hilo haga broadcast (el
//cooldown termina solo, nadie hace broadcast cuando eso pasa).
//Se escribe en "*out" en vez de devolverlo por valor: "struct
//timespec" como tipo de retorno no encaja en la columna de
//alineacion de las declaraciones del header (Norma).
void	deadline_in_ms(int ms, struct timespec *out)
{
	struct timeval	tv;
	long			usec;

	gettimeofday(&tv, NULL);
	usec = tv.tv_usec + (long)ms * 1000;
	out->tv_sec = tv.tv_sec + usec / 1000000;
	out->tv_nsec = (usec % 1000000) * 1000;
}
