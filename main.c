#include "codexion.h"


void cleanup_shared(t_shared    *shared)
{
    int i;

    i = 0;
    while (i < shared->args->nb_coders)
    {
        pthread_mutex_destroy(&shared->dongle_array[i].lock);
        pthread_cond_destroy(&shared->dongle_array[i].cond);
        i++;
    }
    i = 0;
    while (i < shared->args->nb_coders)
    {
        pthread_mutex_destroy(&shared->coders[i].compile_lock);
        i++;
    }
    pthread_mutex_destroy(&shared->mutex_stop);
    pthread_mutex_destroy(&shared->log_mutex);
    free(shared->coders);
    free(shared->dongle_array);
    free(shared);
}

int main(int argc, char **argv)
{
    t_arg   args;
    t_shared    *shared;
    pthread_t *coder_threads;
    pthread_t monitor_thread;
    int i;
    int j;
    i = 0;
    j = 0;


    check_arguments(argc, argv);
    get_arguments(argv, &args);
    shared = init_shared(&args);
    coder_threads = malloc(shared->args->nb_coders * sizeof(pthread_t));
    if (!coder_threads)
    {
        printf("Error, failed to allocate coder_threads.");
        exit(1);
    }
    while (i < shared->args->nb_coders)
    {
        if (pthread_create(&coder_threads[i], NULL, coder_routine, &shared->coders[i]) != 0)
        {
            pthread_mutex_lock(&shared->mutex_stop);
            shared->stop_simulation = 1;
            pthread_mutex_unlock(&shared->mutex_stop);
            wake_all_dongles(shared);
            while (j < i)
            {
                pthread_join(coder_threads[j], NULL);
                j++;
            }
            cleanup_shared(shared);
            printf("Error failed to create coders\n");
            exit(1);
        }
        i++;
    }
    pthread_create(&monitor_thread, NULL, monitor_routine, shared);
    i = 0;
    while (i < shared->args->nb_coders)
    {
        pthread_join(coder_threads[i], NULL);
        i++;
    }
    pthread_join(monitor_thread, NULL);
    cleanup_shared(shared);
    free(coder_threads);
    return (0);

}