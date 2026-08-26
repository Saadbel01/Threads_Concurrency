#include "codexion.h"

#define ARG_CODERS 1
#define ARG_BURNOUT 2
#define ARG_COMPILE 3
#define ARG_DEBUG 4
#define ARG_REFACTOR 5
#define ARG_COMPILES 6
#define ARG_COOLDOWN 7
#define ARG_SCHEDULER 8

int	in_range_int(char *s)
{
	long long	nb;
	long		i;

	nb = 0;
	i = 0;
	while (s[i])
	{
		nb = nb * 10 + (s[i] - '0');
		if (nb > 2147483647 || nb == 0)
			return (0);
		i++;
	}
	return (nb);
}

int	check_is_valid_number(char *s)
{
	int	i;

	i = 0;
	while (s[i])
	{
		if (s[i] >= '0' && s[i] <= '9')
			i++;
		else
			return (0);
	}
	return (1);
}

static void	validate_number_args(char **argv)
{
	int	i;

	i = 1;
	while (i < 8)
	{
		if (!check_is_valid_number(argv[i]))
		{
			printf("Argument %d is not a valid positive integer.\n", i);
			exit(1);
		}
		if (i != 7 && !in_range_int(argv[i]))
		{
			printf("Argument %d should be between 1 and MAX_INT.\n", i);
			exit(1);
		}
		i++;
	}
}

void	check_arguments(int argc, char **argv)
{
	if (argc != 9)
	{
		printf("The number of arguments is not correct.\n");
		exit(1);
	}
	validate_number_args(argv);
	if (ft_strcmp(argv[ARG_SCHEDULER], "fifo") != 0
		&& ft_strcmp(argv[ARG_SCHEDULER], "edf") != 0)
	{
		printf("The scheduler should be exactly edf or fifo.\n");
		exit(1);
	}
}

void	get_arguments(char **argv, t_arg *args)
{
	args->nb_coders = in_range_int(argv[ARG_CODERS]);
	args->time_to_burnout = in_range_int(argv[ARG_BURNOUT]);
	args->time_to_compile = in_range_int(argv[ARG_COMPILE]);
	args->time_to_debug = in_range_int(argv[ARG_DEBUG]);
	args->time_to_refactor = in_range_int(argv[ARG_REFACTOR]);
	args->nb_of_compiles = in_range_int(argv[ARG_COMPILES]);
	args->dongle_cooldown = in_range_int(argv[ARG_COOLDOWN]);
	if (ft_strcmp(argv[ARG_SCHEDULER], "fifo") == 0)
		args->scheduler = 0;
	else
		args->scheduler = 1;
}
