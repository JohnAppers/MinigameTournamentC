#ifndef PLAYER_H
#define PLAYER_H

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <signal.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <ctype.h>
#include <dirent.h>

struct Player
{
	char nome[10], nomeJogo[10];
	int pid;
	int jogo;
	int pontos;
	int suspensao;
	int gamePid;
	int gamePipeRead[2];
	int gamePipeWrite[2];
};

#endif