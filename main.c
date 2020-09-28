#include <pthread.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#ifndef ITER_CNT
#define ITER_CNT 1000000
#endif

#define ERROR_CODE -1
#define SUCCESS_CODE 0
#define MIN_ARG_CNT 2

void exitWithFailure(const char *msg, int errcode){
    errno = errcode;
    fprintf(stderr, "%.256s:%.256s\n", msg, strerror(errno));
    exit(EXIT_FAILURE);
}

/*
    struct for passing arguments
    to the thread
*/
typedef struct Context{
    size_t  thread_cnt,
            /* thread identifier */
            thread_id;
} Context;


void *routine(void *data){
    Context cntx = *((Context*)(data));

    int ind = cntx.thread_id;
    double sum = 0.0;

    /*
        each thread uses (thread_id + k * thread_cnt)th
        numbers of row
    */
    for (int i = 0; i < ITER_CNT; ++i){
        double denum = (ind) * 2.0 + 1.0;
        if (ind % 2 == 1)
            denum *= -1.0;

        sum += 1.0 / denum;
        ind += cntx.thread_cnt;
    }

    /*
        since sizeof(double) > sizeof(void*)
        for 32-bit machines memory must be
        dynamically allocated
    */
    double *res = (double*)malloc(sizeof(double));
    if (res == NULL)
        exitWithFailure("routine", ENOMEM);

    *res = sum;
    pthread_exit((void*)res);
}

int main(int argc, char **argv){
    char *endptr;
    errno = SUCCESS_CODE;
    const size_t thread_cnt = argc >= MIN_ARG_CNT ? strtol(argv[1], &endptr, 10) : 4;
    if (errno != SUCCESS_CODE)
        exitWithFailure("main", EINVAL);

    pthread_t *pid;
    Context *cntx;

    pid = (pthread_t*)malloc(sizeof(pthread_t) * thread_cnt);
    if (pid == NULL)
        exitWithFailure("main", ENOMEM);
    cntx = (Context*)malloc(sizeof(Context) * thread_cnt);
    if (cntx == NULL)
        exitWithFailure("main", ENOMEM);

    for (int i = 0; i < thread_cnt; ++i){
        cntx[i].thread_cnt = thread_cnt;
        cntx[i].thread_id = i;

        int err = pthread_create(&pid[i], NULL, routine, (void*)(&cntx[i]));
        if (err != SUCCESS_CODE)
            exitWithFailure("main", err);
    }

    double sum = 0.0;
    for (int i = 0; i < thread_cnt; ++i){
        double *part_sum;
        /* wait for threads to terminate */
        int err = pthread_join(pid[i], (void*)&part_sum);
        if (err != SUCCESS_CODE)
            exitWithFailure("main", err);

        sum += *part_sum;
        /* release memory*/
        free(part_sum);
    }

    /* release memory again */
    free(pid);
    free(cntx);

    sum *= 4.0;
    printf("%f\n", sum);

    return 0;
}
