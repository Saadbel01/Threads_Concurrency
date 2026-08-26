#include "codexion.h"

int	ft_strcmp(const char *s1, const char *s2)
{
	size_t	i;

	i = 0;
	while ((s1[i] || s2[i]) && (s1[i] == s2[i]))
		i++;
	return ((unsigned char)s1[i] - (unsigned char)s2[i]);
}

int	ft_strlen(const char *str)
{
	int	i;

	i = 0;
	while (str[i])
		i++;
	return (i);
}

long long	get_time_ms(void)
{
	struct timeval	tv;

	gettimeofday(&tv, NULL);
	return ((long long)tv.tv_sec * 1000LL + (long long)tv.tv_usec / 1000LL);
}

int	simulation_stopped(t_shared *shared)
{
	int	stopped;

	if (!shared)
		return (1);
	pthread_mutex_lock(&shared->mutex_stop);
	stopped = shared->stop_simulation;
	pthread_mutex_unlock(&shared->mutex_stop);
	return (stopped);
}

void	safe_sleep(long ms, t_shared *shared)
{
	long	start;

	start = get_time_ms();
	while (!simulation_stopped(shared))
	{
		if (get_time_ms() - start >= ms)
			break ;
		usleep(10);
	}
}
