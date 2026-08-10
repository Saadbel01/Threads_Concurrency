#include "codexion.h"


int take_dongle(t_dongle *dongle, t_coder  *coder)
{
    int stop;

    pthread_mutex_lock(&dongle->lock);
    pthread_mutex_lock(&coder->shared->mutex_stop);
    stop = coder->shared->stop_simulation;
    pthread_mutex_unlock(&coder->shared->mutex_stop);
    while ((dongle->is_held == 1
            || get_time_ms() < dongle->available_at)
        && stop == 0)
    {    
        pthread_cond_wait(&dongle->cond, &dongle->lock);
        pthread_mutex_lock(&coder->shared->mutex_stop);
        stop = coder->shared->stop_simulation;
        pthread_mutex_unlock(&coder->shared->mutex_stop);
    }
    if (stop == 1)
    {
        pthread_mutex_unlock(&dongle->lock);
        return (-1);
    }
    else
    {
        dongle->is_held = 1;
        pthread_mutex_lock(&coder->shared->log_mutex);
        printf("%ld %d has taken a dongle\n", get_time_ms() - coder->shared->start_simulation, coder->coder_id);
        pthread_mutex_unlock(&coder->shared->log_mutex);
        pthread_mutex_unlock(&dongle->lock);
        return (1);
    }
    
}

int acquire_dongles(t_coder *coder)
{
    t_dongle   *first_dongle;
    t_dongle   *second_dongle;

    if (coder->left_dongle < coder->right_dongle)
    {
        first_dongle = &coder->shared->dongle_array[coder->left_dongle - 1];
        second_dongle = &coder->shared->dongle_array[coder->right_dongle - 1];
    }
    else
    {
        first_dongle = &coder->shared->dongle_array[coder->right_dongle - 1];
        second_dongle = &coder->shared->dongle_array[coder->left_dongle - 1];
    }

    if (take_dongle(first_dongle, coder) == -1)
        return (-1);
    if (take_dongle(second_dongle, coder) == -1)
    {
        release_dongle(first_dongle, coder->shared->args->dongle_cooldown);
        return (-1);
    }
    return (1);
}

void    release_dongle(t_dongle *dongle, int cooldown)
{
    pthread_mutex_lock(&dongle->lock);
    dongle->is_held = 0;
    dongle->available_at = get_time_ms() + cooldown;
    pthread_cond_signal(&dongle->cond);
    pthread_mutex_unlock(&dongle->lock);
}

void    release_dongles(t_coder *coder)
{
    t_dongle *left;
    t_dongle *right;

    left = &coder->shared->dongle_array[coder->left_dongle - 1];
    right = &coder->shared->dongle_array[coder->right_dongle - 1];
    release_dongle(left, coder->shared->args->dongle_cooldown);
    release_dongle(right, coder->shared->args->dongle_cooldown);
}

