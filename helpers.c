#include "codexion.h"

static int	calc_num(const char	*str)
{
	int	i;
	int	res;

	i = 0;
	res = 0;
	while (str[i])
	{
		if (str[i] >= 48 && str[i] <= 57)
		{
			res = res * 10 + (str[i] - '0');
		}
		else
		{
			return (res);
		}
		i++;
	}
	return (res);
}

int	ft_atoi(const char	*str)
{
	int	i;
	int	nb;

	i = 0;
	nb = 0;
	while (str[i] == ' ' || str[i] == '\t' || str[i] == '\n' || str[i] == '\r'
		|| str[i] == '\v' || str[i] == '\f')
		i++;
	if (str[i] == '-' || str[i] == '+')
	{
		if (str[i] == '-')
		{
			nb++;
		}
		i++;
	}
	if (nb == 1)
		return (calc_num(str + i) * -1);
	return (calc_num(str + i));
}

int	ft_strcmp(const char	*s1, const char	*s2)
{
	size_t	i;

	i = 0;
	while ((s1[i] || s2[i]) && (s1[i] == s2[i]))
	{
		i++;
	}
	return ((unsigned char)s1[i] - (unsigned char)s2[i]);
}

int	ft_strlen(const char	*str)
{
	int	i;

	i = 0;
	while (str[i])
	{
		i++;
	}
	return (i);
}