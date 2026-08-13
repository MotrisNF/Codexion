/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   main.c                                             :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/13 13:12:17 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/13 15:33:50 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	main(int argc, char **argv)
{
	t_args	*args;

	args = cheack_args(argv, argc);
	if (!args)
		return (printf("Use the correct format.\n"));
	printf("Todo ok.");
	printf("\n");
	return (free(args), 0);
}
