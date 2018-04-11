#include <assert.h>
#include <stdlib.h>
#include <ucontext.h>
#include "thread.h"
#include "interrupt.h"
#include <stdbool.h>

/* This is the thread control block */
struct thread {
	/* ... Fill this in ... */
    ucontext_t context;
    Tid threadID;
    //int ready;
    struct thread* link;
    int exit;
    
};

struct thread* exitQueue;
struct thread* readyQueue;
struct thread* runningThread;
void* stackArray[THREAD_MAX_THREADS];


/* thread starts by calling thread_stub. The arguments to thread_stub are the
 * thread_main() function, and one argument to the thread_main() function. */
void
thread_stub(void (*thread_main)(void *), void *arg)
{
        int enabled =  interrupts_set(1);
	Tid ret;

	thread_main(arg); // call thread_main() function with arg
        interrupts_set(enabled);
        
	ret = thread_exit(THREAD_SELF);
	// we should only get here if we are the last thread. 
	assert(ret == THREAD_NONE);
   
        
	// all threads are done, so process should exit
	exit(0);
}

void exitQueue_clear(){
    if(exitQueue==NULL){
        return;
    }else{
            struct thread *temp;
        while(exitQueue!=NULL){
            temp=exitQueue;
            exitQueue=exitQueue->link;
            free(stackArray[temp->threadID]);
            stackArray[temp->threadID]=NULL;
            free(temp);
        }
    }

}

void linklist_print(){
     
     if(readyQueue==NULL){
         printf("list is completely empty\n");
     }
     else{
         struct thread* temp=readyQueue;
         printf("list contains:\n");
         printf("%d\n",(int)temp->threadID);
         while(temp->link!=NULL){
             temp=temp->link;
             printf("%d\n",(int)temp->threadID);
         }
     }
}

//linked list structure for ready queue
void linklist_add(struct thread* node){
    
    if(readyQueue==NULL){
        readyQueue=node;
        readyQueue->link=NULL;
    }
    else{
        struct thread* temp=readyQueue;
        while(temp->link!=NULL){
            temp=temp->link;
        }
        if(temp->link==NULL){
            temp->link=node;
            node->link=NULL; 
        }
    }
    //printf("added %d\n", node->threadID);
}

//if no more return 0, else return 1
int linklist_removefirst(){
    if(readyQueue==NULL){
            return 0;
    }
    struct thread* temp=readyQueue;
    readyQueue=temp->link;
    free(temp);
    return 1;
}

//successful return 1, unsuccessful return 0 
/*
int linklist_removegiven(Tid given){
        if(readyQueue==NULL){
            return 0;
        }
        else if(readyQueue->link==NULL){
            if(readyQueue->threadID==given){
                free (readyQueue);
                readyQueue=0;
                return 1;
            }
            else return 0;
        }
        else if(readyQueue->threadID==given){
               linklist_removefirst();
               return 1;
        }
        else{
            bool found=false;
            struct thread *prev=readyQueue;            
            struct thread *current=readyQueue->link;
            
            while(current->link!=NULL||found==false){
                if(current->threadID==given){
                    found=true;
                }else{
                    current=current->link;
                    prev=prev->link;
                }
            }
            
            if(found==true){
                prev->link=current->link;
                runningThread=current;
                runningThread->link=NULL;
                return 1;
            }
            else{
                return 0;
            }

        }
}*/

//check if id is within list, true=contains, false=does not contain
bool linklist_checkid(Tid id){
    struct thread* temp=readyQueue;
    if(readyQueue==NULL){
        return false;
    }
    while(temp!=NULL){
        if(temp->threadID==id){
            return true;
        }
        else {
            temp=temp->link;
        }
    }
    return false;
}


struct thread* linklist_findthread(Tid id){
    struct thread* temp=readyQueue;
    if(readyQueue==NULL){
        return NULL;
    }
    while(temp!=NULL){
        if(temp->threadID==id){
            return temp;
        }
        temp=temp->link;
    }
    return NULL;   
}




