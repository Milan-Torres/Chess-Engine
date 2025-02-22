all: chess_engine 

chess_engine:
	gcc boardManager.c main.c memoryUtils.c movementManager.c UIManager.c -lm -o chess_engine 

clean:
	 rm chess_engine
