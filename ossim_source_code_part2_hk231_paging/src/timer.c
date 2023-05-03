
#include "timer.h"
#include <stdio.h>
#include <stdlib.h>

static pthread_t _timer; //// the thread which does the job of the timer

struct timer_id_container_t { //// data structure for a node in the linked list
	struct timer_id_t id;
	struct timer_id_container_t * next;
};

static struct timer_id_container_t * dev_list = NULL; //// a linked list of states for round-robin

static uint64_t _time;

static int timer_started = 0; //// not started yet
static int timer_stop = 0; //// not stopped yet


static void * timer_routine(void * args) {
	while (!timer_stop) {
		printf("Time slot %3lu\n", current_time());
		int fsh = 0;
		int event = 0;
		/* Wait for all devices have done the job in current
		 * time slot */
		struct timer_id_container_t * temp;
		for (temp = dev_list; temp != NULL; temp = temp->next) {
			pthread_mutex_lock(&temp->id.event_lock);
			while (!temp->id.done && !temp->id.fsh) { //// if time slot has not ended yet and the job has not finished -> wait
				pthread_cond_wait(
					&temp->id.event_cond,
					&temp->id.event_lock
				);
			}
			if (temp->id.fsh) {
				fsh++; //// if the task has finished, increase the number of finished tasks
			}
			event++;
			pthread_mutex_unlock(&temp->id.event_lock);
		}

		/* Increase the time slot */
		_time++;
		
		/* Let devices continue their job */
		for (temp = dev_list; temp != NULL; temp = temp->next) {
			pthread_mutex_lock(&temp->id.timer_lock);
			temp->id.done = 0; //// give the job time slot for round-robin
			pthread_cond_signal(&temp->id.timer_cond);
			pthread_mutex_unlock(&temp->id.timer_lock);
		}
		if (fsh == event) {
			break; //// all devices have finished their jobs -> end
		}
	}
	pthread_exit(args);
}

void next_slot(struct timer_id_t * timer_id) {
	/* Tell timer that we have done our job in current slot */
	pthread_mutex_lock(&timer_id->event_lock);
	timer_id->done = 1; //// time slot of round-robin for the current job has ended
	pthread_cond_signal(&timer_id->event_cond);
	pthread_mutex_unlock(&timer_id->event_lock);

	/* Wait for going to next slot */
	pthread_mutex_lock(&timer_id->timer_lock);
	while (timer_id->done) { //// wait for its next turn in round-robin
		pthread_cond_wait(
			&timer_id->timer_cond,
			&timer_id->timer_lock
		);
	}
	pthread_mutex_unlock(&timer_id->timer_lock);
}

uint64_t current_time() {
	return _time;
}

void start_timer() {
	timer_started = 1; //// start the timer and run each task in round-robin
	pthread_create(&_timer, NULL, timer_routine, NULL);
}

void detach_event(struct timer_id_t * event) {
	pthread_mutex_lock(&event->event_lock);
	event->fsh = 1; //// the task has finished
	pthread_cond_signal(&event->event_cond);
	pthread_mutex_unlock(&event->event_lock);
}

struct timer_id_t * attach_event() {
	if (timer_started) { //// if timer has not started, do nothing
		return NULL;
	}else{
		struct timer_id_container_t * container =
			(struct timer_id_container_t*)malloc(
				sizeof(struct timer_id_container_t)		
			); //// create a new timer object
		container->id.done = 0; //// the new object has not done its turn in round-robin
		container->id.fsh = 0; //// the new object has not finished its job
		pthread_cond_init(&container->id.event_cond, NULL); // initialise mutex 
		pthread_mutex_init(&container->id.event_lock, NULL);
		pthread_cond_init(&container->id.timer_cond, NULL);
		pthread_mutex_init(&container->id.timer_lock, NULL);
		if (dev_list == NULL) { //// add the new timer object to the linked list
			dev_list = container;
			dev_list->next = NULL;
		}else{
			container->next = dev_list;
			dev_list = container;
		}
		return &(container->id); //// return the address of the current timer object
	}
}

void stop_timer() {
	timer_stop = 1; //// flag for timer to stop
	pthread_join(_timer, NULL); // wait for the timer thread to finished
	while (dev_list != NULL) { // delete all elements in the linked list
		struct timer_id_container_t * temp = dev_list;
		dev_list = dev_list->next;
		pthread_cond_destroy(&temp->id.event_cond);
		pthread_mutex_destroy(&temp->id.event_lock);
		pthread_cond_destroy(&temp->id.timer_cond);
		pthread_mutex_destroy(&temp->id.timer_lock);
		free(temp);
	}
}




