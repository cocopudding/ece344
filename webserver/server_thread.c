#include "request.h"
#include "server_thread.h"
#include "common.h"
#include "pthread.h"
#include <stdbool.h>
#include <string.h>
#include <limits.h>

pthread_mutex_t l = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;

pthread_mutex_t lcache = PTHREAD_MUTEX_INITIALIZER;

int in=0;
int out=0;

struct server {
	int nr_threads;
	int max_requests;
	int max_cache_size;
	/* add any other parameters you need */
        int * buffer;
        pthread_t * workerThreads;
        /*cache implementation parameteres*/
        struct cache_hashtable* cache;
        struct LRU_queue* queue;
};

struct cache_hashtable {
    struct file_data* file;
    int key;
    struct cache_hashtable *link;
    int size;
    int empty;
    
};

struct LRU_queue{
    char *fileName;
    struct LRU_queue *link;
    int useageFlag;
};

//hash function found online from https://gist.github.com/tonious/1377667
int hash(char *str,int size)
    {
       unsigned long int hashval;
	int i = 0;

	//Convert our string to an integer 
	while( hashval < ULONG_MAX && i < strlen( str ) ) {
		hashval = hashval << 8;
		hashval += str[ i ];
		i++;
	}    
        return hashval% size;
   
    }

/* static functions */
static void
do_server_request(struct server *sv, int connfd);



void workerthread_receive(struct server *sv){
    while(1){
        
        pthread_mutex_lock(&l);
        
        while(in==out){
            pthread_cond_wait(&empty, &l);
        }//empty

        int connfd=sv->buffer[out];
       // do_server_request(sv,connfd);

        if(((in-out+sv->max_requests+1)%(sv->max_requests+1))==sv->max_requests){
            pthread_cond_signal(&full);
        }
        out=(out+1)%(sv->max_requests+1);
        
        pthread_mutex_unlock(&l);
        
        do_server_request(sv,connfd);
        
    }
}


/*void workerthread_function(struct server *sv){
    while(true){
        int connfd=workerthread_receive(sv);
         do_server_request(sv,connfd);
    }
} */

/* initialize file data */
static struct file_data *
file_data_init(void)
{
	struct file_data *data;

	data = Malloc(sizeof(struct file_data));
	data->file_name = NULL;
	data->file_buf = NULL;
	data->file_size = 0;
	return data;
}

/* free all file data */
static void
file_data_free(struct file_data *data)
{
	free(data->file_name);
	free(data->file_buf);
	free(data);
}

struct cache_hashtable * cache_lookup(struct server *sv,char *file_name){
    //struct file_data *tempfile=(struct file_data *)malloc(sizeof(struct file_data));
     int hashnum=hash(file_name,100000);
     if(sv->cache[hashnum].empty==1){
         if(sv->cache[hashnum].link==NULL){
             if(strcmp(sv->cache[hashnum].file->file_name,file_name)==0){
                 return &sv->cache[hashnum];
             }
             else{
                 return NULL;
             }
         }
         else{
             if(strcmp(sv->cache[hashnum].file->file_name,file_name)==0){
                 return &sv->cache[hashnum];
             }
             else{
                 struct cache_hashtable *temp=sv->cache[hashnum].link;
                  while(temp->link!=NULL){
                     if(strcmp(temp->file->file_name,file_name)==0)
                     {
                         return temp;
                     }
                     temp=temp->link;
                  }
                  if(strcmp(temp->file->file_name,file_name)==0)
                  {
                    return temp;
                  }
                  else{
                      return NULL;
                  }
             }  
         }
     }
     else{
         return NULL;
     }
 
 }

int cache_evict(struct server *sv,int amount_to_evict);

