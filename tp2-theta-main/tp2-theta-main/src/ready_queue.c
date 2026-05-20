#include "ready_queue.h"

#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

#include "os.h"

void ready_queue_init(ready_queue_t *queue) {
    // Initialise le mutex et la variable de condition
    pthread_mutex_init(&queue->mutex,NULL);
    pthread_cond_init(&queue->cond,NULL);

    // Parcourt chaque niveau de priorite
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        // Initialise head et tail à NULL et taille à 0
        queue->head[i] = NULL;
        queue->tail[i] = NULL;
        queue->size[i] = 0;

        queue->quantum[i] = INITIAL_QUANTUM << i;
    }
}

void ready_queue_destroy(ready_queue_t *queue) {

    //Verrouille la file
    pthread_mutex_lock(&queue->mutex);

    //Parcourt chaque file de priorite
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {

        // Tete de la file
        node_t *current = queue->head[i];

        while (current != NULL) {
            node_t *next = current->next;

            if (current->process != NULL) {
                destroy_process(current->process);
            }

            // Libere le noeud
            free(current);
            current= next;
        }
        // Quand la file est videe, on met head et tail à NULL et la taille à 0
        queue->head[i] = NULL;
        queue->tail[i] = NULL;
        queue->size[i] = 0;

    }
    
    //Deverrouille la file
    pthread_mutex_unlock(&queue->mutex);

    // Detruit le mutex et la variable de condition
    pthread_mutex_destroy(&queue->mutex);
    pthread_cond_destroy(&queue->cond);
}

void ready_queue_push(ready_queue_t *queue, process_t *process) {
    // Allocation d'un nouveau noeud
    node_t *node = malloc(sizeof(node_t));

    // Si allocation echoue
    if (node == NULL) {
        fprintf(stderr, "ready_queue_push: malloc failed\n");
        return;
    }

    // Stocke le processus
    node->process = process;

    // next est NULL vu qu'on l'ajoute à la fin de la file
    node->next = NULL;

    // Si le processus est NULL
    int level = (process == NULL) ? NUM_PRIORITY_LEVELS - 1 : process->priority_level;

    if (level < 0) level = 0;
    if (level >= NUM_PRIORITY_LEVELS) level = NUM_PRIORITY_LEVELS - 1;

    //Verrouille la file
    pthread_mutex_lock(&queue->mutex);

    // Si file n'est pas vide
    if (queue->tail[level])
        // Ajoute noeud apres l'ancien tail
        queue->tail[level]->next = node;
    else // Sinon
        // Noeud devient le head et le tail
        queue->head[level] = node;

    // Mise a jour de tail et de la taille
    queue->tail[level] = node;
    queue->size[level]++;

    // Signal et deverrouille la file
    pthread_cond_signal(&queue->cond);
    pthread_mutex_unlock(&queue->mutex);
}

process_t *ready_queue_pop(ready_queue_t *queue) {
    //Verrouille la file
    pthread_mutex_lock(&queue->mutex);

    while (1) {
        // Parcourt les files de priorite
        for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
            // Si la file n'est pas vide
            if (queue->size[i] > 0) {

                // Prend le head de la file
                node_t *node = queue->head[i];

                // Prend le processus
                process_t *process = node->process;

                // Avance le head de la file
                queue->head[i] = node->next;

                // Si la file est vide
                if (queue->head[i] == NULL)
                    // Tail est NULL
                    queue->tail[i] = NULL;

                // Decremente la taille de la file
                queue->size[i]--;

                // Libere le noeud
                free(node);

                // Deverrouille la file
                pthread_mutex_unlock(&queue->mutex);

                return process;
            }
        }

        pthread_cond_wait(&queue->cond, &queue->mutex);
    }
}

size_t ready_queue_size(ready_queue_t *queue) {
    //Verrouille la file
    pthread_mutex_lock(&queue->mutex);
    
    size_t size = 0;

    // Somme des tailles de toutes les files de priorite
    for (int i = 0; i < NUM_PRIORITY_LEVELS; i++) {
        size += queue->size[i];
    }

    //Deverrouille la file
    pthread_mutex_unlock(&queue->mutex);
    return size;
}
