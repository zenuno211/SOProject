#include "argus.h"


int main(int argc, char const *argv[]){
	int fdW;
	int fdR;

	char buf[100];
	char buffer[100];
	int bytes_read = 0;
    char const ** args = malloc(sizeof(char**));
	char const comando;
    int i=0;
	int read_bytes=0;

	if(( fdW = open("fifo",O_WRONLY)) < 0){
		perror("open");
		exit(1);
	}


	if (argc ==1){

	write(1,"open is done\n",13);


	if(fork()==0){	
		while((bytes_read = read(0,buf,100)) > 0 ){

			if(write(fdW,buf,bytes_read) < 0){
				perror("write");
				exit(1);
			}
		}
	}
	
	else{
		
		if(fork()==0){	
			
			if(( fdR = open("fifo1",O_RDONLY)) < 0){
				perror("open");
				exit(1);
			}

			while((read_bytes = read(fdR, buffer, 100)) > 0){
			
				if(write(1,buffer,read_bytes) < 0){
					perror("write");
					exit(1);
				}



			}
			_exit(0);
		
		}
		wait(NULL);
		wait(NULL);	
	}

	}

	else {
        
		for(i=1; i<argc; i++){
			args[i-1] = argv[i];
		}
		

		
		for(i= 0; i<argc-1; i++){
			strcat(buffer,args[i]);
			strcat(buffer," ");
		}
		
		if(write(fdW,buffer,strlen(buffer)) < 0){
			perror("write");
			exit(1);
		}

		if(fork()==0){

			if(( fdR = open("fifo1",O_RDONLY)) < 0){
				perror("open");
				exit(1);
			}

			if(strncmp(buffer, "-m", 2) != 0 && strncmp(buffer, "-i", 2) != 0){
				
				int bytes = 0;
                char* recebido[1024];
                
                bytes = read(fdR,recebido,1024);
                write(1,recebido,bytes);
				
			}
			close(fdR);
			
		}
		wait(NULL);
	
	}

	close(fdR);

	
	return 0;
}
