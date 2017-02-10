#include "common.h"
#include <unistd.h>

int fd;	//this is the file descriptor
Shared* shared_mem;	//this is the pointer to the shared memory
int input_buffer_size;//this is the user input buffer size
FILE *fp;


/*
 *funcion:sighandler
 *--------------------------------------------------
 *When a printer is closed,check if it is the last active printer
 *If is, close the shared memory
 */
void sighandler(int signum)
{	

   shared_mem->active_printers--;
   /*unlink if it is the last running printer*/
   if (shared_mem->active_printers == 0){
        printf("This is the last active printer, unlink shared memory\n");
   		shm_unlink(MY_SHM);
   }
   exit(1);
}
/*
 *funcion:setup_shared_mem
 *--------------------------------------------------
 *setup a shared memory,if it does't exist, create it
 *otherwise open the object for access
 *assign fd the file descriptor shm_open returns
 */
int setup_shared_mem(){
	int erro;// this is the erro msg
	fd = shm_open(MY_SHM, O_CREAT | O_RDWR, 0666);
	if(fd == -1){
        printf("shm_open() failed\n");
        exit(1);
    }
    /*truncate the shared memory referenced by fd
     * to the size of Shared*/
    erro = ftruncate(fd, sizeof(Shared));
    if(erro == -1){
    	printf("ftruncate failed\n");
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
 *funciton:init_semaphore
 *--------------------------------------------------
 *Initialize the semaphores: empty,full and mutex
 *Initialize empty to 10(if the buffer size is 10)
 *Initialize full to 0
 *Initialize mutext to 1 
 *
 *buffer_size: the size of buffer of job array
 */
int init_semaphore(int buffer_size){
	sem_init(&(shared_mem->empty),1,buffer_size-1);
	sem_init(&(shared_mem->full),1,0);
	sem_init(&(shared_mem->mutex),1,1);
}

/*
 *funciton:take_a_job
 *--------------------------------------------------
 *Take a job from the job array in shared memory
 *Block if no job
 *Protect the critical section.
 *Wait full and mutex before taking the job
 *Signal empty and mutex after taking the job
 *Return the first job, then move every job 1 slot forward
 *
 *shared: the shared memory
 *
 *return: the first job in the jobs array in shared memory
 */
Job take_a_job(){
    Job job_taken;
    int job_counter;
    Job* jobs_shared;
    sem_wait(&shared_mem->full); // wait full
    sem_wait(&shared_mem->mutex); // wait mutex
    job_counter = shared_mem->counter;
    /*Get handle to shared memory*/
    jobs_shared = shared_mem->jobs;
    job_taken = jobs_shared[0];
    for (int i = 0; i < job_counter-1; i++){
        jobs_shared[i] = jobs_shared[i+1]; 
    }
    shared_mem->counter--;
    sem_post(&shared_mem->mutex);
    sem_post(&shared_mem->empty);
    return job_taken;
}

/*
 *funciton:print_a_msg
 *--------------------------------------------------
 *print a starting message
 *
 *job_print: the job to be print
 *printer_number: the number of current printer
 */
int print_a_msg(Job job_print,int printer_number){
	int page_number = job_print.pages;
	int client_number = job_print.client_ID;
    int source = job_print.source;
	printf("Printer %d starts printing %d pages from client %d, source %d\n", 
		printer_number, page_number, client_number, source);
    fprintf(fp,"Printer %d starts printing %d pages from client %d, source %d\n", 
        printer_number, page_number, client_number, source);
}

/*
 *funciton:go_sleep
 *--------------------------------------------------
 *Sleep for job duration. Print a finish message when wake up
 *
 *job_taken: the job to be deal with.
 *printer_number: the number of current printer
 */
int go_sleep(Job job_taken,int printer_number){
	int sleep_time = job_taken.duration;
	int page_number = job_taken.pages;
	int client_number = job_taken.source;
	sleep(sleep_time);
	printf("Printer %d finishes printing %d pages from client %d, source %d\n", 
		printer_number, page_number, client_number, job_taken.source);
    fprintf(fp,"Printer %d finishes printing %d pages from client %d, source %d\n", 
        printer_number, page_number, client_number, job_taken.source);
}

int main(void){
	setup_shared_mem();
    attach_shared_mem();
    /*uese to unlink if crashed, comment next line when running*/
    //shm_unlink(MY_SHM);
    signal(SIGINT, sighandler);
    /*If this is the first printer, we initialize the semaphores*/
    if (shared_mem->printer_counter==0){
        printf("Pleas enter the buffer size(max 256 and min 2)\n");
        scanf("%d",&input_buffer_size);
        /*overwrite the trace.txt if it is the first printer*/
        fp = fopen("trace.txt", "w");
        fclose(fp);
        if ((input_buffer_size<=256) && (input_buffer_size>1)){
            init_semaphore(input_buffer_size);
            /*Protect shared memory using mutex*/
            sem_wait(&shared_mem->mutex);
            shared_mem->printer_counter++;
            shared_mem->active_printers = 1;
            sem_post(&shared_mem->mutex);
        }
        else{
            printf("invalid buffer size\n");
            exit(0);        
        }
    }
    else{
    	/*
    	 *Increment printer counter and active printer counter
    	 *Protect shared memory using mutex
    	 */
    	sem_wait(&shared_mem->mutex);
    	shared_mem->printer_counter ++;
    	shared_mem->active_printers ++;
    	sem_post(&shared_mem->mutex);
    }

    int printer_number = shared_mem->printer_counter-1;
    signal(SIGINT, sighandler);
   while(1){
   		/*if ( shared_mem->counter == 0){
			printf("No request in buffer, Printer sleeps\n");
   		}*/
		fp = fopen("trace.txt", "a");	
    	Job job_to_do;
    	job_to_do = take_a_job(shared_mem);
    	print_a_msg(job_to_do, printer_number);
    	go_sleep(job_to_do, printer_number);
        fclose(fp);	
    }
}