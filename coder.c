#include "codexion.h"


void    compling(t_coder    *coder)
{
    pthread_mutex_lock(&coder->shared->log_mutex);
    printf("%lld %d is compiling\n", get_time_ms() - coder->shared->start_simulation,coder->coder_id);
    pthread_mutex_unlock(&coder->shared->log_mutex);
    usleep(coder->shared->args->time_to_compile * 1000);
}



void    debugging(t_coder    *coder)
{
    pthread_mutex_lock(&coder->shared->log_mutex);
    printf("%lld %d is debugging\n",get_time_ms() - coder->shared->start_simulation, coder->coder_id);
    pthread_mutex_unlock(&coder->shared->log_mutex);
    usleep(coder->shared->args->time_to_debug * 1000);
}


void    refactoring(t_coder    *coder)
{
    pthread_mutex_lock(&coder->shared->log_mutex);
    printf("%lld %d is refactoring\n", get_time_ms() - coder->shared->start_simulation, coder->coder_id);
    pthread_mutex_unlock(&coder->shared->log_mutex);
    usleep(coder->shared->args->time_to_refactor * 1000);
}


void    *coder_routine(void *arg)
{
    int stop;
    int compiles_done;
    t_coder *coder;
    coder = (t_coder *)arg;


    while (1)
    {
        pthread_mutex_lock(&coder->compile_lock);
        compiles_done = coder->compiles_done;
        pthread_mutex_unlock(&coder->compile_lock);
        if (compiles_done >= coder->shared->args->nb_of_compiles)
            break;
        if (acquire_dongles(coder) == -1)
            break;
        pthread_mutex_lock(&coder->compile_lock);
        coder->last_compile_start = get_time_ms() ;
        pthread_mutex_unlock(&coder->compile_lock);
        
        compling(coder);
        release_dongles(coder);

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

        pthread_mutex_lock(&coder->compile_lock);
        coder->compiles_done += 1;
        pthread_mutex_unlock(&coder->compile_lock);

        pthread_mutex_lock(&coder->shared->mutex_stop);
        stop = coder->shared->stop_simulation;
        pthread_mutex_unlock(&coder->shared->mutex_stop);

        if (stop)
            break;
    }
    return (NULL);
}

void wake_all_dongles(t_shared *shared)
{
    int i;

    i = 0;
    while (i < shared->args->nb_coders)
    {
        pthread_mutex_lock(&shared->dongle_array[i].lock);
        pthread_cond_broadcast(&shared->dongle_array[i].cond);
        pthread_mutex_unlock(&shared->dongle_array[i].lock);
        i++;
    }
}

void    *monitor_routine(void   *arg)
{
    int i;
    long long last_compile;
    int compiles_done;
    int burnout;
    int coder_done;
    int burnout_id;
    t_shared *shared;

    shared = (t_shared *)arg;
    
    while (1)
    {
        i = 0;
        burnout = 0;
        coder_done = 1;
        
        while (i < shared->args->nb_coders)
        {
            pthread_mutex_lock(&shared->coders[i].compile_lock);
            last_compile = shared->coders[i].last_compile_start;
            compiles_done = shared->coders[i].compiles_done;
            pthread_mutex_unlock(&shared->coders[i].compile_lock);
            if (compiles_done < shared->args->nb_of_compiles)
            {
                if (get_time_ms() - last_compile > shared->args->time_to_burnout)
                {
                    burnout = 1;
                    burnout_id = shared->coders[i].coder_id;
                }
                coder_done = 0;
            }
            i++;
        }
        if (burnout == 1)
        {
            pthread_mutex_lock(&shared->log_mutex);
            printf("%lld %d burned out\n", get_time_ms() - shared->start_simulation, burnout_id);
            pthread_mutex_unlock(&shared->log_mutex);
            
            pthread_mutex_lock(&shared->mutex_stop);
            shared->stop_simulation = 1;
            pthread_mutex_unlock(&shared->mutex_stop);
            wake_all_dongles(shared);
            return (NULL);
        }
        else if (coder_done == 1)
        {
            pthread_mutex_lock(&shared->mutex_stop);
            shared->stop_simulation = 1;
            pthread_mutex_unlock(&shared->mutex_stop);
            wake_all_dongles(shared);
            return (NULL);
        }
        usleep(1000);
    }
}