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

    take_dongle(first_dongle, coder->coder_id);
    take_dongle(second_dongle, coder->coder_id);

}

