#include "codexion.h"


int take_dongle(t_dongle *dongle, t_coder  *coder)
{
    int stop;
    long long wait_ms;
    struct timespec timeout;

    // printf("DEBUG: coder %d entering take_dongle for dongle %d\n", coder->coder_id, dongle->dongle_id); fflush(stdout);
    pthread_mutex_lock(&dongle->lock);
    // printf("DEBUG: coder %d locked dongle %d\n", coder->coder_id, dongle->dongle_id); fflush(stdout);

    // printf("DEBUG: coder %d about to lock mutex_stop\n", coder->coder_id); fflush(stdout);
    pthread_mutex_lock(&coder->shared->mutex_stop);
    // printf("DEBUG: coder %d locked mutex_stop\n", coder->coder_id); fflush(stdout);
    stop = coder->shared->stop_simulation;
    pthread_mutex_unlock(&coder->shared->mutex_stop);
    // printf("DEBUG: coder %d read stop=%d, checking while condition\n", coder->coder_id, stop); fflush(stdout);

    while ((dongle->is_held == 1
            || get_time_ms() < dongle->available_at)
        && stop == 0)
    {
        if (dongle->is_held == 1)
        {
            // printf("DEBUG: coder %d entering cond_wait for dongle %d\n", coder->coder_id, dongle->dongle_id); fflush(stdout);
            pthread_cond_wait(&dongle->cond, &dongle->lock);
        }
        else
        {
            wait_ms = dongle->available_at - get_time_ms();
            if (wait_ms > 0)
            {
                clock_gettime(CLOCK_REALTIME, &timeout);
                timeout.tv_sec += wait_ms / 1000;
                timeout.tv_nsec += (wait_ms % 1000) * 1000000L;
                if (timeout.tv_nsec >= 1000000000L)
                {
                    timeout.tv_sec += 1;
                    timeout.tv_nsec -= 1000000000L;
                }
                pthread_cond_timedwait(&dongle->cond, &dongle->lock, &timeout);
            }
        }
        // printf("DEBUG: coder %d woke from cond_wait\n", coder->coder_id); fflush(stdout);
        pthread_mutex_lock(&coder->shared->mutex_stop);
        stop = coder->shared->stop_simulation;
        pthread_mutex_unlock(&coder->shared->mutex_stop);
    }

    // printf("DEBUG: coder %d exited while loop, is_held=%d, stop=%d\n", coder->coder_id, dongle->is_held, stop); fflush(stdout);

    if (stop == 1)
    {
        pthread_mutex_unlock(&dongle->lock);
        return (-1);
    }
    else
    {
        dongle->is_held = 1;
        // printf("DEBUG: coder %d about to lock log_mutex\n", coder->coder_id); fflush(stdout);
        pthread_mutex_lock(&coder->shared->log_mutex);
        // printf("DEBUG: coder %d locked log_mutex\n", coder->coder_id); fflush(stdout);
        printf("%lld %d has taken a dongle\n", get_time_ms() - coder->shared->start_simulation, coder->coder_id);
        fflush(stdout);
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

