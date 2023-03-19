#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/socket.h>
#include <signal.h>
#include <netinet/in.h>
#include <netdb.h>
#include <time.h>
#include "threadpool.h"

int bytes;
int qesize;
pool* p;
que* q;

pthread_mutex_t qlock = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t empty = PTHREAD_COND_INITIALIZER;
pthread_cond_t full = PTHREAD_COND_INITIALIZER;
pthread_cond_t init = PTHREAD_COND_INITIALIZER;

/* read files from directory who get from client to socket */
void read_copy(char* dirname){
  DIR* dirp;
  struct dirent *direntp;
  struct stat statbuff;
  char buff[100] = " ";

  /* open directory */
  if((dirp = opendir(dirname)) == NULL){
    perror("Error failed to open directory\n");
    return;
  }

  /* get files of directory */
  while((direntp = readdir(dirp)) != NULL){

    if(strncmp(direntp->d_name,".",1) == 0 || strncmp(direntp->d_name,"..",2) == 0)
      continue;

    strncpy(buff,dirname,strlen(dirname));    
    strcat(buff,"/");
    strcat(buff,direntp->d_name);

    /* get status of file */
    if(stat(buff, &statbuff) == 1){
      perror("stat\n");
      return;
    }

    /* check type of file */
    if((statbuff.st_mode & S_IFMT) == S_IFREG){
      if(q->queue[q->quesize] == NULL)
        q->queue[q->quesize] = (char*)malloc(100*sizeof(char));
      strncpy(q->queue[q->quesize],buff,strlen(buff));
      q->quesize++;
      printf("%s\n",buff);
    }
    // call recursively directories
    else if((statbuff.st_mode & S_IFMT) == S_IFDIR){
      read_copy(direntp->d_name);
    }

    /* clean buffer */
    strncpy(buff,"",100);
    
  }

  closedir(dirp);

}

/* communication thread */
void* thread_communication(void* arg){

  int sock = *(int*)arg;

  char buffer[250] = " ";
  char buff[250] = " ";
  char temp[100] = ".";
  
  /* read directory from client and send it back */
  read(sock, buffer, 250);
  printf("%s\n", buffer);
  
  read_copy(strcat(temp,buffer));

  pthread_cond_signal(&empty);
  
  if(q->quesize == qesize)
    pthread_cond_wait(&full,&qlock);

  //printf("socket num %d\n",sock);
  
  pthread_exit(NULL);

}

/* worker thread */
void* thread_work(void* arg){
  
  char c;
  char buff1[100] = " ";
  char buff2[100] = " ";
  
  int sock = *(int*)arg;

  pthread_mutex_lock(&qlock);
  
  if(q->quesize == 0){
    pthread_cond_wait(&empty,&qlock);
    printf("queue size is %d\n",q->quesize);
  }

  pthread_cond_signal(&full);
  
  if(q->queue[0] == NULL)
    pthread_exit(NULL);

  write(sock,"filename",bytes);
  write(sock,q->queue[0]+4,bytes);

  FILE* fp;

  if((fp = fopen(q->queue[0],"r")) == NULL)
    pthread_exit(NULL);

    
  while(fgets(buff1,bytes,fp) != NULL) {
    write(sock,buff1,bytes);
    strncpy(buff1,"",100);
  }

  fclose(fp);

  /* when finish read from one file in queue pop first index
     and decrease size by one */
  for(int j = 0; j < q->quesize; j++){
    q->queue[j] = q->queue[j+1];
  }
  
  q->quesize--;

  pthread_mutex_unlock(&qlock);

  pthread_exit(NULL);
  
}

int main(int argc, char** argv){

  struct sockaddr_in myaddr,client;
  int c,lsock,csock,newsock,flag,flagofp = 0,opt,thrps;
  long port;
  socklen_t myaddrlen,clientlen;

  pthread_t tid[2];

  /* check num of arguments from command line */
  while((opt = getopt(argc,argv,"p:q:s:b:")) != -1){
    switch(opt){
      case 'p':
	port = atoi(optarg);
	break;
      case 'q':
	qesize = atoi(optarg);
        break;
      case 's':
	thrps = atoi(optarg);
        break;
      case 'b':
	bytes = atoi(optarg);
        break;
      default:
        return 1;
    }

  }

  /* init and allocate space for queue */ 
  q = (que*)malloc(sizeof(que));

  q->quesize = 0;
  q->queue = (char**)malloc(qesize*sizeof(char**));

  /* socket create */
  if((lsock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("Error failed to create socket\n");
    return 1;
  }

  /* options for socket */
  myaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  myaddr.sin_port = htons(port);
  myaddr.sin_family = AF_INET;

  flag = 1;

  /* reuse socket after termination */
  setsockopt(lsock,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));
  
  if(bind(lsock, (struct sockaddr *)&myaddr, sizeof(myaddr)))
    perror("Error don't bind socket\n");

  if(listen(lsock, 10) < 0)
    perror("Error failed to listen socket\n");

  /* initialize struct for threadpool */ 
  p = NULL;

  p = threadpool_create(p,thrps,thread_work,NULL);
  
  while(1){

    /* listen for connections */
    if((newsock = accept(lsock, NULL, NULL)) < 0)
       perror("Error failed to accept connection\n");

    pthread_create(&tid[0],NULL,thread_communication,&newsock);

    /* create num of threads as the size of threadpool */
    for(int i = 0; i < thrps; i++){
      if(i <= q->quesize){
	//if(flagofp == 0 || p->qsize < 4){
          pthread_create(&(p->threads[i]),NULL,thread_work,&newsock);
	  pthread_join(p->threads[i],NULL);
          //p = threadpool_queue(p,thread_work,&newsock);
	//}
        //else{
	//  (void) p->pqueuehead[i].func(p->pqueuehead[i].arg);
        //}
      }
	
    }

    /* if data in queue are more than num of threads there
       run threads for remaining data in queue */
    if(q->quesize > 0){
      for(int i = q->quesize; i > 0; i--){
        pthread_create(&tid[1],NULL,thread_work,&newsock);
        pthread_join(tid[1],NULL);
      }
    }

    /* wait for communication thread */
    pthread_join(tid[0],NULL);

    flagofp = 1;
    
  }
  
  return 0;
}
