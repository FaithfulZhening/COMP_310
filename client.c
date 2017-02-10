#include "common.h"
#include <unistd.h>
#include <time.h>

int fd; //this is the file descriptor
Shared* shared_mem; //this is the pointer to the shared memory
FILE *fp;

/*
 *funcion:setup_shared_mem
 *--------------------------------------------------
 *setup a shared memory
 *otherwise open the object for access
 *assign fd the file descriptor shm_open returns
 */
int setup_shared_mem(){
    int erro;// this is the erro msg
    fd = shm_open(MY_SHM, O_RDWR, 0666);
    if(fd == -1){
        printf("shm_open() failed\n");
        exit(1);
    }
}
/*
 *funcion:attach_shared_mem
 *--------------------------------------------------
 *Attach to the shared memory created by setup_shared_mem()
 *Use mmap to map shared_mem into shared memory
 *
 *retun 0 on success
 */
int attach_shared_mem(){
    /*Map shared_mem to shaerd memory*/
    shared_mem = (Shared*) mmap(NULL, sizeof(Shared), PROT_READ | PROT_WRITE, MAP_SHARED,fd,0);
    /*If the mmap fails, exit*/
    if(shared_mem == MAP_FAILED){
        printf("mmap() failed\n");
        exit(1);
    }
    return 0;
}

/*
 *funcion:get_params_and_create
 *--------------------------------------------------
 *Ask user to input a job source
 *and create a job with the randomly generated params
 *
 *retun the job created with the params
 */
Job get_params_and_create(){
    int duration;
    int pages;
    int source_num;
    Job job_created;
    srand(time(NULL));
    duration = rand() % 10;
    pages = rand() % 15;
    printf("What's your source number?\n");
    scanf("%d",&source_num);
    sem_wait(&shared_mem->mutex);
    job_created.pages = pages;
    job_created.duration = duration;
    job_created.source = source_num;
    job_created.client_ID = shared_mem->client_ID;
    sem_post(&shared_mem->mutex); 
    return job_created;
}

/*
*function:put_a_job
*--------------------------------------------------
*Put the job into shared job array
*increment job counter by 1
*
*job_put: the job to be put in array
*shared: the shared memory
*
*/
int put_a_job(Job job_put){
    Job* jobs_shared;
    /*Get handle to shared memory*/
    jobs_shared = shared_mem->jobs;
    sem_wait(&shared_mem->empty); // wait empty
    sem_wait(&shared_mem->mutex); // wait mutex to protect CS
    /*put job into shared jobs array
    *increment it by 1
    */
    jobs_shared[shared_mem->counter] = job_put;
    printf("Client %d has %d pages to print from source %d, puts request in Buffer\n",
        job_put.source, job_put.pages, shared_mem->client_ID );
    fprintf(fp,"Client %d has %d pages to print from source %d, puts request in Buffer\n",
        job_put.source, job_put.pages, shared_mem->client_ID++ );
    shared_mem->counter++;
    sem_post(&shared_mem->mutex); // signal mutex
    sem_post(&shared_mem->full);  // signal full
}

/*
*function:release_shared_memory
*--------------------------------------------------
*release shared memory
*/
int release_shared_memory(){
    munmap(shared_mem,sizeof(Shared));
}

int main(void){
    fp = fopen("trace.txt", "a");
    int client_number;
    Job job_to_put;
    setup_shared_mem();
    attach_shared_mem();
    job_to_put = get_params_and_create();
    put_a_job(job_to_put);
    fclose(fp);
    release_shared_memory();
}