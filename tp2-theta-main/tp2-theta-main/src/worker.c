#include "worker.h"

#include <stdio.h>
#include <stdlib.h>

#include "process.h"
#include "os.h"

void *worker_run(void *user_data) {

    worker_t *worker = (worker_t *) user_data;
    int core = worker->core;
    ready_queue_t *ready_queue = worker->ready_queue;

    for (;;) {
        process_t *process = ready_queue_pop(ready_queue);

        if (process == NULL) break; // No more processes (poison pill)

        int level = process->priority_level;

        if (level < 0) level = 0;
        if (level >= NUM_PRIORITY_LEVELS) level = NUM_PRIORITY_LEVELS - 1;

        uint64_t quantum = ready_queue->quantum[level];

        int status = os_run_process(process,core, quantum);

        if (status == OS_RUN_PREEMPTED) {
            if (process->priority_level < NUM_PRIORITY_LEVELS - 1) {
                // Descend d'un niveau
                process->priority_level++;
            }
            // Remet dans la file
            ready_queue_push(ready_queue,process);
        } else if (status == OS_RUN_BLOCKED) {
            // Met a niveau 0
            process->priority_level = 0;
            // Simule une I/O
            os_start_io(process);
        } else if (status == OS_RUN_DONE) {
            // Termine
        }
    }

    return NULL;
}

worker_t *worker_create(int core, ready_queue_t *ready_queue) {
    worker_t *worker = malloc(sizeof(worker_t));

    // Si l'allocation echoue
    if (worker == NULL) {
        fprintf(stderr, "worker_create: malloc failed\n");
        return NULL;
    }

    worker->core = core;
    worker->ready_queue = ready_queue;

    //verification de pthread_create
    if (pthread_create(&worker->thread, NULL, worker_run, worker) != 0) {
        fprintf(stderr, "worker_create: pthread_create failed\n");
        free(worker);
        return NULL;
    }

    return worker;
}

void worker_destroy(worker_t *worker) {
    // Libere le worker
    free(worker);
}

void worker_join(worker_t *worker) {

    pthread_join(worker->thread,NULL);
}
