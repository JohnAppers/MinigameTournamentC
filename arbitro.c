#include "arbitro.h"
#include "player.h"

int gameOver = 0;
int setMaxPlayers()
{
	int maxPlayers;
	char* getMaxPlayers;
	getMaxPlayers = getenv("MAXPLAYERS");
	if(getMaxPlayers == NULL || atoi(getMaxPlayers)<0)
	{
		char choice;
		printf("Número máximo de jogadores não definido, definir? s/n ");
		scanf("%c", &choice);
		if(choice == 's')
		{
			printf("Máximo de jogadores: ");
			scanf("%d", &maxPlayers);
		}
		else
		{
			maxPlayers = 30;
		}
	}
	else
	{
		maxPlayers = atoi(getMaxPlayers);
	}
	if(maxPlayers > 30) maxPlayers = 30;
	if(maxPlayers < 2) maxPlayers = 2;
	return maxPlayers;
}

int validatePlayerName(struct Player* players, char* nome, int playerCount)
{
	for(int i = 0; i < playerCount; i++)
	{
		if(!strcmp(players[i].nome, nome))
			return 0;
	}
	return 1;
}

void printSintaxe()
{
	fprintf(stderr,"Sintaxe: ./arbitro -t duracao_do_campeonato(1-300) -e tempo_de_espera(1-30)\n");
	exit(1);
}

void sigalrmDuration()
{
	gameOver = 1;
}

