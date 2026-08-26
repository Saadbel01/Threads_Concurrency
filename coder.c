#include "codexion.h"

void	compling(t_coder *coder)
{
	print_log(coder, "is compiling");
	safe_sleep(coder->shared->args->time_to_compile, coder->shared);
}

void	debugging(t_coder *coder)
{
	print_log(coder, "is debugging");
	safe_sleep(coder->shared->args->time_to_debug, coder->shared);
}

void	refactoring(t_coder *coder)
{
	print_log(coder, "is refactoring");
	safe_sleep(coder->shared->args->time_to_refactor, coder->shared);
}

static int	coder_cycle(t_coder *coder)
{
	if (acquire_dongles(coder) == -1)
		return (0);
	pthread_mutex_lock(&coder->compile_lock);
	coder->last_compile_start = get_time_ms();
	pthread_mutex_unlock(&coder->compile_lock);
	compling(coder);
	release_dongles(coder);
	if (simulation_stopped(coder->shared))
		return (0);
	debugging(coder);
	if (simulation_stopped(coder->shared))
		return (0);
	refactoring(coder);
	pthread_mutex_lock(&coder->compile_lock);
	coder->compiles_done += 1;
	pthread_mutex_unlock(&coder->compile_lock);
	if (simulation_stopped(coder->shared))
		return (0);
	return (1);
}

void	*coder_routine(void *arg)
{
	t_coder	*coder;
	int		done;

	coder = (t_coder *)arg;
	if (coder->coder_id % 2 == 0)
		usleep(1000);
	while (1)
	{
		pthread_mutex_lock(&coder->compile_lock);
		done = coder->compiles_done;
		pthread_mutex_unlock(&coder->compile_lock);
		if (done >= coder->shared->args->nb_of_compiles)
			break ;
		if (!coder_cycle(coder))
			break ;
	}
	return (NULL);
}