int cache_insert(struct server *sv,struct file_data *file){
    if(file->file_size>sv->max_cache_size){
        return -2;
    }
    else if((sv->cache->size)<(file->file_size)){
        fprintf(stderr,"EVICT START:\n");
        int check=0;
        check=cache_evict(sv,1);
        if(check==-1){
            return -1;
        }
        return 0;
        //evict
    }
    /*add file into hash table according to file name*/
    int hashnum=hash(file->file_name,100000);
    
    if(sv->cache[hashnum].empty==0){
        sv->cache[hashnum].file=file;
        sv->cache[hashnum].link=NULL;
        sv->cache[hashnum].empty=1;    
        sv->cache->size=(sv->cache->size)-(file->file_size);
    }
    else{
         struct cache_hashtable *newNode=(struct cache_hashtable *)malloc(sizeof(struct cache_hashtable));
         struct cache_hashtable *temp;
         newNode->file=file;
         newNode->link=NULL;
         newNode->empty=1;
         
         if(sv->cache[hashnum].link==NULL){
             if(strcmp(file->file_name,sv->cache[hashnum].file->file_name)==0){
               free(newNode);
           }
            sv->cache[hashnum].link=newNode;
            sv->cache->size=(sv->cache->size)-(file->file_size);
         }
        else{ 
             if(strcmp(file->file_name,sv->cache[hashnum].file->file_name)==0){
                 free(newNode);
             }
             else{
                 temp=sv->cache[hashnum].link;
           
                while(temp->link!=NULL){
                    if(strcmp(file->file_name,temp->file->file_name)==0)
                     {
                        break;
                    }
                     temp=temp->link;

                 }
                 
                  if(strcmp(file->file_name,temp->file->file_name)==0)
                  {
                    free(newNode);
                  }
                  else{
                        temp->link=newNode;
                        sv->cache->size=(sv->cache->size)-(file->file_size);
                  }
             }
            
        }                     
    }
    
    /*add file name into LRU list for LRU eviction*/
    struct LRU_queue* node=(struct LRU_queue *)malloc(sizeof(struct LRU_queue));
    node->fileName=file->file_name;
    node->link=NULL;
    node->useageFlag=0;
    
    if(sv->queue==NULL){
        sv->queue=node;
        sv->queue->link=NULL;
    }
    else{
        struct LRU_queue* temp=sv->queue;
        while(temp->link!=NULL){
            temp=temp->link;
        }
        if(temp->link==NULL){
            temp->link=node;
            node->link=NULL; 
        }
    } 
    
    return 1;
}

int delete_from_cache(struct server *sv,char *file_name){
     int hashnum=hash(file_name,100000);
     if(sv->cache[hashnum].link==NULL){
         if(strcmp(sv->cache[hashnum].file->file_name,file_name)==0){
            sv->cache[hashnum].empty=0;
             sv->cache[hashnum].file=NULL;
             fprintf(stderr,"DELETED\n");
            return 1;
         }
         else{
             return 0;
         }
     }
     else{
         struct cache_hashtable *temp=sv->cache[hashnum].link;
         if(temp->link==NULL){
             if(strcmp(sv->cache[hashnum].file->file_name,file_name)==0){
                 free(sv->cache[hashnum].file);
                 sv->cache[hashnum].file=sv->cache[hashnum].link->file;
                 sv->cache[hashnum].link->file=NULL;
                 free(temp);
                 fprintf(stderr,"DELETED\n");
                 return 1;
             }else if(strcmp(temp->file->file_name,file_name)==0){
                 //free(temp->file);
                 temp->file=NULL;
                 free(temp);
                 sv->cache[hashnum].link=NULL;
                 fprintf(stderr,"DELETED\n");
                 return 1;
             }
             else{
                 return 0;
             }
         }
         else{
             struct cache_hashtable *temp2=sv->cache[hashnum].link->link;
             if(strcmp(sv->cache[hashnum].file->file_name,file_name)==0){
                 free(sv->cache[hashnum].file);
                 //sv->cache[hashnum].file=NULL;
                 sv->cache[hashnum].file=sv->cache[hashnum].link->file;
                 sv->cache[hashnum].link->file=NULL;
                 sv->cache[hashnum].link=temp->link;
                 free(temp);
                 fprintf(stderr,"DELETED\n");
                 return 1;
             }
             else if(strcmp(temp->file->file_name,file_name)==0){
                 free(temp->file);
                 //temp->file=NULL;
                 sv->cache[hashnum].link=temp->link;
                 free(temp);         
                 fprintf(stderr,"DELETED\n");
                 return 1;
             }
             else{
                 while(strcmp(temp2->file->file_name,file_name)!=0||temp2!=NULL){
                     temp2=temp2->link;
                     temp=temp->link;
                 }
                 if(temp2==NULL){
                     if(strcmp(temp->file->file_name,file_name)==0){
                         free(temp->file);
                     }
                     else{
                         return 0;
                     }
                 }
                 free(temp2->file);
                 //temp2->file=NULL;
                 temp->link=temp2->link;
                 free(temp2);
                 fprintf(stderr,"DELETED\n");
                 return 1;
             }            
         }            
     } 
     return 0;
}

