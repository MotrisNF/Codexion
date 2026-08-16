/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   heap_utils.c                                       :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

//Reordenamiento del heap (subir/bajar), separado de heap.c para no
//pasarse del limite de funciones por archivo de la Norma. No estan
//pensadas para llamarse desde fuera de heap.c/heap_utils.c, pero
//necesitan enlace externo (no static) al vivir en otro archivo.

void	heap_swap(t_heap_node *a, t_heap_node *b)
{
	t_heap_node	tmp;

	tmp = *a;
	*a = *b;
	*b = tmp;
}

//Tras insertar al final del array, lo va intercambiando con su padre
//mientras su key sea menor que la de este (recupera la invariante
//de min-heap "subiendo" el nodo nuevo).
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

//Tras mover el ultimo elemento a la raiz (posicion 0), lo va
//intercambiando con el menor de sus dos hijos mientras alguno de
//ellos tenga una key menor que la suya ("baja" el nodo). "n" es un
//alias corto de heap->nodes, para no pasarse de columna.
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
