#include "codexion.h"

int init_dongle_array(t_dongle *array, int nb_coders)
{
    int i;
    int j;

    i = 0;
    while (i < nb_coders)
    {
        array[i].dongle_id = i + 1;
        array[i].available_at = 0;
        array[i].is_held = 0;

        if (pthread_mutex_init(&array[i].lock, NULL) != 0)
        {
            j = i - 1;
            while (j >= 0)
            {
                pthread_cond_destroy(&array[j].cond);
                pthread_mutex_destroy(&array[j].lock);
                j--;
            }
            return (-1);
        }

        if (pthread_cond_init(&array[i].cond, NULL) != 0)
        {
            pthread_mutex_destroy(&array[i].lock);

            j = i - 1;
            while (j >= 0)
            {
                pthread_cond_destroy(&array[j].cond);
                pthread_mutex_destroy(&array[j].lock);
                j--;
            }
            return (-1);
        }

        i++;
    }

    return (0);
}

void init_coders(t_coder *coders, t_shared *shared, int nb_coders)
{
    int i;
    i = 1;

    while (i <= nb_coders)
    {
        coders->coder_id = i;
        coders->compiles_done = 0;
        coders->last_compile_start = 0;
        coders->phase = 0;
        coders->shared = shared;
        coders->right_dongle = i;
        coders->left_dongle = i - 1;
        if (i == 1)
            coders->left_dongle = nb_coders;
        coders ++;
        i++;
    }
}

t_shared *init_shared(t_arg *args)
{
    t_shared *shared_args;
    int res_init_dongle_array;

    shared_args = malloc(sizeof(t_shared));

    if (!shared_args)
    {
        printf("Error while allocating memory for shared_args.");
        exit(1);
    }
    shared_args->dongle_array = malloc(sizeof(t_dongle) * args->nb_coders);
    shared_args->args = args;
    if (!shared_args->dongle_array)
    {
        printf("Error while allocating memory for dongle_array.");
        free(shared_args);
        exit(1);
    }

    res_init_dongle_array = init_dongle_array(shared_args->dongle_array, shared_args->args->nb_coders);
    if (res_init_dongle_array != 0)
    {
        printf("Error failed to init the dongle array.");        
        free(shared_args->dongle_array);
        free(shared_args);
        exit(1);
    }

    shared_args->coders = malloc(sizeof(t_coder) * args->nb_coders);
    if (!shared_args->coders)
    {
        printf("Error while allocating memory for coders.");
        free(shared_args);
        exit(1);
    }
    init_coders(shared_args->coders, shared_args, shared_args->args->nb_coders);
    if (pthread_mutex_init(&shared_args->log_mutex, NULL) != 0)
    {
        printf("Error the log mutex failed.");
        free(shared_args->coders);
        free(shared_args->dongle_array);
        free(shared_args);
        exit(1);
    }
    if (pthread_mutex_init(&shared_args->mutex_stop, NULL) != 0)
    {
        printf("Error the stop mutex failed.");
        free(shared_args->coders);
        free(shared_args->dongle_array);
        pthread_mutex_destroy(&shared_args->log_mutex);
        free(shared_args);
        exit(1);
    }
    shared_args->start_simulation = get_time_ms();
    shared_args->stop_simulation = 0;

    return (shared_args);
}