void
thread_init(void)
{
    //int enabled =  interrupts_set(0);
	/* your optional code here */
    exitQueue=NULL;
    readyQueue=NULL;
    runningThread=malloc(sizeof(struct thread));
    //runningThread->ready=0;
    runningThread->threadID=0;
    runningThread->link=NULL;
    getcontext(&(runningThread->context));
    //stackArray[0]=malloc(THREAD_MIN_STACK);
   //interrupts_set(enabled);
}

Tid
thread_id()
{
	return runningThread->threadID;
}

Tid
thread_create(void (*fn) (void *), void *parg)
{
    int enabled =  interrupts_set(0);
    
    Tid id=1;

    
    for(id=1;id<=THREAD_MAX_THREADS;id++){
        if(stackArray[id]==NULL){
            break;
        }
    }
    
    /*if(id<200){
        printf("id is:%d\n",id);
    }*/
    
        
    if(id==THREAD_MAX_THREADS){
       interrupts_set(enabled);
        return THREAD_NOMORE;
    }
    
    void* newStack=malloc(THREAD_MIN_STACK+8);
    if(newStack==NULL){
        interrupts_set(enabled);
        return THREAD_NOMEMORY;
    }
    
     stackArray[id]=newStack;
     
    struct thread* newThread=(struct thread*)malloc(sizeof(struct thread));
    newThread->threadID=id;
    newThread->link=NULL;
    newThread->exit=0;
    //newThread->ready=0;
    
   
    
        //runningThread=newThread;

    

    getcontext(&(newThread->context));
    
    
  
    newThread->context.uc_mcontext.gregs[REG_RSP] = (unsigned long)newStack+THREAD_MIN_STACK+8;
    newThread->context.uc_mcontext.gregs[REG_RIP] = (unsigned long)(thread_stub);
    newThread->context.uc_mcontext.gregs[REG_RDI] = (unsigned long)fn;
    newThread->context.uc_mcontext.gregs[REG_RSI] = (unsigned long)parg;
  
    linklist_add(newThread);
    
   // printf("FOR ADD:\n");
   // linklist_print();
    

    interrupts_set(enabled);
    return id;
}

Tid
thread_yield(Tid want_tid)
{
    int enabled =  interrupts_set(0);
    
    exitQueue_clear();
    
    if(runningThread->exit==1){
        
        //add running thread into exit queue
        if(exitQueue==NULL){
            exitQueue=runningThread;
            exitQueue->link=NULL;
        }
        else{
            struct thread* temp=exitQueue;
            while(temp->link!=NULL){
                temp=temp->link;
            }
            if(temp->link==NULL){
                temp->link=runningThread;
                runningThread->link=NULL; 
            }
        }
        
        if(readyQueue==NULL){
            interrupts_set(enabled);
            return THREAD_NONE;
        }
        //put the next thread into running thread
           int check=0;
           getcontext(&(runningThread->context));
           
           if(check==0){
               runningThread=readyQueue;
               readyQueue=readyQueue->link;
               runningThread->link=NULL;
               check=1;
               setcontext(&(runningThread->context));
           }

    }
    
    if(want_tid==THREAD_SELF||want_tid==runningThread->threadID){
        interrupts_set(enabled);
        return runningThread->threadID;
    }
    else if(want_tid==THREAD_ANY){
        if(readyQueue==NULL){ 
            interrupts_set(enabled);
            return THREAD_NONE;
        }
        else{
            int check=0;
            getcontext(&(runningThread->context));
            
            if(check==1){
                interrupts_set(enabled);
                return runningThread->threadID;
            }
            if(check==0){
                linklist_add(runningThread);
                runningThread=readyQueue;
                readyQueue=readyQueue->link;
                runningThread->link=NULL;
                check=1;
                setcontext(&(runningThread->context));      
            }
        }      
    }else{
        if(readyQueue==NULL||want_tid>THREAD_MAX_THREADS||want_tid<-2||linklist_checkid(want_tid)==false){
            interrupts_set(enabled);
            return THREAD_INVALID;
        }
        else{                                
                int check=0;
                getcontext(&(runningThread->context));
                
                if(check==1){
                    interrupts_set(enabled);
                    return want_tid;
                }
                if(readyQueue->threadID==want_tid){
                    linklist_add(runningThread);
                    runningThread=readyQueue;
                    readyQueue=readyQueue->link;
                    runningThread->link=NULL;   
                    check=1;
                    setcontext(&(runningThread->context)); 
                }
                else{
                    struct thread* temp=readyQueue;
                    struct thread* past = NULL;
                    while(temp!=NULL){
                        if(temp->threadID==want_tid)
                            break;
                        else{
                            past=temp;
                            temp=temp->link;
                        }
                    }
                    linklist_add(runningThread);
                    past->link=temp->link;
                    runningThread = temp;
                    runningThread->link=NULL;
                    check=1;
                    setcontext(&(runningThread->context)); 
                }

        }
    
    }
    interrupts_set(enabled);
    return THREAD_INVALID;
}

