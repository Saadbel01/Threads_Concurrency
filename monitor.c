#include "codexion.h"

static int	check_coder_burnout(t_shared *shared, int i, int *burnout_id)
{
	long long	last_compile;
	int			compiles;

	pthread_mutex_lock(&shared->coders[i].compile_lock);
	last_compile = shared->coders[i].last_compile_start;
	compiles = shared->coders[i].compiles_done;
	pthread_mutex_unlock(&shared->coders[i].compile_lock);
	if (compiles < shared->args->nb_of_compiles)
	{
		if (get_time_ms() - last_compile >= shared->args->time_to_burnout)
		{
			*burnout_id = shared->coders[i].coder_id;
			return (1);
		}
		return (0);
	}
	return (2);
}

static int	monitor_check_all(t_shared *shared, int *burnout_id)
{
	int	i;
	int	completed_count;
	int	res;

	i = 0;
	completed_count = 0;
	while (i < shared->args->nb_coders)
	{
		res = check_coder_burnout(shared, i, burnout_id);
		if (res == 1)
			return (1);
		if (res == 2)
			completed_count++;
		i++;
	}
	if (completed_count == shared->args->nb_coders)
		return (2);
	return (0);
}

static void	handle_burnout(t_shared *shared, int burnout_id)
{
	pthread_mutex_lock(&shared->log_mutex);
	printf("%lld %d burned out\n",
		get_time_ms() - shared->start_simulation, burnout_id);
	pthread_mutex_lock(&shared->mutex_stop);
	shared->stop_simulation = 1;
	pthread_mutex_unlock(&shared->mutex_stop);
	pthread_mutex_unlock(&shared->log_mutex);
	wake_all_dongles(shared);
}

static void	handle_completion(t_shared *shared)
{
	pthread_mutex_lock(&shared->mutex_stop);
	shared->stop_simulation = 1;
	pthread_mutex_unlock(&shared->mutex_stop);
	wake_all_dongles(shared);
}

void	*monitor_routine(void *arg)
{
	t_shared	*shared;
	int			burnout_id;
	int			status;

	shared = (t_shared *)arg;
	while (1)
	{
		burnout_id = 0;
		status = monitor_check_all(shared, &burnout_id);
		if (status == 1)
		{
			handle_burnout(shared, burnout_id);
			return (NULL);
		}
		if (status == 2)
		{
			handle_completion(shared);
			return (NULL);
		}
		usleep(1000);
	}
	return (NULL);
}