int cache_evict(struct server *sv,int amount_to_evict){
    int evicted_amount=0;
    
    if(sv==NULL){
        return -1;
    }
    else if(amount_to_evict<=0){
        return 0;
    }
    else{
        //while(evicted_amount!=amount_to_evict){
            if(sv->queue==NULL){
                fprintf(stderr,"empty queue!!\n");
                return -1;
            }
            else if(sv->queue->link==NULL){
                if(sv->queue->useageFlag==0){
                    delete_from_cache(sv,sv->queue->fileName);
                    free(sv->queue);
                    sv->queue=NULL;
                    evicted_amount++;
                     fprintf(stderr,"evict1\n");
                }
                else{
                    fprintf(stderr,"evict2\n");
                    return 0;
                }
            }
            else{
                struct LRU_queue* temp=sv->queue;
                struct LRU_queue* temp2=sv->queue;
                if(temp->useageFlag==0){
                    sv->queue=temp->link;
                    temp2=temp->link;
                    delete_from_cache(sv,temp->fileName);
                    free(temp);
                    fprintf(stderr,"evict3\n");
                    evicted_amount++;
                }
                else{
                    temp=temp->link;
                    while(temp->useageFlag>0){
                        temp=temp->link;
                        temp2=temp2->link; 
                        if(temp==NULL){
                            fprintf(stderr,"evict4\n");
                            return evicted_amount;
                        }
                    }
                    temp2->link=temp->link;
                    delete_from_cache(sv,temp->fileName);
                    free(temp);
                    fprintf(stderr,"evict5\n");
                    evicted_amount++;
                }
            }
       // }
        return evicted_amount;
    }
}

void update_LRU(struct server *sv,char* file_name,int add){
    
    struct LRU_queue* temp=sv->queue;
    struct LRU_queue* prev=sv->queue;
    
    if(temp==NULL){
        return;
    }
    if(temp->link==NULL){
        while(strcmp(temp->fileName,file_name)!=0||temp!=NULL){
            temp=temp->link;
        }
        
          if(temp!=NULL){
            if(add==1){
               temp->useageFlag++;
               fprintf(stderr,"update1\n");
            }
            else{
               temp->useageFlag--;
               fprintf(stderr,"update2\n");
            }
        }
    }
    if(temp->link!=NULL){
        temp=temp->link;
        while(strcmp(temp->fileName,file_name)!=0||temp!=NULL){
            temp=temp->link;
            prev=prev->link;
        }
        
        if(temp!=NULL&&temp->link!=NULL){
             if(add==1){
               temp->useageFlag++;
            }
            else{
               temp->useageFlag--;
               prev->link=temp->link;
               while(prev->link!=NULL){
                   prev->link=temp;
               }
            }
        }
        else if(temp->link==NULL){
             if(add==1){
               temp->useageFlag++;
            }
            else{
               temp->useageFlag--;
            }
        }
    }
    
  

}

