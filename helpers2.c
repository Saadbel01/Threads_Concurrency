#include "codexion.h"

void	wake_all_dongles(t_shared *shared)
{
	int	i;

	i = 0;
	while (i < shared->args->nb_coders)
	{
		pthread_mutex_lock(&shared->dongle_array[i].lock);
		pthread_cond_broadcast(&shared->dongle_array[i].cond);
		pthread_mutex_unlock(&shared->dongle_array[i].lock);
		i++;
	}
}

void	print_log(t_coder *coder, char *msg)
{
	long long	ts;

	pthread_mutex_lock(&coder->shared->log_mutex);
	if (!simulation_stopped(coder->shared))
	{
		ts = get_time_ms() - coder->shared->start_simulation;
		printf("%lld %d %s\n", ts, coder->coder_id, msg);
	}
	pthread_mutex_unlock(&coder->shared->log_mutex);
}
