#include "codexion.h"

void    acquire_dongles(t_coder *coder)
{
    t_dongle   *first_dongle;
    t_dongle   *second_dongle;
    int         second_acquired;

    second_acquired = 0;

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

    pthread_mutex_lock(&first_dongle->lock);

    if (first_dongle->is_held == 0
        && get_time_ms() >= first_dongle->available_at)
    {
        first_dongle->is_held = 1;
        printf("Coder %d has taken a dongle.\n", coder->coder_id);
    }
    else
    {
        pthread_mutex_unlock(&first_dongle->lock);
        return;
    }

    pthread_mutex_unlock(&first_dongle->lock);

    pthread_mutex_lock(&second_dongle->lock);

    if (second_dongle->is_held == 0
        && get_time_ms() >= second_dongle->available_at)
    {
        second_dongle->is_held = 1;
        second_acquired = 1;
        printf("Coder %d has taken a dongle.\n", coder->coder_id);
    }

    pthread_mutex_unlock(&second_dongle->lock);

    if (second_acquired == 0)
    {
        pthread_mutex_lock(&first_dongle->lock);
        first_dongle->is_held = 0;
        pthread_mutex_unlock(&first_dongle->lock);
    }
}

