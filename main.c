#include "codexion.h"

int funct(t_shared *shared)
{
    pthread_t *coder_threads;
    pthread_t monitor_thread;
    int i;
    int j;

    i = 0;
    j = 0;

    coder_threads = malloc((shared->args->nb_coders + 1) * sizeof(pthread_t));
    if (!coder_threads)
    {
        printf("Error, failed to allocate coder_threads.");
        exit(1);
    }

    while (i < shared->args->nb_coders - 1)
    {
        if (pthread_create(&coder_threads[i], NULL, coder_routine, &shared->coders[i]) != 0)
        {
            pthread_mutex_lock(&shared->mutex_stop);
            shared->stop_simulation = 1;
            pthread_mutex_unlock(&shared->mutex_stop);
            while (j < i)
            {
                pthread_join(coder_threads[j], NULL);
                j++;
            }
            free(coder_threads);
            free(shared->coders);
            free(shared->dongle_array);
            pthread_mutex_destroy(&shared->log_mutex);
            free(shared);
            printf("Error failed to create coders.");
            exit(1);

        }
        i++;

    }
    
    
}