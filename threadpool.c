#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include "threadpool.h"

pool* threadpool_create(pool* threadpool,int tpsize,void*(*func)(void*),void* arg){

  threadpool = (pool*)malloc(sizeof(pool));
    
  threadpool->pqueuehead = NULL;
  threadpool->pqueuehead = (workt*)malloc(tpsize*sizeof(workt));
  threadpool->threads = (pthread_t*)malloc(tpsize*sizeof(pthread_t));  
  threadpool->qsize = 0;
  threadpool->thsize = tpsize;
  
  return threadpool; 
}

pool* threadpool_queue(pool* threadpool,void*(*func)(void*),void* arg){

  int indexq = threadpool->qsize++;

  
  threadpool->pqueuehead[indexq].func = func;
  threadpool->pqueuehead[indexq].arg = arg;
  //threadpool->threads[indext] = pid;
  

  return threadpool;
}
