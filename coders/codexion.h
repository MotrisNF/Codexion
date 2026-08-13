/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 13:14:47 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/13 16:11:06 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#ifndef CODEXION_H
# define CODEXION_H

//Includes
# include <pthread.h>
# include <sys/time.h>
# include <unistd.h>
# include <stdlib.h>
# include <stdio.h>
# include <string.h>

//Struct of the simulation
typedef struct s_sim
{
	t_args			*args;
	t_dongle		**dongles;
	t_coder			**coders;
	pthread_t		thread;
	pthread_mutex_t	mutex_log;
	int				stop_flag;
	pthread_mutex_t	mutex_stop_flag;
	long			start_time_ms;
}	t_sim;

//Struct for save the arguments recived on the program
typedef struct s_args
{
	int				number_of_coders;
	int				time_to_burnout;
	int				time_to_compile;
	int				time_to_debug;
	int				time_to_refactor;
	int				number_of_compiles_required;
	int				dongle_cooldown;
	char			*schedule;
}	t_args;

//Struct for dongle
typedef struct s_dongle
{
	int				id;
	int				taked;
	long			aviable_at_ms;
	pthread_mutex_t	mutex;
	pthread_cond_t	cond;
}	t_dongle;

//Struct for the coders
typedef struct s_coder
{
	int				id;
	pthread_t		thread;
	long			last_compile_start_ms;
	pthread_mutex_t	mutex;
	int				compilations_done;
	t_sim			*simulation;
}	t_coder;

//Parser
t_args	*cheack_args(char **argv, int argc);
#endif