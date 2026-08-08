#include "codexion.h"


void    compling(t_coder    *coder)
{
    pthread_mutex_lock(&coder->shared->log_mutex);
    printf("%ld %d is compiling\n", get_time_ms() - coder->shared->start_simulation,coder->coder_id);
    pthread_mutex_unlock(&coder->shared->log_mutex);
    usleep(coder->shared->args->time_to_compile * 1000);
}



void    debugging(t_coder    *coder)
{
    pthread_mutex_lock(&coder->shared->log_mutex);
    printf("%ld %d is debugging\n",get_time_ms() - coder->shared->start_simulation, coder->coder_id);
    pthread_mutex_unlock(&coder->shared->log_mutex);
    usleep(coder->shared->args->time_to_debug * 1000);
}


void    refactoring(t_coder    *coder)
{
    pthread_mutex_lock(&coder->shared->log_mutex);
    printf("%ld %d is refactoring\n", get_time_ms() - coder->shared->start_simulation, coder->coder_id);
    pthread_mutex_unlock(&coder->shared->log_mutex);
    usleep(coder->shared->args->time_to_refactor * 1000);
}


void    *coder_routine(void *arg)
{
    int stop;
    t_coder *coder;
    coder = (t_coder *)arg;

    while (1)
    {
        acquire_dongles(coder);
        pthread_mutex_lock(&coder->compile_lock);
        coder->last_compile_start = get_time_ms() ;
        pthread_mutex_unlock(&coder->compile_lock);
        
        compling(coder);
        release_dongles(coder);
        
        pthread_mutex_lock(&coder->compile_lock);
        coder->compiles_done += 1;
        pthread_mutex_unlock(&coder->compile_lock);

        pthread_mutex_lock(&coder->shared->mutex_stop);
        stop = coder->shared->stop_simulation;
        pthread_mutex_unlock(&coder->shared->mutex_stop);

        if (stop)
            break;

        debugging(coder);

        pthread_mutex_lock(&coder->shared->mutex_stop);
        stop = coder->shared->stop_simulation;
        pthread_mutex_unlock(&coder->shared->mutex_stop);

        if (stop)
            break;

        refactoring(coder);

        pthread_mutex_lock(&coder->shared->mutex_stop);
        stop = coder->shared->stop_simulation;
        pthread_mutex_unlock(&coder->shared->mutex_stop);

        if (stop)
            break;
    }
    return (NULL);
}