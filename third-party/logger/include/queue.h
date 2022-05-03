/**
 * @file   queue.h
 * @brief  Queue implementation
 * @author Bram Vlerick <bram.vlerick@openpixelsystems.org>
 * @date   Mon May 13 15:48:56 2019
 */

#ifndef _QUEUE_H_
#define _QUEUE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifndef CFG_LOGGER_DEEP_EMBEDDED
#include <pthread.h>
#endif /* CFG_LOGGER_DEEP_EMBEDDED */


#include "logger-wrapper.h"

/** Typedef for destroy function pointer */
typedef void (*queue_destroy_t)(void *data);

/* --------------------------------------------------------------------------*/
/**
 * @brief  Message queue element structure
 * Used to store data in the queue
 */
/* --------------------------------------------------------------------------*/
struct queue_elm_t {
	int			id;     //!< Psuedo unique id for element
	struct queue_elm_t *	next;   //!< Ptr to next element
	struct queue_elm_t *	prev;   //!< Ptr to previous element
	void *			data;   //!< Ptr to actual data
};


/* --------------------------------------------------------------------------*/
/**
 * @brief  Queue structure
 * Structure contain vital queue data
 */
/* --------------------------------------------------------------------------*/
struct queue_t {
	int			n_elements;     //!< Number of elements in the queue
	struct queue_elm_t *	head;           //!< Ptr to ::queue_elm_t:: that's the head of the queue
	struct queue_elm_t *	tail;           //!< Ptr to ::queue_elm_t:: that's the tail of the queue
	queue_destroy_t		destroy;        //!< Callback for deleting data
#ifndef CFG_LOGGER_DEEP_EMBEDDED
	pthread_mutex_t		mutex;          //!< Mutex to protect the queue
#endif /* CFG_LOGGER_DEEP_EMBEDDED */
};

/** Retrieve the size of a given queue */
#define QUEUE_SIZE(x) (x)->n_elements

/**
 * @brief Create new queue
 *
 * @param destroy Destroy callback called when queue is removed
 *
 * @return
 */
struct queue_t *queue_create(queue_destroy_t destroy);

/**
 * @brief Push some data to a queue
 *
 * @param q Queue to which the data will be pushed
 * @param data Data that will be stored
 *
 * @return -1 if failed otherwise 0
 */
int queue_push(struct queue_t *q, void *data);

/**
 * @brief Pop data from the queue
 *
 * @param q Queue from which data will be pop'd
 * @param data Pointer to the address of the stored data
 *
 * @return -1 if failed otherwise 0
 */
int queue_pop(struct queue_t *q, void **data);

/**
 * @brief Remove a given queue
 *
 * @param q Queue that will be removed
 */
void queue_destroy(struct queue_t *q);

#endif
