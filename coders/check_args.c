/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   check_args.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 13:26:38 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/18 11:16:38 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	check_numeric_args(char **argv)
{
	int		i;
	char	*args[7];

	args[0] = "number_of_coders";
	args[1] = "time_to_burnout";
	args[2] = "time_to_compile";
	args[3] = "time_to_debug";
	args[4] = "time_to_refactor";
	args[5] = "number_of_compiles_required";
	args[6] = "dongle_cooldown";
	i = 1;
	while (i <= 7)
	{
		if (!is_valid_number(argv[i]))
		{
			printf("\n%s is not a valid number (%s)\n\n", args[i], argv[i]);
			return (0);
		}
		i++;
	}
	return (1);
}

static void	fill_struct(char **argv, t_args *type)
{
	type->number_of_coders = atoi(argv[1]);
	type->time_to_burnout = atoi(argv[2]);
	type->time_to_compile = atoi(argv[3]);
	type->time_to_debug = atoi(argv[4]);
	type->time_to_refactor = atoi(argv[5]);
	type->number_of_compiles_required = atoi(argv[6]);
	type->dongle_cooldown = atoi(argv[7]);
	type->schedule = argv[8];
}

int	check_struct_values(char **argv, t_args *args)
{
	int	value;

	value = 0;
	if (args->number_of_coders <= 0)
		value = printf("number_of_coders is <= 0 (%s).\n", argv[1]);
	if (args->time_to_burnout <= 0)
		value = printf("time_to_burnout is <= 0 (%s).\n", argv[2]);
	if (args->time_to_compile <= 0)
		value = printf("time_to_compile is <= 0 (%s).\n", argv[3]);
	if (args->time_to_debug <= 0)
		value = printf("time_to_debug is <= 0 (%s).\n", argv[4]);
	if (args->time_to_refactor <= 0)
		value = printf("time_to_refactor is <= 0 (%s).\n", argv[5]);
	if (args->number_of_compiles_required <= 0)
		value = printf("number_of_compiles_required is <= 0 (%s).\n", argv[6]);
	if (args->dongle_cooldown < 0)
		value = printf("dongle_cooldown is < 0 (%s).\n", argv[7]);
	if (strcmp("fifo", args->schedule) != 0
		&& strcmp("edf", args->schedule) != 0)
	{
		value = printf("schedule can be 'fifo' or 'edf' <= 0 (argv[8]). ");
		printf("Yours: %s\n", args->schedule);
	}
	return (value);
}

t_args	*cheack_args(char **argv, int argc)
{
	t_args	*args;

	if (argc != 9)
		return (NULL);
	if (!check_numeric_args(argv))
		return (NULL);
	args = malloc(sizeof(t_args));
	if (!args)
		return (NULL);
	fill_struct(argv, args);
	if (check_struct_values(argv, args) != 0)
		return (free(args), NULL);
	return (args);
}
