/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   sim_builders.c                                     :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

//Destruye los "count" primeros dongles ya creados con exito (y el
//array que los contiene). "count" es cuantos hay validos, no la
//capacidad del array: si dongle_create fallo en la posicion i,
//count vale i (los dongles 0..i-1 son validos, el resto ni se
//llego a intentar).
void	free_dongles(t_dongle **dongles, int count)
{
	int	i;

	i = 0;
	while (i < count)
	{
		dongle_destroy(dongles[i]);
		i++;
	}
	free(dongles);
}

//Un dongle por asiento en la mesa (id 0..n-1). capacity = n porque,
//como mucho, esperan el mismo dongle todos los coders a la vez. Si
//algun dongle_create falla a mitad, deshace lo que ya se creo antes
//de devolver NULL (nada queda a medio reservar).
t_dongle	**create_dongles(int n)
{
	t_dongle	**dongles;
	int			i;

	dongles = malloc(sizeof(t_dongle *) * n);
	if (!dongles)
		return (NULL);
	i = 0;
	while (i < n)
	{
		dongles[i] = dongle_create(i, n);
		if (!dongles[i])
			return (free_dongles(dongles, i), NULL);
		i++;
	}
	return (dongles);
}

//Destruye los "count" primeros coders ya creados con exito (mutex
//incluido) y el array que los contiene. Mismo significado de
//"count" que en free_dongles.
void	free_coders(t_coder **coders, int count)
{
	int	i;

	i = 0;
	while (i < count)
	{
		pthread_mutex_destroy(&coders[i]->mutex);
		free(coders[i]);
		i++;
	}
	free(coders);
}

//Mesa circular: al coder de indice i (array 0-based) le tocan el
//dongle i (izquierda) y el dongle (i+1) % n (derecha); con n == 1
//da left == right sin caso especial aqui. El id que se expone
//(coder->id = i + 1) es 1-based, como exige el subject. Si algun
//malloc falla a mitad, deshace lo ya creado antes de devolver NULL.
t_coder	**create_coders(t_sim *sim)
{
	t_coder	**coders;
	int		n;
	int		i;

	n = sim->args->number_of_coders;
	coders = malloc(sizeof(t_coder *) * n);
	if (!coders)
		return (NULL);
	i = 0;
	while (i < n)
	{
		coders[i] = malloc(sizeof(t_coder));
		if (!coders[i])
			return (free_coders(coders, i), NULL);
		coders[i]->id = i + 1;
		coders[i]->thread_launched = 0;
		coders[i]->left = sim->dongles[i];
		coders[i]->right = sim->dongles[(i + 1) % n];
		coders[i]->last_compile_start_ms = get_now_ms();
		coders[i]->compilations_done = 0;
		coders[i]->simulation = sim;
		pthread_mutex_init(&coders[i]->mutex, NULL);
		i++;
	}
	return (coders);
}
