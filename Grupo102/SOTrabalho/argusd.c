#include "argus.h"

typedef struct tarefa{
    int pid_tarefa;
    char* estado;
}Tarefa;

Tarefa* Tarefas;




int executa_comando(char* comando){
    char** exec_args = (char**) malloc(sizeof(char**));
    char* string;
    char* aux;
    int i = 0;

    string = strtok(comando, " ");

    while(string!=NULL){
        exec_args[i]=string;
        string=strtok(NULL," ");
        i++;
    }

    exec_args[i] = NULL;

    
    execvp(exec_args[0],exec_args);
    
    return 0;
}


int *pids;
int nr_pids = 0;
int tempo_execucao = 0;
int tempo_inatividade = 0;
int nr_tarefas = 0;
int pid_tarefa = 0;
char**estados;
int status;


void timeout_handler(int sig){
    for(int i = 0; i < nr_pids; i++){
        if(pids[i] > 0){
            kill(pids[i] , SIGKILL);
        }   
    }
    kill(pid_tarefa,SIGKILL);
}

/*
void timeComunic_handler(int sig){
    for(int i = 0; i < nr_pids; i++){
        printf("terminou %d\n", pids[i]);
        if(pids[i] > 0){
            kill(pids[i] , SIGKILL);
        }   
    }
    printf("Interrompeu a tarefa com pid %d\n",pid_tarefa);
    estados[nr_tarefas] = "max inatividade";
    kill(pid_tarefa,SIGKILL);
}
*/

void extremina_handler(int sig){
    for(int i = 0; i < nr_pids; i++){
        if(pids[i] > 0){
            kill(pids[i] , SIGKILL);
        }   
    }
    //kill(pid_tarefa,SIGKILL);
}


int executa(char* buf){
    int nr_comandos = 0;
    char* string; 
    char* comando;  
    char** comandos = malloc(sizeof(char**));
    pids = (int*) malloc(sizeof(int));
    int status;
    int pid;
    
    string = strdup(buf);
    
    /*
    if(signal(SIGALRM, timeComunic_handler) < 0){
        perror("signal");
        exit(-1);   
    }*/

    for(nr_comandos = 0; (comando = strsep(&string,"|")) != NULL; nr_comandos++){
        comandos[nr_comandos] = strdup(comando);
    }

    int pipe_fd[nr_comandos-1][2];

    if(nr_comandos == 1){
        if((pid = fork())==0){
            executa_comando(comandos[0]);
            _exit(0);
        }
        else{
            pids[nr_pids++] = pid;
            signal(SIGCHLD, SIG_IGN);
            wait(NULL);
        }
    }

    else{
        for(int c = 0; c < nr_comandos; c++){
            if(c == 0){
                //alarm(tempo_inatividade);
                if(pipe(pipe_fd[0]) < 0){
                    perror("pipe");
                    exit(-1);
                }
                if((pid = fork()) == 0){
                    close(pipe_fd[0][0]);
                    dup2(pipe_fd[0][1],1);
                    close(pipe_fd[0][1]);
                    executa_comando(comandos[0]);
                    _exit(0);
                }
                else {
                    pids[nr_pids++] = pid;
                    close(pipe_fd[0][1]);
                    signal(SIGCHLD, SIG_IGN);
                }        
            }
            else if(c == nr_comandos -1){
                //alarm(tempo_inatividade);
                if((pid = fork()) == 0){
                    dup2(pipe_fd[c-1][0],0);
                    close(pipe_fd[c-1][0]);
                    executa_comando(comandos[c]);
                    _exit(0);
                }
                else{
                    pids[nr_pids++] =  pid;
                    close(pipe_fd[c-1][0]);
                    signal(SIGCHLD, SIG_IGN);
                }
            }
            else {
                //alarm(tempo_inatividade);
                if(pipe(pipe_fd[c]) < 0){
                    perror("pipe");
                    exit(-1);
                }
                if((pid = fork()) == 0){
                    close(pipe_fd[c][0]);
                    dup2(pipe_fd[c][1],1);
                    close(pipe_fd[c][1]);
                    dup2(pipe_fd[c-1][0],0);
                    close(pipe_fd[c-1][0]);
                    executa_comando(comandos[c]);
                    _exit(0);
                }
                else{
                    pids[nr_pids++] = pid;
                    close(pipe_fd[c][1]);
					close(pipe_fd[c-1][0]);
                    signal(SIGCHLD, SIG_IGN);
                }
            }
        }
    }
    
    for(int i = 0; i < nr_comandos; i++){
        wait(NULL);
    }
    return 0;
}

int keep;
int k = 0;
void child_handler(int sig){
    keep = wait(&status);
    if(WEXITSTATUS(status) == 0){

        for( k = 1; k<= nr_tarefas && keep!= Tarefas[k].pid_tarefa; k++);
        Tarefas[k].estado = "Max Execucao";
    }
    else{
        Tarefas[WEXITSTATUS(status)].estado = "Concluida";
    }
}


