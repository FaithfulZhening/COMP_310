#ifndef _INCLUDE_COMMON_H_
#define _INCLUDE_COMMON_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

// from `man shm_open`
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <signal.h>
#include <semaphore.h>

#define MY_SHM "/PRINT_SHM"
#define MAX_SIZE 256

typedef struct {
	int pages;
	int duration;
	int source;
    int client_ID;
}Job;

typedef struct {
    sem_t empty; //counts the empty slots
    sem_t full;  //counts full buffer slots
    sem_t mutex; // protects the critical section
    int counter; //Jobs ID counter
    int printer_counter;//check if it is the first printer
    int active_printers;// Counter of active printers;
    int client_ID; // Make sure no two clients have the smae ID
    Job jobs[MAX_SIZE];   
} Shared;	

#endif //_INCLUDE_COMMON_H_
