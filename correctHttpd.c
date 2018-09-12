#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#define IN_PORT             0x1
#define IN_HELP             0x2
typedef char bool;
#define true                1
#define false               0
#define MAX_CONNECT_NUMBER  50
#define READ_BLOCK_SIZE     1000
char *orders[4]={"-p","--port","-h","--help"};
char helpInfo[]="This is a http server started from terminal.\n\
httpd [OPTION] .. DIR\n\
-p, --port to bind the port.\n\
-h, --help to show help info. Enter the file you want to use.";
char *mapSuffix[]={"jpg","jpeg","png","ico","gif","html","css","js","json","pdf","pptx"};
char *mapTypes[]={"image/jpg","image/jpeg","image/png","image/ico","image/gif","text/html","text/css","application/js","application/json","application/pdf","application/x-ppt"};
pthread_cond_t processed;
pthread_mutex_t gzipLock;
typedef struct requestGroup{
	bool gzipOn;
	int fd;
	int requestTime;
}RequestGroup;
char *typeJudge(char *fileName,bool gzipOn)
{
	printf("Judge file type for %s\n",fileName);
	char *pointer=fileName+strlen(fileName);
	if(!gzipOn){
		while(*pointer--!='.') ;
		pointer+=2;
	}
	else{
		while(*pointer--!='.') ;
		while(*pointer--!='.') ;
		pointer+=2;
		char *temp=(char *)malloc(sizeof(char )*12);
		memset(temp,0,12);
		for(int i=0;i<11;i++){
			if(pointer[i]=='.')
				break;
			temp[i]=pointer[i];
		}
		pointer=temp;
	}
	printf("\033[31mType:%s\n\033[0m",pointer);
	for(int i=0;i<11;i++)
		if(strcmp(pointer,mapSuffix[i])==0)
			return mapTypes[i];
	return mapTypes[5];
}
int programState=0;
int port,socketFd;
char *sitePath=NULL;
pthread_cond_t waitDone[10];
void analyzeState(int argc,char *argv[])
{
	for(int i=1;i<argc;i++){
		bool isOrder=false;
		for(int j=0;j<4;j++){
			if(strcmp(argv[i],orders[j])==0){
				programState|=1<<(j/2);
				isOrder=true;
			}
		}	
		if(!isOrder){
			if(argv[i][0]=='.')
				sitePath=argv[i];
			else if(isdigit(argv[i][0]))
				port=atoi(argv[i]);
			else{
				printf("Unable to analyze at #%d, %s!\n",i,argv[i]);
				programState=0;
				break;
			}
		}
	}
	if(programState==0||(programState&IN_HELP)){
		printf("%s\n",helpInfo);
		exit(EXIT_SUCCESS);
	}
	else{
		printf("You choose to visit %s from port %d.\n",sitePath,port);
		int cd=chdir(sitePath);
		if(cd<0){
			printf("For %s ",sitePath);
			perror("chdir");
			exit(EXIT_FAILURE);
		}
		int ad=access("index.html",O_RDONLY);
		assert(ad>=0);
	}
}
void sigint_handler(int signum)
{
	printf("\033[31mKeyboard interrupt detected, closing server...\n");
	int sc=shutdown(socketFd,SHUT_RD);
	if(sc<0){
		perror("shutdown server");
		exit(EXIT_FAILURE);
	}
	int cc=close(socketFd);
	if(cc<0){
		perror("close socket");
		exit(EXIT_FAILURE);
	}
	sleep(2);
	printf("Shutdown.\n\033[0m");
	exit(EXIT_SUCCESS);
}
static void requestDealer(void *requestInfo)
{
	RequestGroup *rg=(RequestGroup *)requestInfo;
	int fd=rg->fd;
	bool gzipOn=rg->gzipOn;
	printf("Fd in requestDealer:%d\n",fd);
	int length=1;
	//fcntl(fd,F_SETFL,O_NONBLOCK);
	char *buffer=(char *)malloc(sizeof(char )*READ_BLOCK_SIZE);
	memset(buffer,0,READ_BLOCK_SIZE);
	read(fd,buffer,READ_BLOCK_SIZE-1);
//	printf("Buffer:\n%s\n",buffer);
	if(strlen(buffer)==0){//invalid access
		printf("Request exited.\n");
		//pthread_cond_signal(&processed);
		close(fd);
		pthread_exit(NULL);
	}
	char *firstLine=strtok(buffer,"\r\n");
	printf("First line:%s\n",firstLine);
	assert(firstLine);
	char *getStr=strtok(firstLine," ");
	getStr=strtok(NULL," ");
	printf("Path:%s\n",getStr);
	getStr++;
	int ac=access(getStr,O_RDONLY);
	bool accessAble=true;
	if(ac<0){
		strcat(getStr,"index.html");
		ac=access(getStr,O_RDONLY);
		if(ac<0){
			perror("access");
			accessAble=false;
		}
	}
	int fileLength=0;
	int fileFd=-1;
	if(accessAble){
		printf("File:%s\n",getStr);
		fileFd=open(getStr,O_RDONLY);
		if(fileFd<0){
			perror("Open file");
			accessAble=false;
		}
		else{
			char *gzipPath=(char *)malloc(sizeof(char )*40);
			assert(gzipPath);
			memset(gzipPath,0,40);
			sprintf(gzipPath,"%s.gz",getStr);
			printf("%s\n",gzipPath);
			int ac=access(gzipPath,O_RDONLY);	
			if(ac<0)
				perror("access gzip file");
			else
				gzipOn=true;
			/*	
	
			int rc=fork();
			if(rc==0){
				printf("In child fork %d for gzip.\n",getpid());
				char *args[5];
				args[0]="gzip";
				args[1]="-k";
				args[2]="-f";
				args[3]=getStr;
				args[4]=NULL;
				int ec=execvp("gzip",args);
				if(ec<0){
					perror("Execvpe");
					gzipOn=false;
					exit(EXIT_FAILURE);
				}
			}
			else{
				int wc=wait(NULL);
				//close(fileFd);
			*/
				if(gzipOn){
					close(fileFd);
					strcat(getStr,".gz");
					fileFd=open(getStr,O_RDONLY);
					if(fileFd<0){
						perror("fileFd open");
						exit(EXIT_FAILURE);
					}
				}
				fileLength=lseek(fileFd,0,SEEK_END);
				if(fileLength<0){
					perror("lseek");
					assert(0);
				}
				lseek(fileFd,0,SEEK_SET);
			//}
		}
	}
	printf("Malloc length:%d\n",1000+fileLength);
	char *response=(char *)malloc(sizeof(char )*(1000+fileLength));
	assert(response);
	memset(response,0,1000+fileLength);
	char *start=response;
	int headerLength=0;
	if(accessAble){
		sprintf(response,"HTTP/1.1 200 OK\r\n");
		response+=strlen(response);
		sprintf(response,"Content-Type : %s\r\n",typeJudge(getStr,gzipOn));
		response+=strlen(response);
		if(gzipOn){
			sprintf(response,"Content-Encoding: gzip\r\n");
			response+=strlen(response);
		}
		sprintf(response,"Content-Length : %d\r\n\r\n",fileLength);
		response+=strlen(response);
		headerLength=strlen(start);
		assert(headerLength==response-start);
		for(int i=0;i<fileLength;i++){
			read(fileFd,&response[i],1);
		}
	}
	else{
		sprintf(response,"HTTP/1.1 404 NOT FOUND\r\n");
		response+=strlen(response);
		sprintf(response,"Content-Type : text/html\r\n");
		response+=strlen(response);
		sprintf(response,"Content-Length : 12\r\n\r\n");
		response+=strlen(response);
		sprintf(response,"No such file");
		response+=strlen(response);
		headerLength=85;//static header
	}
	//printf("Final response:{\n%s}\n",start);
	int wc=write(fd,start,headerLength+fileLength);
	if(wc<0)
		perror("Write");
	close(fd);
	close(fileFd);
	if(wc!=headerLength+fileLength){
		printf("Wrote %d bytes while true one is %d\n",headerLength+fileLength);
		assert(wc==headerLength+fileLength);
	}
	free(start);
	//close(fd);
	printf("\033[32mWROTE DONE for %d, fd %d\n\033[0m",rg->requestTime,fd);
	//pthread_cond_signal(&processed);
	pthread_exit(NULL);
}
void mainDealer()
{
	struct sockaddr_in sAddrIn;
	struct sockaddr_in sAddrClient;
	socketFd=socket(AF_INET,SOCK_STREAM,0);
	if(socketFd<0){
		perror("socket");
		exit(EXIT_FAILURE);
	}
	const int timer=5;
	int sc=setsockopt(socketFd,SOL_SOCKET,SO_REUSEADDR,(const void *)&timer,sizeof(int));
	if(sc<0){
		perror("Setsocketopt");
		exit(EXIT_FAILURE);
	}
	pthread_cond_init(&processed,NULL);
	pthread_mutex_init(&mutex,NULL);
	memset(&sAddrIn,0,sizeof(sAddrIn));
	sAddrIn.sin_family=AF_INET;
	sAddrIn.sin_addr.s_addr=htonl(INADDR_ANY);
	sAddrIn.sin_port=htons(port);
	int fd=bind(socketFd,(struct sockaddr *)&sAddrIn,sizeof(sAddrIn));
	if(fd<0){
		perror("bind");
		exit(EXIT_FAILURE);
	}
	fd=listen(socketFd,MAX_CONNECT_NUMBER);
	if(fd<0){
		perror("listen");
		exit(EXIT_FAILURE);
	}
	int recvTimes=0;
	int recvFd=-1;
	int clientLength=sizeof(sAddrClient);
	while((recvFd=accept(socketFd,&sAddrClient,&clientLength))!=-1){
		printf("\033[33mNew connection on %d...\n\033[0m",recvFd);
		pthread_t dealer;
		RequestGroup *rg=(RequestGroup *)malloc(sizeof(RequestGroup));
		assert(rg);
		rg->gzipOn=false;
		rg->fd=recvFd;
		rg->requestTime=++recvTimes;
		int pc=pthread_create(&dealer,NULL,(void *)requestDealer,(void *)rg);
		if(pc<0){
			perror("Thread create");
			exit(EXIT_FAILURE);
		}
		//pthread_cond_wait(&processed,&mutex);
		printf("Recvtimes:%d\n",recvTimes);
	}
}
int main(int argc, char *argv[])
{
	signal(SIGINT,sigint_handler);
	analyzeState(argc,argv);
	mainDealer();
	return 0;
}
