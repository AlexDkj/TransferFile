#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <time.h>
#include "threadpool.h"

int main(int argc, char** argv){

  struct sockaddr_in serveraddr,myaddr;
  struct hostent* h;
  struct in_addr address;
  int sock,flag,fd,flagc = 0,opt;
  long port;
  char* tk = " ",*ip,*directory;
  char buff[2048] = " ";
  char outdir[100] = "../outputfiles/";
  FILE* fp;

  /* check num of arguments from command line */
  while((opt = getopt(argc,argv,"i:p:d:")) != -1){
    switch(opt){
      case 'i':
        ip = optarg;
	break;
      case 'p':
        port = atoi(optarg);
        break;
      case 'd':
	directory = optarg;
        break;
      default:
        return 1;
    }

  }

  /* create outputf file for results */
  if(mkdir(outdir,0755) < 0){
    if(errno != EEXIST){
      perror("Error failed to create outfile\n");
      return 1;
    }
  }

  /* socket create */ 
  if((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0){
    perror("Error failed to create socket\n");
    return 1;
  }

  // get host address
  inet_aton(ip, &address);
  
  if((h = gethostbyaddr((const char*)&address, sizeof(address), AF_INET)) == NULL){
    herror("Error failed gethostbyaddr");
    return 1;
  }

  /* options for socket */
  memcpy(&serveraddr.sin_addr, h->h_addr, h->h_length);
  serveraddr.sin_port = htons(port);
  serveraddr.sin_family = AF_INET;

  /* connect to socket for communication with server */
  if(connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) != 0){
    perror("Error failed to connect\n");
    return 1;
  }

  flag = 1;

  /* reuse socket after termination */ 
  setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&flag,sizeof(flag));

  /* write name of directory to socket for the server side */ 
  write(sock, directory, 100);

  /* read files and their data and create copy of them to output file */
  while(read(sock, buff, 100) > 0){
    /* filename are flag to see when finish data of each file */
    if(strncmp(buff,"filename",8) == 0){

      flagc = 1;

      /* close previous file descriptor of files */
      if(flagc == 1){
	close(fd);
      }

      /* read file who will copied */
      read(sock, buff, 100);
      
      /* create files in outdir */
      if((fd = open(strcat(outdir,buff),O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0){
        perror("Error failed to create outfile\n");
      }

      /* clear outfile buffer from previous data */
      strncpy(outdir+15,"",strlen(buff));
    }
    /* write contents of files who read from socket */
    write(fd,buff,100);
    //printf("%s\n",buff);
  }
  
  //printf("%s\n",h->h_name);

  return 0;

}
