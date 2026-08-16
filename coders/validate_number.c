/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   validate_number.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/16 00:00:00 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	is_digit_char(char c)
{
	return (c >= '0' && c <= '9');
}

//Acumula un digito mas en *value y comprueba overflow de int en el
//mismo paso (asi el valor nunca crece mas de lo que hace falta para
//detectarlo). "value" es long: en esta maquina (x86_64) es de 64
//bits, mas que de sobra para no desbordar antes de la comprobacion.
static int	accumulate_digit(long *value, char digit, int sign)
{
	*value = *value * 10 + (digit - '0');
	if (sign == 1 && *value > INT_MAX)
		return (0);
	if (sign == -1 && -(*value) < INT_MIN)
		return (0);
	return (1);
}

//1 si str es un entero valido para atoi (con signo opcional, solo
//digitos, sin overflow de int), 0 en cualquier otro caso: vacio,
//caracteres no numericos, espacios, "+"/"-" solos, etc.
int	is_valid_number(const char *str)
{
	int		i;
	long	value;
	int		sign;

	i = 0;
	sign = 1;
	value = 0;
	if (!str || !str[0])
		return (0);
	if (str[0] == '+' || str[0] == '-')
	{
		if (str[0] == '-')
			sign = -1;
		i++;
	}
	if (!str[i])
		return (0);
	while (str[i])
	{
		if (!is_digit_char(str[i]) || !accumulate_digit(&value, str[i], sign))
			return (0);
		i++;
	}
	return (1);
}
