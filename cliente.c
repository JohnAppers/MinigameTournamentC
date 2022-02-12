#include "cliente.h"
#include "player.h"

int gameStart = 0, kicked = 0;

void inicioJogo()
{
	gameStart = 1;
	printf("Os jogos vão começar.\n");
}

void kickedJogo()
{
	kicked = 1;	
	printf("Saindo do jogo...\n");
}

int main(int argc, char** argv)
{
	int fdw, fdr, pid, rbytes, nfd;
	char pidChar[6], buffer[40], command[30], message[35], nome[30];
	struct timeval tv;
	fd_set read_fds;
	
	pid = getpid();
	sprintf(pidChar,"%05d",pid);

	signal(SIGUSR1,kickedJogo);
	signal(SIGUSR2,inicioJogo);
	mkfifo(pidChar,0777);
	
	//CICLO DE CHAMPIONSHIP
	do
	{
		//ENVIO DO PID E NOME CLIENTE
		strncpy(buffer, pidChar, 5);
		printf("Insira um nome (10 char. máx): ");
		scanf("%s", nome);
		if(!strcmp(nome,"#quit"))
		{
			close(fdw);
			break;
		}
		strncpy(buffer + 5, nome, 10);
		
		fdw = open(MAINPIPE,O_WRONLY);
		if(write(fdw,buffer,sizeof(buffer))==0)
		{
			printf("Falha ao enviar pid e nome.\n");
			close(fdw);
			unlink(pidChar);
			exit(-1);
		}
		close(fdw);
		rbytes=0;
		fdr = open(pidChar,O_RDONLY);
		rbytes = read(fdr,buffer,sizeof(buffer));
		buffer[rbytes] = '\0';
		if(!strcmp(buffer,pidChar))
		{
			fprintf(stderr,"Ligacao rejeitada.\n");
			exit(-1);
		}
		if(rbytes == -1)
			fprintf(stderr,"Erro ao ler mensagem\n");
		close(fdr);
		
		printf("Esperando que os jogos comecem...\n");
		while(gameStart==0)
			sleep(1);
		
		fdr = open(pidChar,O_RDONLY | O_NONBLOCK);
		do
		{
			tv.tv_sec = 10;
			tv.tv_usec = 0;
			
			FD_ZERO(&read_fds);
			FD_SET(0, &read_fds);
			FD_SET(fdr, &read_fds);
			
			nfd = select(fdr+1, &read_fds, NULL, NULL, &tv);
			
			if(kicked)
				continue;
				
			if(nfd == 0)
			{
				printf("À espera de input...\n");
				fflush(stdout);
				continue;
			}
			
			if(nfd == -1)
			{
				fprintf(stderr,"Erro ao usar select.\n");
				close(fdr);
				unlink(pidChar);
				return -1;
			}
			
			if(FD_ISSET(0, &read_fds))
			{
				scanf("%s",command);
				fdw = open(MAINPIPE,O_WRONLY);
				if(!strcmp(command,"#mygame"))
				{
					sprintf(message,"%s#g",pidChar);
				}
				else if(!strcmp(command,"#quit"))
				{
					sprintf(message,"%s#q",pidChar);
				}
				else
				{
					sprintf(message,"%s%s",pidChar,command);
				}
				rbytes = write(fdw,message,sizeof(message));
				if(rbytes==-1)
					fprintf(stderr,"Erro a enviar mensagem.\n");
				close(fdw);
			}
			
			if(FD_ISSET(fdr, &read_fds))
			{
				char systemMessage[512];
				rbytes = read(fdr,systemMessage,sizeof(systemMessage));
				if(rbytes==0||rbytes==-1)
					continue;
				systemMessage[rbytes]='\0';
				printf("%s\n",systemMessage);
			}
			
		}while(strcmp(command,"#quit") && !kicked);
		if(strcmp(command,"#quit"))
		{
			sleep(0.5);
			rbytes = read(fdr,buffer,sizeof(buffer));
			if(rbytes > 0)
			{
				buffer[rbytes]='\0';
				printf("%s\n",buffer);
			}
		}
		close(fdr);
		gameStart = 0;
		kicked = 0;
	}while(strcmp(command,"#quit"));
	unlink(pidChar);
	return 0;
}
