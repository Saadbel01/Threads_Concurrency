#include "codexion.h"

static void	compute_timeout(long long wait_ms, struct timespec *timeout)
{
	clock_gettime(CLOCK_REALTIME, timeout);
	timeout->tv_sec += wait_ms / 1000;
	timeout->tv_nsec += (wait_ms % 1000) * 1000000L;
	if (timeout->tv_nsec >= 1000000000L)
	{
		timeout->tv_sec += 1;
		timeout->tv_nsec -= 1000000000L;
	}
}

static int	try_take_both(t_coder *coder, t_dongle *d1, t_dongle *d2)
{
	long long	now;

	now = get_time_ms();
	if (d1->heap[TOP].coder_id == coder->coder_id && !d1->is_held
		&& now >= d1->available_at
		&& d2->heap[TOP].coder_id == coder->coder_id && !d2->is_held
		&& now >= d2->available_at)
	{
		heap_pop(d1);
		d1->is_held = 1;
		heap_pop(d2);
		d2->is_held = 1;
		print_log(coder, "has taken a dongle");
		print_log(coder, "has taken a dongle");
		return (1);
	}
	return (0);
}

static void	wait_dongle(t_coder *c, t_dongle *target)
{
	long long		now;
	long long		wait_ms;
	struct timespec	timeout;

	now = get_time_ms();
	if (target->heap[TOP].coder_id == c->coder_id && !target->is_held
		&& now < target->available_at)
	{
		wait_ms = target->available_at - now;
		if (wait_ms > 0)
		{
			compute_timeout(wait_ms, &timeout);
			pthread_cond_timedwait(&target->cond, &target->lock, &timeout);
		}
	}
	else
		pthread_cond_wait(&target->cond, &target->lock);
	pthread_mutex_unlock(&target->lock);
}

static void	assign_dongles(t_coder *c, t_dongle **d1, t_dongle **d2)
{
	if (c->left_dongle < c->right_dongle)
	{
		*d1 = &c->shared->dongle_array[c->left_dongle - 1];
		*d2 = &c->shared->dongle_array[c->right_dongle - 1];
	}
	else
	{
		*d1 = &c->shared->dongle_array[c->right_dongle - 1];
		*d2 = &c->shared->dongle_array[c->left_dongle - 1];
	}
}

int	acquire_dongles(t_coder *coder)
{
	t_dongle	*d1;
	t_dongle	*d2;

	if (coder->shared->args->nb_coders == 1)
		return (handle_single_coder(coder));
	assign_dongles(coder, &d1, &d2);
	push_both_requests(coder, d1, d2);
	while (!simulation_stopped(coder->shared))
	{
		pthread_mutex_lock(&d1->lock);
		pthread_mutex_lock(&d2->lock);
		if (try_take_both(coder, d1, d2))
		{
			pthread_mutex_unlock(&d2->lock);
			pthread_mutex_unlock(&d1->lock);
			return (1);
		}
		if (d1->heap[TOP].coder_id != coder->coder_id || d1->is_held
			|| get_time_ms() < d1->available_at)
		{
			pthread_mutex_unlock(&d2->lock);
			wait_dongle(coder, d1);
		}
		else
		{
			pthread_mutex_unlock(&d1->lock);
			wait_dongle(coder, d2);
		}
	}
	remove_both_requests(coder, d1, d2);
	return (-1);
}
