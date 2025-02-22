all: chess-engine 

chess-engine:
	gcc boardManager.c main.c memoryUtils.c movementManager.c UIManager.c -lm -o chess-engine 

clean:
	 rm chess-engine
