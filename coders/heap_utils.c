/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 09:17:26 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

void	heap_swap(t_heap_node *a, t_heap_node *b)
{
	t_heap_node	tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

void	heap_sift_up(t_heap *heap, int i)
{
	int	parent;

	while (i > 0)
	{
		parent = (i - 1) / 2;
		if (heap->nodes[parent].key <= heap->nodes[i].key)
			break ;
		heap_swap(&heap->nodes[parent], &heap->nodes[i]);
		i = parent;
	}
}

void	heap_sift_down(t_heap *heap, int i)
{
	t_heap_node	*n;
	int			left;
	int			right;
	int			smallest;

	n = heap->nodes;
	left = 2 * i + 1;
	right = 2 * i + 2;
	smallest = i;
	if (left < heap->size && n[left].key < n[smallest].key)
		smallest = left;
	if (right < heap->size && n[right].key < n[smallest].key)
		smallest = right;
	if (smallest != i)
	{
		heap_swap(&n[i], &n[smallest]);
		heap_sift_down(heap, smallest);
	}
}
