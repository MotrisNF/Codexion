/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   log.c                                              :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 10:13:44 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

long	now_ms(t_sim *sim)
{
	return (get_now_ms() - sim->start_time_ms);
}

static const char	*color_for(int coder_id)
{
	const char	*colors[22];

	colors[0] = "\033[38;5;196m";
	colors[1] = "\033[38;5;46m";
	colors[2] = "\033[38;5;226m";
	colors[3] = "\033[38;5;21m";
	colors[4] = "\033[38;5;201m";
	colors[5] = "\033[38;5;51m";
	colors[6] = "\033[38;5;208m";
	colors[7] = "\033[38;5;118m";
	colors[8] = "\033[38;5;220m";
	colors[9] = "\033[38;5;27m";
	colors[10] = "\033[38;5;165m";
	colors[11] = "\033[38;5;45m";
	colors[12] = "\033[38;5;22m";
	colors[13] = "\033[38;5;154m";
	colors[14] = "\033[38;5;214m";
	colors[15] = "\033[38;5;33m";
	colors[16] = "\033[38;5;177m";
	colors[17] = "\033[38;5;37m";
	colors[18] = "\033[38;5;160m";
	colors[19] = "\033[38;5;70m";
	colors[20] = "\033[38;5;190m";
	colors[21] = "\033[38;5;63m";
	return (colors[coder_id % 22]);
}

void	log_event(t_sim *sim, int coder_id, const char *event)
{
	pthread_mutex_lock(&sim->mutex_log);
	printf("%s%ld %d %s\033[0m\n", color_for(coder_id), now_ms(sim),
		coder_id, event);
	pthread_mutex_unlock(&sim->mutex_log);
}