Tid
thread_exit(Tid tid)
{
    int enabled =  interrupts_set(0);	
   
    if(tid==THREAD_SELF||tid==runningThread->threadID){
        //empty state
        if(readyQueue==NULL){
           // interrupts_on();
            interrupts_set(enabled);
            return THREAD_NONE;
        }
        
        runningThread->exit=1;
        
        //add running thread into exit queue
        if(exitQueue==NULL){
            exitQueue=runningThread;
            exitQueue->link=NULL;
        }
        else{
            struct thread* temp=exitQueue;
            while(temp->link!=NULL){
                temp=temp->link;
            }
            if(temp->link==NULL){
                temp->link=runningThread;
                runningThread->link=NULL; 
            }
        }
        
        //put the next thread into running thread
        int check=0;
        getcontext(&(runningThread->context));

        if(check==0){
            runningThread=readyQueue;
            readyQueue=readyQueue->link;
            runningThread->link=NULL;
            check=1;
            setcontext(&(runningThread->context));
        }
        interrupts_set(enabled);
         return exitQueue->threadID;

        
    }
    else if(tid==THREAD_ANY){
        if(readyQueue==NULL){
            interrupts_set(enabled);
            return THREAD_NONE;
        }
        
        readyQueue->exit=1;
        interrupts_set(enabled);
        return readyQueue->threadID;
    }
    else{
        if(tid<-2||tid>=THREAD_MAX_THREADS||linklist_checkid(tid)==false){
            interrupts_set(enabled);
            return THREAD_INVALID;
        }
        else{
            struct thread* temp=linklist_findthread(tid);
            temp->exit=1;
           interrupts_set(enabled);
            return tid;
        }
        
    }
   // interrupts_off();
        
    //invalid thread
    /*if(tid<-2||tid>=THREAD_MAX_THREADS){
      //  interrupts_on();
        return THREAD_INVALID;
    }
    
    
    if((tid==THREAD_ANY&&readyQueue==NULL)||(tid==THREAD_SELF&&readyQueue==NULL)){
       // interrupts_on();
        return THREAD_NONE;
    }
    
    
    if(tid==THREAD_ANY){
        Tid index=readyQueue->threadID;
        linklist_removefirst();
        free(stackArray[index]);  
        //interrupts_on();
	return index;
    }
    else if(tid==THREAD_SELF||runningThread->threadID==tid){
        Tid index=runningThread->threadID;
        
        runningThread->context=readyQueue->context;
        runningThread->threadID=readyQueue->threadID;
       // runningThread->ready=readyQueue->ready;
        runningThread->link=NULL;
        
        linklist_removefirst();
        
        free(stackArray[index]);  
      //  interrupts_on();
	return index;

    }
    else{
        if(linklist_checkid(tid)==false){
            return THREAD_INVALID;
        }
        else{
             if(readyQueue->threadID==tid){
                 struct thread* temp=readyQueue;
                    readyQueue=readyQueue->link;
                    temp->link=NULL;   
                    free(temp);
                    free(stackArray[tid]);
             }
             else{
                 struct thread* temp2=readyQueue;
                 struct thread* temp=readyQueue;
                    struct thread* past = NULL;
                    while(temp!=NULL){
                        if(temp->threadID==tid)
                            break;
                        else{
                            past=temp;
                            temp=temp->link;
                        }
                    }
                    past->link=temp->link;
                    temp2 = temp;
                    temp2->link=NULL;
                    free(temp2);
                    free(stackArray[tid]);
             }
        }
    }


    
         
        // interrupts_on();
	return THREAD_FAILED;*/
    
}

