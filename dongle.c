#include "codexion.h"


int take_dongle(t_dongle *dongle, t_coder *coder)
{
    t_request request;

    request.coder_id = coder->coder_id;
    request.arrival_time = get_time_ms();

    pthread_mutex_lock(&coder->compile_lock);
    request.deadline = coder->last_compile_start
        + coder->shared->args->time_to_burnout;
    pthread_mutex_unlock(&coder->compile_lock);

    pthread_mutex_lock(&dongle->lock);

    heap_push(dongle, request, coder->shared->args->scheduler);

    while ((dongle->heap[0].coder_id != coder->coder_id
            || dongle->is_held
            || get_time_ms() < dongle->available_at)
        && !simulation_stopped(coder->shared))
    {
        pthread_cond_wait(&dongle->cond, &dongle->lock);
    }

    if (simulation_stopped(coder->shared))
    {
        heap_remove(dongle, coder->coder_id);
        pthread_mutex_unlock(&dongle->lock);
        return (-1);
    }

    heap_pop(dongle);
    dongle->is_held = 1;

    pthread_mutex_lock(&coder->shared->log_mutex);
    printf("%lld %d has taken a dongle\n",
        get_time_ms() - coder->shared->start_simulation,
        coder->coder_id);
    pthread_mutex_unlock(&coder->shared->log_mutex);

    pthread_mutex_unlock(&dongle->lock);
    return (0);
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

