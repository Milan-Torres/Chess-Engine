#include "boardManager.h"

typedef struct MovesList
{
    char *moves;
    unsigned char size;
    
} MovesList;

typedef struct KingRelatedSquares
{
    char kingPosition;
    char linesRelated[3]; /*Each position is 3 bit long
                          the first position is the minimal column to be concerned, the second the maximal, the third the minimal line and the forth the maximal
                          the fifth is the minimal first diagonal, the second the maximal, the third the minimal second diagonal and the forth the maximal
                          */
} KingRelatedSquares;


char isSquareVulnerable(unsigned char position, char color);

void setKingPosition(KingRelatedSquares *kingRelatedSquares, char position);

void updateKingRelatedPosition(KingRelatedSquares *kingRelatedSquares, char index);

char isSquareRelatedToKing(KingRelatedSquares *kingRelatedSquares, char position);

char isASide(unsigned char position, char side);

char canCastle(char* piece, char* position, char side);

MovesList getPieceMovement(KingRelatedSquares *kingRelatedSquares, char piece, unsigned char position, char checkVulnerability);