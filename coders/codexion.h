/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   codexion.h                                         :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 13:14:47 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/13 15:34:26 by saperez-         ###   ########.fr       */
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

//Struct for save the arguments recived on the program
typedef struct s_args
{
	int		number_of_coders;
	int		time_to_burnout;
	int		time_to_compile;
	int		time_to_debug;
	int		time_to_refactor;
	int		number_of_compiles_required;
	int		dongle_cooldown;
	char	*schedule;
}	t_args;

//Struct for the coders
typedef struct coders
{
	int		time_to_burnout;
	int		time_to_compile;
	int		compiling;
	int		time_to_debug;
	int		debuging;
	int		number_of_compiles_required;
	int		compilations;
	int		last_compile_start;
	int		left_dongle;
	int		rigth_dongle;
}	t_coder;

//Parser
t_args	*cheack_args(char **argv, int argc);
#endif