static void
do_server_request(struct server *sv, int connfd)
{
        
       if(sv->max_cache_size==0){
           int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();
            /*fills data->file_name with name of the file being requested */
            rq = request_init(connfd, data);
            if (!rq) {
		file_data_free(data);
		return;
            }
              // printf("~~~~~~~FILE NAME: %s\n",data->file_name);
            /* reads file, 
            * fills data->file_buf with the file contents,
             * data->file_size with file size. */
            ret = request_readfile(rq);
                if (!ret)
		goto out;
                /* sends file to client */
             request_sendfile(rq);
             
         out:
	request_destroy(rq);
	file_data_free(data);
            
      }
       else{
           int ret;
	struct request *rq;
	struct file_data *data;

	data = file_data_init();
 
	rq = request_init(connfd, data);
	if (!rq) {
		file_data_free(data);
		return;
	}
        
        // printf("~~~~~~~FILE NAME: %s\n",data->file_name);
            pthread_mutex_lock(&lcache);
            struct cache_hashtable *lookfor=cache_lookup(sv,data->file_name);
            pthread_mutex_unlock(&lcache);
            
            if (lookfor==NULL){
                
                //printf("add\n");
                ret = request_readfile(rq);
                if (!ret)
		goto out2;              
                request_sendfile(rq);                
                pthread_mutex_lock(&lcache);
                fprintf(stderr,"add to cache\n");
               //int check=cache_insert(sv,data);
                cache_insert(sv,data);
               
                pthread_mutex_unlock(&lcache);
                
             /*  while(check==0){
                pthread_mutex_lock(&lcache);
                check=cache_insert(sv,data);
                pthread_mutex_unlock(&lcache);
                }*/
                    
                
                
                /*if(check==-1||check==-2){
                pthread_mutex_lock(&lcache);
                request_set_data(rq,data); 
                pthread_mutex_unlock(&lcache);
                request_sendfile(rq);
                }*/
                
               /*if(check==-1||check==0){
                fprintf(stderr,"add to cache failed\n");
                pthread_mutex_lock(&lcache);
                request_set_data(rq,data); 
                pthread_mutex_unlock(&lcache);
                
                if(check==0){
                  pthread_mutex_lock(&lcache);
                update_LRU(sv,data->file_name,1);
                pthread_mutex_unlock(&lcache);
                }
                
                request_sendfile(rq);
                
                if(check==0){
                pthread_mutex_lock(&lcache);
                update_LRU(sv,data->file_name,0);
                pthread_mutex_unlock(&lcache);
                }
                }*/
                /*if(check==-1||check==0){
                    struct cache_hashtable *lookforagain=cache_lookup(sv,data->file_name);
                 if(lookforagain==NULL){
                     cache_insert(sv,data);
                 }
                 else{
                     file_data_free(data);
                 }
                }*/
                
               
            }
            else{
                 //printf("found\n");
               pthread_mutex_lock(&lcache);
                request_set_data(rq,lookfor->file); 
               pthread_mutex_unlock(&lcache);
                
                pthread_mutex_lock(&lcache);
                update_LRU(sv,data->file_name,1);
                pthread_mutex_unlock(&lcache);
                
                request_sendfile(rq);
                
                pthread_mutex_lock(&lcache);
                update_LRU(sv,data->file_name,0);
                pthread_mutex_unlock(&lcache);
            }
             out2:
	request_destroy(rq);
	//file_data_free(data); 
        }
        return;
	

}

/* entry point functions */

struct server *
server_init(int nr_threads, int max_requests, int max_cache_size)
{
	struct server *sv;

	sv = Malloc(sizeof(struct server));
	sv->nr_threads = nr_threads;
	sv->max_requests = max_requests;
	sv->max_cache_size = max_cache_size;

	if (nr_threads > 0 || max_requests > 0 || max_cache_size > 0) {
            if(max_requests>0){
                sv->buffer=Malloc(sizeof(int)*(max_requests+1));
            }
            if(nr_threads>0){
                sv->workerThreads=Malloc(sizeof(pthread_t)*(nr_threads));
                int i=0;
                for(i=0;i<nr_threads;i++){
                    pthread_create(&(sv->workerThreads[i]), NULL, (void *)&workerthread_receive, (void *)sv);
                }
                
            }
            if(max_cache_size>0){
                sv->cache=(struct cache_hashtable *)malloc(sizeof(struct cache_hashtable)*100000);
                assert(sv->cache);

                sv->cache->size=max_cache_size;
                
                int i=0;
                while(i<100000){
                    sv->cache[i].empty=0;
                    sv->cache[i].file=NULL;
                    sv->cache[i].key=0;
                    sv->cache[i].link=NULL;
                    i++;
                }
                struct LRU_queue *temp;
                while(sv->queue!=NULL){
                temp=sv->queue;
                sv->queue=sv->queue->link;
                free(temp);
                }
                sv->queue=NULL;
            }
	}
        
        

        
        
	/* Lab 4: create queue of max_request size when max_requests > 0 */

	/* Lab 5: init server cache and limit its size to max_cache_size */

	/* Lab 4: create worker threads when nr_threads > 0 */

	return sv;
}



void
server_request(struct server *sv, int connfd)
{
	if (sv->nr_threads == 0) { /* no worker threads */
		do_server_request(sv, connfd);
	} else {
		/*  Save the relevant info in a buffer and have one of the
		 *  worker threads do the work. */
                pthread_mutex_lock(&l);
                while((in-out+sv->max_requests+1)%(sv->max_requests+1)==sv->max_requests){
                    pthread_cond_wait(&full, &l);
                }//full

                sv->buffer[in]=connfd;
                if(in==out){
                    pthread_cond_signal(&empty);
                }

                in=(in+1)%(sv->max_requests+1);
                pthread_mutex_unlock(&l);
	}
}


