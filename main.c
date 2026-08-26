#include "codexion.h"

void	cleanup_shared(t_shared *shared)
{
	int	i;

	i = 0;
	while (i < shared->args->nb_coders)
	{
		pthread_mutex_destroy(&shared->dongle_array[i].lock);
		pthread_cond_destroy(&shared->dongle_array[i].cond);
		pthread_mutex_destroy(&shared->coders[i].compile_lock);
		i++;
	}
	pthread_mutex_destroy(&shared->mutex_stop);
	pthread_mutex_destroy(&shared->log_mutex);
	free(shared->coders);
	free(shared->dongle_array);
	free(shared);
}

static void	stop_and_join_coders(t_shared *s, pthread_t *threads, int count)
{
	int	j;

	pthread_mutex_lock(&s->mutex_stop);
	s->stop_simulation = 1;
	pthread_mutex_unlock(&s->mutex_stop);
	wake_all_dongles(s);
	j = 0;
	while (j < count)
	{
		pthread_join(threads[j], NULL);
		j++;
	}
}

static int	create_coders(t_shared *s, pthread_t *threads)
{
	int	i;

	i = 0;
	while (i < s->args->nb_coders)
	{
		if (pthread_create(&threads[i], NULL, coder_routine,
				&s->coders[i]) != 0)
		{
			stop_and_join_coders(s, threads, i);
			return (0);
		}
		i++;
	}
	return (1);
}

static void	join_all_threads(t_shared *s, pthread_t *threads, pthread_t mon)
{
	int	i;

	i = 0;
	while (i < s->args->nb_coders)
		pthread_join(threads[i++], NULL);
	pthread_join(mon, NULL);
}

int	main(int argc, char **argv)
{
	t_arg		args;
	t_shared	*shared;
	pthread_t	*threads;
	pthread_t	mon;

	check_arguments(argc, argv);
	get_arguments(argv, &args);
	shared = init_shared(&args);
	if (!shared)
		return (1);
	threads = malloc(shared->args->nb_coders * sizeof(pthread_t));
	if (!threads || !create_coders(shared, threads))
		return (1);
	if (pthread_create(&mon, NULL, monitor_routine, shared) != 0)
	{
		stop_and_join_coders(shared, threads, shared->args->nb_coders);
		free(threads);
		cleanup_shared(shared);
		return (1);
	}
	join_all_threads(shared, threads, mon);
	cleanup_shared(shared);
	free(threads);
	return (0);
}