/*******************************************************************
 * Important: The rest of the code should be implemented in Lab 3. *
 *******************************************************************/

/* This is the wait queue structure */
struct wait_queue {
	/* ... Fill this in ... */
    struct thread* wait_queue_head;
    int threadNum;
    
};

struct wait_queue *
wait_queue_create()
{
	struct wait_queue *wq;

	wq = malloc(sizeof(struct wait_queue));
	assert(wq);

	wq->wait_queue_head=NULL;
        wq->threadNum=0;

	return wq;
}

void
wait_queue_destroy(struct wait_queue *wq)
{
	 if(wq->wait_queue_head==NULL){
           wq->threadNum=0;  
           free(wq);
        }else{
            struct thread *temp;
        while(wq->wait_queue_head!=NULL){
            temp=wq->wait_queue_head;
            wq->wait_queue_head=wq->wait_queue_head->link;
            //free(stackArray[temp->threadID]);
           // stackArray[temp->threadID]=NULL;
           // free(temp);
            thread_exit(temp->threadID);
        }
            free(wq);
    }
	//free(wq);
}

Tid
thread_sleep(struct wait_queue *queue)
{
     int enabled =  interrupts_set(0);	
     
    if (queue==NULL){
        interrupts_set(enabled);
        return THREAD_INVALID;
    } 
     else if (readyQueue==NULL){
        interrupts_set(enabled);
        return THREAD_NONE;
    }

    
    Tid takeover_id=readyQueue->threadID;
    int check=0;
    getcontext(&(runningThread->context));
                
    if(check==1){
       interrupts_set(enabled);
       return takeover_id;
    }
    
     if(queue->wait_queue_head==NULL){
        queue->wait_queue_head=runningThread;
        queue->wait_queue_head->link=NULL;
        queue->threadNum++; 
    }
    else{
        struct thread* temp=queue->wait_queue_head;
        while(temp->link!=NULL){
            temp=temp->link;
        }
        if(temp->link==NULL){
            temp->link=runningThread;
            temp->link->link=NULL; 
        }
        queue->threadNum++; 
    }
    
    runningThread=readyQueue;
    readyQueue=readyQueue->link;
    runningThread->link=NULL;   
    check=1;
    setcontext(&(runningThread->context)); 

    interrupts_set(enabled);       
    return THREAD_FAILED;
}

/* when the 'all' parameter is 1, wakeup all threads waiting in the queue.
 * returns whether a thread was woken up on not. */
