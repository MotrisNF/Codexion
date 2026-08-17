/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_lookup.c                                      :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/17 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

int	heap_find(t_heap *heap, int coder_id, long *key_out)
{
	int	i;

	i = 0;
	while (i < heap->size)
	{
		if (heap->nodes[i].coder_id == coder_id)
		{
			*key_out = heap->nodes[i].key;
			return (1);
		}
		i++;
	}
	return (0);
}

int	heap_remove_id(t_heap *heap, int coder_id)
{
	int	i;

	i = 0;
	while (i < heap->size && heap->nodes[i].coder_id != coder_id)
		i++;
	if (i == heap->size)
		return (0);
	heap->size--;
	heap->nodes[i] = heap->nodes[heap->size];
	heap_sift_down(heap, i);
	heap_sift_up(heap, i);
	return (1);
}
