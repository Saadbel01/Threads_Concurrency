#include "codexion.h"


void take_dongle(t_dongle *dongle, int coder_id)
{
    pthread_mutex_lock(&dongle->lock);
    while (dongle->is_held == 1
            || get_time_ms() < dongle->available_at)
    {
        pthread_cond_wait(&dongle->cond, &dongle->lock);
    }
    dongle->is_held = 1;
    printf("Coder %d has taken dongle %d.\n", coder_id, dongle->dongle_id);
    pthread_mutex_unlock(&dongle->lock);
    
}

void    acquire_dongles(t_coder *coder)
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

    take_dongle(first_dongle, coder->coder_id);
    take_dongle(second_dongle, coder->coder_id);

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

