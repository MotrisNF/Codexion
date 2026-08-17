/* ************************************************************************** */
/*                                                                            */
/*                                                        :::      ::::::::   */
/*   validate_number.c                                  :+:      :+:    :+:   */
/*                                                    +:+ +:+         +:+     */
/*   By: saperez- <saperez-@student.42madrid.com    +#+  +:+       +#+        */
/*                                                +#+#+#+#+#+   +#+           */
/*   Created: 2026/08/16 00:00:00 by saperez-          #+#    #+#             */
/*   Updated: 2026/08/17 09:20:44 by saperez-         ###   ########.fr       */
/*                                                                            */
/* ************************************************************************** */

#include "codexion.h"

static int	is_digit_char(char c)
{
	return (c >= '0' && c <= '9');
}

static int	accumulate_digit(long *value, char digit, int sign)
{
	*value = *value * 10 + (digit - '0');
	if (sign == 1 && *value > INT_MAX)
		return (0);
	if (sign == -1 && -(*value) < INT_MIN)
		return (0);
	return (1);
}

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
