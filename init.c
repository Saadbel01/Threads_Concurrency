#include "codexion.h"

static void	destroy_dongles(t_dongle *array, int count)
{
	int	j;

	j = count - 1;
	while (j >= 0)
	{
		pthread_cond_destroy(&array[j].cond);
		pthread_mutex_destroy(&array[j].lock);
		j--;
	}
}

int	init_dongle_array(t_dongle *array, int nb_coders)
{
	int	i;

	i = 0;
	while (i < nb_coders)
	{
		array[i].dongle_id = i + 1;
		array[i].available_at = 0;
		array[i].is_held = 0;
		array[i].heap_size = 0;
		if (pthread_mutex_init(&array[i].lock, NULL) != 0)
		{
			destroy_dongles(array, i);
			return (-1);
		}
		if (pthread_cond_init(&array[i].cond, NULL) != 0)
		{
			pthread_mutex_destroy(&array[i].lock);
			destroy_dongles(array, i);
			return (-1);
		}
		i++;
	}
	return (0);
}

static void	destroy_coders(t_coder *coders, int count)
{
	int	j;

	j = count - 1;
	while (j >= 0)
	{
		pthread_mutex_destroy(&coders[j].compile_lock);
		j--;
	}
}

int	init_coders(t_coder *coders, t_shared *shared, int nb_coders)
{
	int	i;

	i = 0;
	while (i < nb_coders)
	{
		coders[i].coder_id = i + 1;
		coders[i].compiles_done = 0;
		coders[i].last_compile_start = shared->start_simulation;
		coders[i].phase = 0;
		coders[i].shared = shared;
		coders[i].right_dongle = i + 1;
		coders[i].left_dongle = i;
		if (i == 0)
			coders[i].left_dongle = nb_coders;
		if (pthread_mutex_init(&coders[i].compile_lock, NULL) != 0)
		{
			destroy_coders(coders, i);
			return (-1);
		}
		i++;
	}
	return (0);
}

t_shared	*init_shared(t_arg *args)
{
	t_shared	*shared;

	shared = malloc(sizeof(t_shared));
	if (!shared)
		return (NULL);
	shared->start_simulation = get_time_ms();
	shared->stop_simulation = 0;
	shared->args = args;
	shared->dongle_array = malloc(sizeof(t_dongle) * args->nb_coders);
	shared->coders = malloc(sizeof(t_coder) * args->nb_coders);
	if (!shared->dongle_array || !shared->coders
		|| init_dongle_array(shared->dongle_array, args->nb_coders) != 0
		|| init_coders(shared->coders, shared, args->nb_coders) != 0
		|| pthread_mutex_init(&shared->log_mutex, NULL) != 0
		|| pthread_mutex_init(&shared->mutex_stop, NULL) != 0)
	{
		free(shared->dongle_array);
		free(shared->coders);
		free(shared);
		return (NULL);
	}
	return (shared);
}
