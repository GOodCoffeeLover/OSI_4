#include<unistd.h>
#include<fcntl.h>
#include<stdlib.h>
#include<sys/wait.h>
#include<sys/mman.h>
#include<semaphore.h>
#include<stdio.h>
#include<errno.h>

int main(){
	sem_unlink("readmap");
		
	const size_t SIZE_OF_MAP = 101;
	const off_t ZERO_OFFSET = 0;
	const int FALSE_FD = -1;
	char* mmaps = mmap( NULL, SIZE_OF_MAP ,
							  PROT_READ | PROT_WRITE,
							  MAP_SHARED | MAP_ANON,
							  FALSE_FD, ZERO_OFFSET);
	if(mmaps==MAP_FAILED){
		printf("MMAP ERROR: %d\n", errno);
		return -1;
	}
	sem_t* sem_par=sem_open("writemap", O_CREAT, S_IRGRP | S_IWGRP, 1);
	if(sem_par==SEM_FAILED){
		printf("SEM ERROR: %d\n", errno);
		return -2;
	}
	sem_t* sem_chld=sem_open("readmap", O_CREAT, S_IRGRP | S_IWGRP, 0);
	if(sem_chld==SEM_FAILED){
		printf("SEM ERROR: %d\n", errno);
		return -2;
	}
	pid_t child_pid = fork();
	if(child_pid< -1){
		printf("FORK ERROR: %d\n", errno);
		return -3;
	}
	if( child_pid ){
		char s;
		int status=1;
		while(status){
			sem_wait(sem_par);
			mmaps[0]=0;
			while((status=read(STDIN_FILENO, &s, sizeof(char)) ) >0){
				mmaps[0]+=1;
			//	printf("%d\n",mmaps[0]);
				mmaps[mmaps[0]]=s;
				if(s=='\n' || mmaps[0] == SIZE_OF_MAP - 1){ 
					break;
				}			
			}
			sem_post(sem_chld);
		
		}
		sem_wait(sem_par);
		mmaps[0]=0;
		sem_post(sem_chld);
		int k; 
		if(wait(&k) == -1){
			printf("WAIT PID ERROR: %d\n", errno);
			return -3;
		}
		sem_close(sem_par);
		sem_close(sem_chld);
		sem_unlink("readmap");
		sem_unlink("writemap");
		munmap(mmaps, SIZE_OF_MAP);
	}else{
		char* name=malloc(sizeof(char));
		int name_size=1, file_d=0;
		int i=0;
		int swtch1=1, swtch=0;
		while(1){
			sem_wait(sem_chld);
			if(!mmaps[0]) break;
			for(i=1; i<mmaps[0]+1; ++i){
				if(swtch1){
					switch(swtch){
						case 0: {
							if(!((mmaps[i]>='a'&& mmaps[i]<='z') || (mmaps[i]>='A'&& mmaps[i]<='Z') || (mmaps[i]=='\'') )) continue; 
							if(mmaps[i]=='\''){
								swtch=1;	
							}else{
								name[name_size-1]=mmaps[i];
								name=realloc(name, sizeof(char)*(++name_size));
								swtch=-1;
							}
							break;
						}
						case -1: {
							if((mmaps[i]>='a'&&mmaps[i]<='z'||mmaps[i]>='A' && mmaps[i]<='Z'|| mmaps[i]>='0' && mmaps[i]<='9' || mmaps[i]=='.')){
								name[name_size-1]=mmaps[i];
								name=realloc(name, sizeof(char)*(++name_size));
							}else{
								name[name_size-1]='\0';
								swtch=2;
							}
							break;
						}
						case 1:{
							if(mmaps[i]!='\''){
								name[name_size-1]=mmaps[i];
								name=realloc(name, sizeof(char)*(++name_size));
							}else{
								name[name_size-1]='\0';
								swtch=2;
								i+=1;
							}
							break;
						}	
						case 2:{
							file_d=open(name, O_WRONLY | O_CREAT, S_IRUSR | S_IWUSR);
							if(file_d==-1){
								printf("FILE ERROR: %d\n", errno );
								sem_post(sem_par);
								return -4;
							}
							i-=1;
							swtch1=0;
							break;
						}
					}	
				}else{
					if(!(mmaps[i]==' ' && mmaps[i+1]==' ')){ 
						write(file_d, &mmaps[i], sizeof(char));
					}
					if((i==SIZE_OF_MAP-2) && (mmaps[SIZE_OF_MAP-1]!=' '))
						write(file_d, &mmaps[i+1], sizeof(char));
				}
			}
			sem_post(sem_par);
		}
		sem_post(sem_par);
		close(file_d);
		free(name);
	}
	return 0;
}