int
thread_wakeup(struct wait_queue *queue, int all)
{
    int enabled =  interrupts_set(0);	
    
    if(queue==NULL){
        interrupts_set(enabled);
        return 0;
    }
    
    if(all==0){
        if(queue->wait_queue_head==NULL){
            interrupts_set(enabled);
            return 0;
        }
        else{
            struct thread* temp=queue->wait_queue_head;
            queue->wait_queue_head=queue->wait_queue_head->link;
            temp->link=NULL;
            linklist_add(temp);
            queue->threadNum--;
            interrupts_set(enabled);
            return 1;
        }
    }
    else{
        if(queue->wait_queue_head==NULL){
            interrupts_set(enabled);
            return 0;
        }
        else{
            int originalNum=queue->threadNum;
            struct thread* temp=queue->wait_queue_head;
            queue->wait_queue_head=NULL;
            //linklist_add(temp);
            
            
        //add all of wait_queue into readyQueue    
        if(readyQueue==NULL){
            readyQueue=temp;
        //readyQueue->link=NULL;
        }
        else{
            struct thread* temp2=readyQueue;
            while(temp2->link!=NULL){
                temp2=temp2->link;
            } 
            temp2->link=temp;

        }
               
            queue->threadNum=0;
            interrupts_set(enabled);
            return originalNum;
        }
    }
        
	//return 0;
}

struct lock {
	/* ... Fill this in ... */
    struct wait_queue* wq;
    int available;
    //Tid id;
};

struct lock *
lock_create()
{
    int enabled =  interrupts_set(0);	
	struct lock *lock;

	lock = malloc(sizeof(struct lock));
	assert(lock);
        
        lock->wq =wait_queue_create();
	
        lock->available =1;
//        lock->id=0;
        
         interrupts_set(enabled);
	return lock;
}

void
lock_destroy(struct lock *lock)
{
     //int enabled =  interrupts_set(0);	
	assert(lock != NULL);
        
        if(lock->available==1){    
            wait_queue_destroy(lock->wq);
            lock->available=0;
      //      lock->id=0;
            free(lock);
        }
      //   interrupts_set(enabled);
}

void
lock_acquire(struct lock *lock)
{
    int enabled =  interrupts_set(0);	
	assert(lock != NULL);

 
            while(lock->available==0){
               thread_sleep(lock->wq);
            //   printf("waiting to aquire\n");
           }
        
            lock->available=0;
            //lock->id=runningThread->threadID;


    interrupts_set(enabled);
}

void
lock_release(struct lock *lock)
{
        int enabled =  interrupts_set(0);	
	assert(lock != NULL);
        

            //while(lock->available==1)
            //thread_wakeup(lock->wq,1);

        
            
            thread_wakeup(lock->wq,1);
            lock->available=1;   
            
         //   printf("released lock: id is %d\n",lock->id);
   
       // else if(lock->available==1){
          //  thread_wakeup(lock->wq,1);
        //}
        

       // else
            //lock->available=0;
        

        interrupts_set(enabled);
	//TBD();
}

struct cv {
	/* ... Fill this in ... */
    struct wait_queue* wq;
};

struct cv *
cv_create()
{
	struct cv *cv;

	cv = malloc(sizeof(struct cv));
	assert(cv);

	cv->wq =wait_queue_create();

	return cv;
}

void
cv_destroy(struct cv *cv)
{
	assert(cv != NULL);

	int enabled =  interrupts_set(0);
        
        if(cv->wq->wait_queue_head!=NULL){    
            wait_queue_destroy(cv->wq);
            free(cv);
        }
        
        interrupts_set(enabled);

	//free(cv);
}

void
cv_wait(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);
        
        int enabled =  interrupts_set(0);
        
        lock_release(lock);
         thread_sleep(cv->wq);
         lock_acquire(lock);
         
         interrupts_set(enabled);

}

void
cv_signal(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);
        
        int enabled =  interrupts_set(0);
        
        if(lock->available==0){
            thread_wakeup(cv->wq,0);
        }
        
        interrupts_set(enabled);


}

void
cv_broadcast(struct cv *cv, struct lock *lock)
{
	assert(cv != NULL);
	assert(lock != NULL);
        
        int enabled =  interrupts_set(0);
        
        if(lock->available==0){
            thread_wakeup(cv->wq,1);
        }
        
        interrupts_set(enabled);

}
