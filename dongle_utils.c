#include "codexion.h"

void	release_dongle(t_dongle *dongle, int cooldown)
{
	pthread_mutex_lock(&dongle->lock);
	dongle->is_held = 0;
	dongle->available_at = get_time_ms() + cooldown;
	pthread_cond_broadcast(&dongle->cond);
	pthread_mutex_unlock(&dongle->lock);
}

void	release_dongles(t_coder *coder)
{
	t_dongle	*left;
	t_dongle	*right;

	left = &coder->shared->dongle_array[coder->left_dongle - 1];
	right = &coder->shared->dongle_array[coder->right_dongle - 1];
	release_dongle(left, coder->shared->args->dongle_cooldown);
	release_dongle(right, coder->shared->args->dongle_cooldown);
}

void	init_dongle_requests(t_coder *coder, t_dongle **d1, t_dongle **d2)
{
	t_request	req;

	if (coder->left_dongle < coder->right_dongle)
	{
		*d1 = &coder->shared->dongle_array[coder->left_dongle - 1];
		*d2 = &coder->shared->dongle_array[coder->right_dongle - 1];
	}
	else
	{
		*d1 = &coder->shared->dongle_array[coder->right_dongle - 1];
		*d2 = &coder->shared->dongle_array[coder->left_dongle - 1];
	}
	req.coder_id = coder->coder_id;
	req.arrival_time = get_time_ms();
	pthread_mutex_lock(&coder->compile_lock);
	req.deadline = coder->last_compile_start
		+ coder->shared->args->time_to_burnout;
	pthread_mutex_unlock(&coder->compile_lock);
	pthread_mutex_lock(&(*d1)->lock);
	heap_push(*d1, req, coder->shared->args->scheduler);
	pthread_mutex_unlock(&(*d1)->lock);
	pthread_mutex_lock(&(*d2)->lock);
	heap_push(*d2, req, coder->shared->args->scheduler);
	pthread_mutex_unlock(&(*d2)->lock);
}

void	remove_both_requests(t_coder *coder, t_dongle *d1, t_dongle *d2)
{
	pthread_mutex_lock(&d1->lock);
	heap_remove(d1, coder->coder_id);
	pthread_mutex_unlock(&d1->lock);
	pthread_mutex_lock(&d2->lock);
	heap_remove(d2, coder->coder_id);
	pthread_mutex_unlock(&d2->lock);
}

int	handle_single_coder(t_coder *coder)
{
	t_dongle	*dongle;

	dongle = &coder->shared->dongle_array[0];
	pthread_mutex_lock(&dongle->lock);
	dongle->is_held = 1;
	print_log(coder, "has taken a dongle");
	while (!simulation_stopped(coder->shared))
		pthread_cond_wait(&dongle->cond, &dongle->lock);
	pthread_mutex_unlock(&dongle->lock);
	return (-1);
}
