/*************************
*	 Nomeia Polígonos    *
*************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>

int pontos = 0;

void end_sig(int sig)
{
	exit(pontos);
}

int main(int argc, char** argv)
{
    int nLados;
	char nomePoligono[9][15] = {"Triangulo","Quadrado","Pentagono","Hexagono","Heptagono","Octogono","Eneagono","Decagono","Undecagono"};
	char resposta[30];
	
	printf("Este jogo chama-se nomeia polígonos.\n");
	fflush(stdout);
	printf("O objetivo é nomear corretamente os polígonos, sendo indicado o número de lados.\n");
	fflush(stdout);
	
	signal(SIGUSR1, end_sig);
	
    while(1)
	{
		nLados = rand() % 9 + 3; //número entre 3 e 11
		printf("Como se chama um polígono de %d lados? \n", nLados);
		fflush(stdout);
		fflush(stdin);
		scanf("%s", resposta);
		fflush(stdin);
		if(!strcmp(resposta,nomePoligono[nLados-3]))
		{
			printf("Resposta correta!\n");
			fflush(stdout);
			pontos++;
		}
		else
		{
			printf("Resposta %s errada, a correta seria %s.\n",resposta,nomePoligono[nLados-3]);
			fflush(stdout);
			if(pontos>0) pontos--;
		}
	}
}
