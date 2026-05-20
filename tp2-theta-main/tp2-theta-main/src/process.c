#include "process.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>

#include "os.h"

process_t *create_process(int pid) {
    assert(pid >= 0);
    process_t *process = malloc(sizeof(process_t));
    if (!process) {
        return NULL;
    }
    memset(process, 0, sizeof(process_t));

    process->pid = pid;
    process->burst_time = 0;
    process->io_time = 0;
    process->priority_level = DEFAULT_PRIORITY_LEVEL;
    pthread_mutex_init(&process->mutex,NULL);
    return process;
}

void destroy_process(process_t *process) {
    assert(process != NULL);
    pthread_mutex_destroy(&process->mutex);
    free(process);
}