int main(int argc, char const *argv[]){

    int read_bytes = 0;
    int bytes_read = 0;
    char buf[100];
    estados = malloc(sizeof(char**));
    char** tarefa_nome = malloc(sizeof(char**));
    int pid = -1;
    Tarefas = (Tarefa*) malloc(sizeof(struct tarefa));
    char* buffer = malloc(sizeof(char*));
    char* message = malloc(sizeof(char*));
    char* historico = malloc(sizeof(char*));
    char* interruption = malloc(sizeof(char*));
    char* output = malloc(sizeof(char*));

    if(mkfifo("fifo", 0666) == -1){
            perror("mkfifo");
        }

    
    if(mkfifo("fifo1", 0666) == -1){
            perror("mkfifo");
        }

    

    if(signal(SIGALRM, timeout_handler) < 0){
        perror("signal");
        exit(-1);   
    }


    if(signal(SIGCHLD, child_handler) < 0){
                perror("signal");
                exit(-1);   
    }

    if(signal(SIGQUIT, extremina_handler) < 0){
                perror("signal");
                exit(-1);   
    }
    


    int fifo_fdR = -1;
    int fifo_fdW = -1;
    while(1){
        if((fifo_fdR = open("fifo", O_RDONLY)) == -1){
            perror("fifo");
        }
        
        if((fifo_fdW = open("fifo1", O_WRONLY)) == -1){
            perror("fifo");
        }
        


        while((read_bytes = read(fifo_fdR, buf, 100)) > 0){
            buf[read_bytes-1] = '\0';


             if(strncmp(buf,"historico",9) == 0 || strncmp(buf,"-r", 2) == 0){
                 
                 int count=0 ;
                    for(int h=1;h<=nr_tarefas; h++){
                            if(kill(Tarefas[h].pid_tarefa,0) != 0){
                                count++;
                            }
                    }
                 
                 if(nr_tarefas>0 && count>0){
                    for(int i=1;i<=nr_tarefas;i++){
                        if(kill(Tarefas[i].pid_tarefa,0) != 0){
                            sprintf(historico,"#%d: %s: %s\n",i,Tarefas[i].estado,tarefa_nome[i]);
                            write(fifo_fdW,historico,strlen(historico));
                        }
                        
                    }
                 }else {
                         write(fifo_fdW,"Ainda nao existem dados no historico\n",37);
                        }
             }
            

            if(strncmp(buf,"terminar",8) == 0 || strncmp(buf,"-t",2) == 0){
                if(nr_tarefas>0){ 
                    if(strncmp(buf,"terminar",8) == 0){
                         memmove(buf,buf+9,strlen(buf+9)+1);
                    }
                    else{
                        memmove(buf,buf+3,strlen(buf+3)+1);
                    }
                    int tarefa_finish = atoi(buf);
                    if(kill(Tarefas[tarefa_finish].pid_tarefa,0) == 0){
                        kill(Tarefas[tarefa_finish].pid_tarefa,SIGQUIT);
                        sprintf(interruption,"A tarefa %d com pid %d foi interrompida\n",tarefa_finish,Tarefas[tarefa_finish].pid_tarefa);
                            write(fifo_fdW,interruption,strlen(interruption));
                    }
                    else{
                        write(fifo_fdW,"A Tarefa que tentou terminar ja foi concluida\n",46);
                    }
                }else{
                    write(fifo_fdW,"Ainda nao forma executadas tarefas\n",35);
                }
            }

            
            if(strncmp(buf,"ajuda",5) == 0 || strncmp(buf,"-h", 2) == 0){ 
                    write(fifo_fdW,">tempo-inatividade segs\n>tempo-execucao segs\n>executar p1 | p2 | ... | pn\n>listar\n>terminar 'numero de tarefa'\n>historico\n>ajuda\n",129);
		    }

            if(strncmp(buf,"listar",6) ==0 || strncmp(buf,"-l",2) ==0){
                    
                    int contador= 0 ;
                    for(int j=1;j<=nr_tarefas; j++){
                            if(kill(Tarefas[j].pid_tarefa,0) != 0){
                                contador++;
                            }
                    }

                    if(nr_tarefas>0 && nr_tarefas>contador){
                        for(int j=1;j<=nr_tarefas; j++){
                            if(kill(Tarefas[j].pid_tarefa,0) == 0){
                                sprintf(buffer,"#%d: %s\n",j,tarefa_nome[j]);
                                write(fifo_fdW,buffer,strlen(buffer));
                            }
                        }
                    }else{
                        write(fifo_fdW,"Nao existem tarefas em execucao\n",32);
                    }
            }

            if(strncmp(buf,"tempo-execucao",14) == 0 || strncmp(buf,"-m",2) == 0){ 
                    if(strncmp(buf,"tempo-execucao",14) == 0){
                        memmove(buf,buf+15,strlen(buf+15)+1);
                    }
                    else{
                        memmove(buf,buf+3,strlen(buf+3)+1);
                    }
                    tempo_execucao = atoi(buf);  
            }

            if(strncmp(buf,"tempo-inatividade",17) == 0 || strncmp(buf,"-i",2) == 0){ 
                    if(strncmp(buf,"tempo-inatividade",17) == 0){
                        memmove(buf,buf+18,strlen(buf+18)+1);
                    }
                    else{
                        memmove(buf,buf+3,strlen(buf+3)+1);   
                    }
                    tempo_inatividade = atoi(buf);  
            }
            
            
            if( strncmp(buf,"executar",8) == 0 || strncmp(buf, "-e", 2) == 0){
                    if(strncmp(buf,"executar",8) == 0){
                        memmove(buf,buf+9,strlen(buf+9)+1);
                    }
                    else{
                        memmove(buf,buf+3,strlen(buf+3)+1);
                    }
                    nr_tarefas++;
                    sprintf(output,"nova tarefa #%d\n",nr_tarefas);
                    write(fifo_fdW,output,strlen(output));
                    tarefa_nome[nr_tarefas] = strdup(buf);

                    if((pid = fork()) == 0){

                            pid_tarefa = getpid();
                            alarm(tempo_execucao);
                            dup2(fifo_fdW, 1);
                            executa(buf);
                            _exit(nr_tarefas);
                    }
                    else{
                         Tarefas[nr_tarefas].pid_tarefa = pid;

                         
                    }

            }

    }
    
    close(fifo_fdW);
    close(fifo_fdR);
    }
    
    return 0;
}