int main(int argc, char** argv)
{
	char* gamesFolder;
	char pidChar[6], buffer[40], command[30];
	int maxPlayers, playerCount = 0, opt, duration, waitTime, tOpt = 0, eOpt = 0, fd, fdplayer, nfd;
	int rbytes, playerWait = 0, gameCount = 0;
	struct Player players[30];
	struct timeval tv;
	fd_set read_fds;
	DIR* d;
	struct dirent *dir;
	
	if(argc != 5)
		printSintaxe();
	
	while((opt=getopt(argc,argv,"t:e:"))!=-1)
	{
		switch(opt)
		{
			case 't':
				if(!tOpt) tOpt = 1;
				else printSintaxe();
				duration = atoi(optarg);
				if(duration<1 || duration > 300)
					printSintaxe();
				break;
			case 'e':
				if(!eOpt) eOpt = 1;
				else printSintaxe();
				waitTime = atoi(optarg);
				if(waitTime < 1 || waitTime > 30)
					printSintaxe();
				break;
			case '?':
				printSintaxe();
				break;
		}
	}
	
	maxPlayers = setMaxPlayers();
	
	//LER FICHEIROS DE JOGO
	gamesFolder = getenv("GAMEDIR");
	if(gamesFolder == NULL)
		d = opendir(".");
	else
		d = opendir(gamesFolder);
	if(d)
	{
		while((dir = readdir(d)) != NULL)
		{
			if(dir->d_name[0] == 'g' && dir->d_name[1] == '_' && dir->d_name[5] != '.')
				gameCount++;
		}
		closedir(d);
	}
	
	mkfifo(MAINPIPE,0777);
	
	//CICLO PRINCIPAL CHAMPIONSHIP
	do
	{
		playerCount = 0;
		playerWait = 0;
		gameOver = 0;
		command[0]='0';
		printf("Esperando por jogadores...\n");
		fd = open(MAINPIPE,O_RDONLY | O_NONBLOCK);
		
		//LER PID DOS CLIENTES
		do
		{
			rbytes = read(fd, buffer, sizeof(buffer));
			if(rbytes>0)
			{
				buffer[rbytes]='\0';
				strncpy(pidChar, buffer, 5);
				pidChar[5] = '\0';
				strncpy(players[playerCount].nome, buffer + 5, 10);
				if(!validatePlayerName(players,players[playerCount].nome, playerCount))
				{
					int responseFd = open(pidChar,O_WRONLY);
					if(write(responseFd, pidChar, sizeof(pidChar)) == -1)
						fprintf(stderr,"Erro ao enviar aviso ao jogador\n");
					close(responseFd);
					continue;
				}
				int responseFd = open(pidChar,O_WRONLY);
				if(write(responseFd, "OK", 2) == -1)
					fprintf(stderr,"Erro ao enviar aviso ao jogador\n");
				close(responseFd);
				players[playerCount].pid = atoi(pidChar);
				players[playerCount].jogo = rand() % gameCount;
				players[playerCount].pontos = 0;
				players[playerCount].suspensao = 0;
				playerCount++;
				printf("Jogador %s adicionado com PID %d\n",players[playerCount-1].nome, atoi(pidChar));
				rbytes=0;
			}
			if(playerCount > 1 && playerCount != maxPlayers)
			{
				printf("Iniciando em %d...\n",(waitTime-playerWait));
				sleep(1);
				playerWait++;
			}
		}while (playerWait < waitTime && playerCount < maxPlayers);
		
		//ATRIBUIR JOGOS AOS CLIENTES
		for(int i = 0;i < playerCount; i++)
		{
			int counter = 0;
			if(gamesFolder == NULL)
				d = opendir(".");
			else
				d = opendir(gamesFolder);
			while((dir = readdir(d)) != NULL)
			{
				if(dir->d_name[0] == 'g' && dir->d_name[1] == '_' && dir->d_name[5] != '.')
					if(counter == players[i].jogo)
					{
						strcpy(players[i].nomeJogo, dir->d_name);
					}
					else
						counter++;
			}
			closedir(d);
		}
		//INICIAR JOGOS
		for(int i = 0;i < playerCount; i++)
		{
			if(kill(players[i].pid, SIGUSR2)==-1)
				fprintf(stderr,"Erro a avisar %d do início.\n", players[i].pid);
				
			char tempPid[6];
			sprintf(tempPid,"%05d",players[i].pid);
			int responseFd = open(tempPid,O_WRONLY);
			char gameName[100];
			sprintf(gameName,"O seu jogo é o %s\n",players[i].nomeJogo);
			if(write(responseFd, gameName, sizeof(gameName)) == -1)
				fprintf(stderr,"Erro ao enviar aviso ao jogador\n");
			close(responseFd);
			
			
			if(pipe(players[i].gamePipeRead) == -1)
				fprintf(stderr,"Erro ao abrir pipe de leitura n%d\n",i);
			if(pipe(players[i].gamePipeWrite) == -1)
				fprintf(stderr,"Erro ao abrir pipe de escrita n%d\n",i);
			
			int childPid = fork();
			if(childPid == 0)
			{
				if(close(players[i].gamePipeRead[1])==-1)
					fprintf(stderr,"Erro ao fechar escrita no pipe de leitura do filho\n");
				if(close(players[i].gamePipeWrite[0])==-1)
					fprintf(stderr,"Erro ao fechar leitura no pipe de escrita do filho\n");
				
				if(dup2(players[i].gamePipeRead[0],0) == -1)
					fprintf(stderr,"Erro ao duplicar leitura no pipe de leitura do filho\n");
				if(close(players[i].gamePipeRead[0])==-1)
					fprintf(stderr,"Erro ao fechar leitura no pipe de leitura do filho\n");
					
				if(dup2(players[i].gamePipeWrite[1],1) == -1)
					fprintf(stderr,"Erro ao duplicar escrita no pipe de escrita do filho\n");
				if(close(players[i].gamePipeWrite[1])==-1)
					fprintf(stderr,"Erro ao fechar escrita no pipe de escrita do filho\n");
				
				char gamePath[50];
				if(gamesFolder != NULL)
					sprintf(gamePath,"%s/%s", gamesFolder, players[i].nomeJogo);
				else
					sprintf(gamePath,"./%s", players[i].nomeJogo);
				execl(gamePath, gamePath, (char*)NULL);
				
				fprintf(stderr,"Erro na instrucao execl\n");
				return -1;
			}
			if(childPid == -1)
				fprintf(stderr, "Erro ao executar fork()\n");
			
			players[i].gamePid = childPid;
			
			if(close(players[i].gamePipeRead[0])==-1)
					fprintf(stderr,"Erro ao fechar leitura no pipe de leitura do pai\n");
			if(close(players[i].gamePipeWrite[1])==-1)
					fprintf(stderr,"Erro ao fechar escrita no pipe de escrita do pai\n");
		}
		
		signal(SIGALRM,sigalrmDuration);
		alarm(duration);
		//CICLO PRINCIPAL DE LEITURA
		do
		{
			tv.tv_sec = 10;
			tv.tv_usec = 0;
			
			FD_ZERO(&read_fds);
			FD_SET(0, &read_fds);
			FD_SET(fd, &read_fds);
			int selectFd = fd;
			for(int i = 0; i < playerCount; i++)
			{
				FD_SET(players[i].gamePipeWrite[0], &read_fds);
				if(selectFd < players[i].gamePipeWrite[0])
					selectFd = players[i].gamePipeWrite[0];
			}
			
			nfd = select(selectFd+1, &read_fds, NULL, NULL, &tv);
			
			if(nfd == 0)
			{
				printf("À espera de input...\n");
				fflush(stdout);
				continue;
			}
			
			if(nfd == -1)
			{
				fprintf(stderr,"Erro ao usar select.\n");
				close(fd);
				unlink(MAINPIPE);
				return -1;
			}
			
			if(FD_ISSET(0, &read_fds))
			{
				scanf("%s",command);
				if(!strcmp(command,"players"))
				{
					printf("Jogadores:\n");
					for(int i=0;i<playerCount;i++)
					{
						printf(" - %s a jogar %s\n", players[i].nome, players[i].nomeJogo);
					}
				}
				if(!strcmp(command,"games"))
				{
					printf("Jogos:\n");
					if(gamesFolder == NULL)
						d = opendir(".");
					else
						d = opendir(gamesFolder);
					if(d)
					{
						while((dir = readdir(d)) != NULL)
						{
							if(dir->d_name[0] == 'g' && dir->d_name[1] == '_' && strlen(dir->d_name) == 5)
								printf(" - %s\n",dir->d_name);
						}
						closedir(d);
					}
				}
				if(command[0]=='k')
				{
					int index = -1;
					for(int i=0;i<playerCount;i++)
					{
						if(strcmp(command+1,players[i].nome)==0)
							index=i;
					}
					if(index==-1)
					{
						printf("Jogador não foi encontrado.\n");
						continue;
					}
					
					if(kill(players[index].pid,SIGUSR1)==-1)
						fprintf(stderr,"Erro a enviar sinal de remoção.");
					char pidGame[6];
					sprintf(pidGame,"%05d",players[index].gamePid);
					kill(players[index].gamePid,SIGUSR1);
					players[index].pid=players[playerCount-1].pid;
					players[index].jogo=players[playerCount-1].jogo;
					players[index].pontos=players[playerCount-1].pontos;
					players[index].suspensao=players[playerCount-1].suspensao;
					players[index].gamePid=players[playerCount-1].gamePid;
					players[index].gamePipeRead[1]=players[playerCount-1].gamePipeRead[1];
					players[index].gamePipeWrite[0]=players[playerCount-1].gamePipeWrite[0];
					strcpy(players[index].nome,players[playerCount-1].nome);
					strcpy(players[index].nomeJogo,players[playerCount-1].nomeJogo);
					playerCount--;
					printf("Jogador %s removido.\n",pidChar);
				}
				if(command[0]=='s')
				{
					int index = -1;
					for(int i=0;i<playerCount;i++)
					{
						if(strcmp(command+1,players[i].nome)==0)
							index=i;
					}
					if(index==-1)
					{
						printf("Jogador não foi encontrado.\n");
						continue;
					}
					if(!players[index].suspensao)
					{
						players[index].suspensao = 1;
						printf("%s foi suspendido.\n",players[index].nome);
					}
					else
					{
						printf("Jogador ja se encontra em suspensao.\n");
					}
				}
				
				if(command[0]=='r')
				{
					int index = -1;
					for(int i=0;i<playerCount;i++)
					{
						if(strcmp(command+1,players[i].nome)==0)
							index=i;
					}
					if(index==-1)
					{
						printf("Jogador não foi encontrado.\n");
						continue;
					}
					if(players[index].suspensao)
					{
						players[index].suspensao = 0;
						printf("%s foi retirado de suspensao.\n",players[index].nome);
					}
					else
					{
						printf("Jogador nao se encontra em suspensao.\n");
					}
				}
			}
			
			//VER OUTPUT DE JOGOS
			for(int i = 0;i<playerCount;i++)
			{
				if(players[i].suspensao)
					continue;
					
				if(FD_ISSET(players[i].gamePipeWrite[0], &read_fds))
				{
					char gameMessage[510];
					rbytes = read(players[i].gamePipeWrite[0], gameMessage, sizeof(gameMessage));
					if(rbytes==0 || rbytes==-1)
						continue;
					gameMessage[rbytes] = '\0';
					sprintf(pidChar,"%05d",players[i].pid);
					int responseFd = open(pidChar,O_WRONLY);
					if(write(responseFd, gameMessage, sizeof(gameMessage)) == -1)
							fprintf(stderr,"Erro ao reenviar info ao jogador %s\n",players[i].nome);
					close(responseFd);
				}
			}
			
			//VER INPUT DE CLIENTES
			if(FD_ISSET(fd, &read_fds))
			{
				rbytes = read(fd, buffer, sizeof(buffer));
				if(rbytes==0 || rbytes==-1)
					continue;
				buffer[rbytes] = '\0';
				strncpy(pidChar,buffer,5);
				pidChar[5] = '\0';
				int index = -1, npid = atoi(pidChar);
				for(int i=0;i<playerCount;i++)
				{	
					if(players[i].pid==npid)
					{
						index=i;
						break;
					}
				}
				if(index==-1)
				{
					fprintf(stderr,"Erro a encontrar pid %s, rejeitando ligacao...\n",pidChar);
					int responseFd = open(pidChar,O_WRONLY);
					if(write(responseFd, pidChar, sizeof(pidChar)) == -1)
							fprintf(stderr,"Erro ao enviar aviso de rejeicao\n");
					close(responseFd);
					continue;
				}
				
				if(players[index].suspensao)
					continue;
					
				if(buffer[5] == '#') //SE FOR UM COMANDO
				{
					if(buffer[6]=='g') //#mygame
					{
						fdplayer = open(pidChar,O_WRONLY);
						if(write(fdplayer,players[index].nomeJogo,sizeof(players[index].nomeJogo))==-1)
							fprintf(stderr,"Erro ao responder ao pid %s\n",pidChar);
						close(fdplayer);
					}
					if(buffer[6]=='q') //#quit
					{
						kill(players[index].gamePid,SIGUSR1);
						players[index].pid=players[playerCount-1].pid;
						players[index].jogo=players[playerCount-1].jogo;
						players[index].pontos=players[playerCount-1].pontos;
						players[index].suspensao=players[playerCount-1].suspensao;
						players[index].gamePid=players[playerCount-1].gamePid;
						players[index].gamePipeRead[1]=players[playerCount-1].gamePipeRead[1];
						players[index].gamePipeWrite[0]=players[playerCount-1].gamePipeWrite[0];
						strcpy(players[index].nome,players[playerCount-1].nome);
						strcpy(players[index].nomeJogo,players[playerCount-1].nomeJogo);
						playerCount--;
						printf("Jogador %s removido.\n",pidChar);
					}
				}
				else //SE FOR UMA RESPOSTA AO JOGO
				{
					buffer[rbytes] = '\n';
					if(write(players[index].gamePipeRead[1], buffer+5, sizeof(buffer)-5)==-1)
						fprintf(stderr,"Erro ao responder ao jogo\n");
				}
			}
		}while(strcmp(command,"end") && !gameOver && strcmp(command,"exit"));
		
		//TERMINAR JOGOS E GUARDAR PONTUAÇÃO
		for(int i=0;i<playerCount;i++)
		{
			if(kill(players[i].gamePid,SIGUSR1)==-1)
				fprintf(stderr,"Erro a enviar sinal ao jogo.\n");
			int points;
			if(waitpid(players[i].gamePid, &points, 0) == -1)
				fprintf(stderr,"Erro waitpid\n");
			players[i].pontos = WEXITSTATUS(points);
			printf("%d pontos para %s\n", WEXITSTATUS(points), players[i].nome);
		}
		
		//TERMINAR CLIENTES E ENVIAR PONTUAÇÃO
		for(int i=0;i<playerCount;i++)
		{
			if(kill(players[i].pid,SIGUSR1)==-1)
				fprintf(stderr,"Erro a enviar sinal ao cliente.\n");
			sprintf(buffer, "Fim do jogo, conseguiste %d pontos!\n", players[i].pontos);
			sprintf(pidChar,"%05d",players[i].pid);
			int responseFd = open(pidChar,O_WRONLY);
			if(write(responseFd, buffer, sizeof(buffer)) == -1)
				fprintf(stderr,"Erro ao enviar pontuacao\n");
			close(responseFd);
		}
		close(fd);
	}while(strcmp(command,"exit"));
	unlink(MAINPIPE);
	return 0;
}
