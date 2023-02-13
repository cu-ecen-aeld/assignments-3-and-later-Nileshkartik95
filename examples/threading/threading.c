/*****************************************************************************
​* Copyright​ ​(C)​ ​2023 ​by​ ​Nileshkartik Ashokkumar
​*
​* ​​Redistribution,​ ​modification​ ​or​ ​use​ ​of​ ​this​ ​software​ ​in​ ​source​ ​or​ ​binary
​* ​​forms​ ​is​ ​permitted​ ​as​ ​long​ ​as​ ​the​ ​files​ ​maintain​ ​this​ ​copyright.​ ​Users​ ​are
​​* ​permitted​ ​to​ ​modify​ ​this​ ​and​ ​use​ ​it​ ​to​ ​learn​ ​about​ ​the​ ​field​ ​of​ ​embedded
​* software.​ Nileshkartik Ashokkumar ​and​ ​the​ ​University​ ​of​ ​Colorado​ ​are​ ​not​ ​liable​ ​for
​​* ​any​ ​misuse​ ​of​ ​this​ ​material.
​*
*****************************************************************************
​​*​ ​@file​ ​threading.c
​​*​ ​@brief ​ functions called from the test file to test if the start_thread_obtaining_mutex creates threads perform the actions specified by the thread
* thread performs following action: wait for x ms to lock the mutex, lock the mutex, wait for x ms to release the mutex and unlock the mutex with the status success
​​* uninitialized or unintended access to threadfunc should return thread_complete_success as false
*
​​*
​​*​ ​@author​ ​Nileshkartik Ashokkumar
​​*​ ​@date​ ​Feb​ ​8​ ​2023
​*​ ​@version​ ​1.0
​*
*/

#include "threading.h"
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>

#define RES_MS     (1000)
// Optional: use these functions to add debug or error prints to your application
#define DEBUG_LOG(msg,...)
//#define DEBUG_LOG(msg,...) printf("threading: " msg "\n" , ##__VA_ARGS__)
#define ERROR_LOG(msg,...) printf("threading ERROR: " msg "\n" , ##__VA_ARGS__)

void* threadfunc(void* thread_param)
{

    struct thread_data* thread_func_args = (struct thread_data *)thread_param;
    thread_func_args->thread_complete_success = false;               			/*thread status initialization*/
    if(thread_func_args != NULL)
    {
        if(usleep(thread_func_args->wait_to_obtain_ms*RES_MS)!= 0)             		/*wait for wait_to_obtain_ms*/
	{
		perror("usleep");                       				/*redirecting the failure to stderr*/
        	ERROR_LOG("usleep");     						/*redirecting the failure to stdout*/
		return thread_param;
	}
        if(pthread_mutex_lock(thread_func_args->lock)!= 0)                     		/*lock the mutex*/
	{
                perror("mutex lock failure");                      		 	/*redirecting the failure to stderr*/
                ERROR_LOG("mutex lock failure");			     		/*redirecting the failure to stdout*/
                return thread_param;
        }
        if(usleep(thread_func_args->wait_to_release_ms*RES_MS)!= 0)            		/*wait for wait_to_obtain_ms 1000 us*/
        {
                perror("usleep");                       				/*redirecting the failure to stderr*/
                ERROR_LOG("usleep");     						/*redirecting the failure to stdout*/
                return thread_param;
        }
	if(pthread_mutex_unlock(thread_func_args->lock)!= 0)                   		/*unlock the mutex*/
        {
                perror("mutex unlock failure");                                         /*redirecting the failure to stderr*/
                ERROR_LOG("mutex unlock failure");                                      /*redirecting the failure to stdout*/
                return thread_param;
        }
	thread_func_args->thread_complete_success = true;                                  /*thread completion success*/
    }

    return thread_param;
}


bool start_thread_obtaining_mutex(pthread_t *thread, pthread_mutex_t *mutex,int wait_to_obtain_ms, int wait_to_release_ms)
{

    struct thread_data * thread_args = (struct thread_data *)malloc(sizeof(struct thread_data));

    if(thread_args == NULL)
    {
        perror("malloc");                       /*redirecting the failure to stderr*/
        ERROR_LOG("Failure during malloc");     /*redirecting the failure to stdout*/
        return false;                           /*return false in case of failure*/
    }

    thread_args->wait_to_obtain_ms = wait_to_obtain_ms;                 /*initializing the wait to obtain ms variable */
    thread_args->wait_to_release_ms = wait_to_release_ms;               /*initializing the wait to release ms variable*/
    thread_args->thread = thread;                                       /*initializing the thread attribute varible*/
    thread_args->lock = mutex;                                          /*initializing the mutex lock variable*/

    /*create the thread*/
    if(pthread_create(thread,NULL,&threadfunc,thread_args) != 0)
    {
        perror("pthread create");                                       /*error to stderror in case creation of thread failed*/
        ERROR_LOG("pthread Creation failure");                          /*error to stdout in case creation of thread failed*/
        free(thread_args);
        return false;
    }
    return true;
}
