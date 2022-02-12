all: cliente árbitro jogo001

cliente: cliente.o
	gcc cliente.o -o cliente -Wall cliente.h

árbitro: arbitro.o
	gcc arbitro.o -o arbitro -Wall arbitro.h player.h

jogo001: g_001.o
	gcc g_001.o -o g_001 -Wall

cliente.o:
	gcc cliente.c -c -o cliente.o -Wall

árbitro.o:
	gcc arbitro.c -c -o arbitro.o -Wall
	
jogo001.o:
	gcc g_001.c -c -o g_001.o -Wall

clean:
	rm arbitro cliente g_001 *.o
