typedef struct work_thread{
  void*(*func)(void*);
  void* arg;
  struct work_thread* next;
}workt;

typedef struct threadpool{
  int qsize;
  int thsize;
  workt* pqueuehead;
  pthread_t* threads;
  workt* pqueuetail;
}pool;

typedef struct queuex{
  int quesize;
  char** queue;
}que;

pool* threadpool_create(pool* ,int ,void*(*func)(void*) ,void* );
pool* threadpool_queue(pool* ,void*(*func)(void*) ,void* );
