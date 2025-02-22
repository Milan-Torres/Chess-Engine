#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

#include "UIManager.h"

char board[32] = {0}; //a basic chess board has 64 squares, each square is 4 bits long, index 0 corresponds to a8, index 31 corresponds to h1
char turnOf = 0; //the player who's playing, 0 -> whites, 1 -> blacks
char checked = 0; //is the player currently playing checked
char lastPushedPawn = -1; //the coordinates of the last pushed pawn of the game, useful for the en passant
char hasKingMoved[2] = {0}; //self explanatory 
char hasRookMoved[4] = {0}; //indexes: 1 left white, 2 right white, 3 left black, 4 right black
char turnCount = 0; //the number of turns since the last pawn movement or the last piece capture, useful for the 50 moves rule
char pieceCount[2] = {28, 28}; // every piece has a value of 2 but the knight and the bishop which have a value of 1 and the king which has no value, when a piece is captured the value of its corresponding piece count is decreased when the piece count of both side is inferior to 2 it's a draw

KingRelatedSquares kingsRelatedSquares[2]; // 0 for the whites and 1 for the blacks as usual

void setupBasePosition()
{
    board[0] = ((ROOK | 1) << 4) | (KNIGHT | 1);
    board[1] = ((BISHOP | 1) << 4) | (QUEEN | 1);
    board[2] = ((KING | 1) << 4) | (BISHOP | 1);
    board[3] = ((KNIGHT | 1) << 4) | (ROOK | 1);

    for (unsigned char i = 4; i < 8; i++)
        board[i] = ((PAWN | 1) << 4) | (PAWN | 1);

    for (unsigned char i = 24; i < 28; i++)
        board[i] = (PAWN << 4) | PAWN;

    board[28] = (ROOK << 4 | KNIGHT);
    board[29] = (BISHOP << 4 | QUEEN);
    board[30] = (KING << 4 | BISHOP);
    board[31] = (KNIGHT << 4 | ROOK);

    setKingPosition(&kingsRelatedSquares[0], 60);
    setKingPosition(&kingsRelatedSquares[1], 4);
}

unsigned char askForPosition()
{
    char file, rank;
    scanf("%c%c", &file, &rank);
    while (getchar() != '\n')
        ;

    file = (char)toupper(file); // Convert the file character to uppercase

    if (file < 'A' || file > 'H') {
        printf("Invalid file, please provide the position in the following format: \'A1\'\n");
        return askForPosition();
    }
    if (rank < '1' || rank > '8') {
        printf("Invalid rank, please provide the position in the following format: \'A1\'\n");
        return askForPosition();
    }

    return (('8' - rank) * 8) + (file - 'A');
}

char selectPiece(char *selectedPosition)
{
    printf("Select the position of the piece you want to move:\n");
    unsigned char scopeSelectedPosition = askForPosition();
    char selectedPiece = getPieceAt(scopeSelectedPosition);

    if (selectedPiece == EMPTY)
    {
        printf("You cannot select an empty square\n");
        return selectPiece(selectedPosition);
    }

    if ((selectedPiece & 0x01) != turnOf)
    {
        printf("You cannot select a piece of the opposing side\n");
        return selectPiece(selectedPosition);
    }

    printf("Selected position: %d\n", scopeSelectedPosition);
    *selectedPosition = scopeSelectedPosition;
    return selectedPiece;
}

char handleMovementRelationWithOpposingKing(char *basePos, char *finalPos, char *isEnPassant)
{
    // return true if the movement is related to the opposing king, false otherwise
    KingRelatedSquares *opposingKingRelatedSquaresPtr = &(kingsRelatedSquares[turnOf ^ 0b01]);

    char basePosRelation = isSquareRelatedToKing(opposingKingRelatedSquaresPtr, *basePos);
    if (basePosRelation)
        updateKingRelatedPosition(opposingKingRelatedSquaresPtr, basePosRelation - 1);

    char enPassantRelation = 0;
    if (*isEnPassant)
    {
        enPassantRelation = isSquareRelatedToKing(opposingKingRelatedSquaresPtr, ((*finalPos) + (turnOf ? -8 : 8)));
        if (enPassantRelation)
            updateKingRelatedPosition(opposingKingRelatedSquaresPtr, enPassantRelation - 1);
    }

    char finalPosRelation = isSquareRelatedToKing(opposingKingRelatedSquaresPtr, *finalPos);
    if (finalPosRelation && finalPosRelation <= 8) 
        updateKingRelatedPosition(opposingKingRelatedSquaresPtr, finalPosRelation - 1);

    return basePosRelation || enPassantRelation || finalPosRelation;
}

char handleChecks()
{
    // returns true if there is a checkmate, false otherwise

    if (isSquareVulnerable(kingsRelatedSquares[turnOf ^ 0b01].kingPosition, turnOf))
    {
        printf("Check\n");
        checked = 1;
        for (unsigned char square = 0; square < 64; square++)
        {
            char piece = getPieceAt(square);
            if (piece == EMPTY || (piece & 0x01) == turnOf)
                continue;

            MovesList moves = getPieceMovement(&kingsRelatedSquares[turnOf ^ 0b01], piece, square, 1);
            if (moves.size > 0)
            {
                free(moves.moves);
                return 0;
            }
            free(moves.moves);
        }

        printf("Checkmate\n");
        return 1;
    }

    checked = 0;
    return 0;
}

char handleDraw()
{
    // returns true if there is a draw, false otherwise
    if (turnCount >= 50) {
        printf("Draw by 50 moves rule\n");
        return 1;
    }

    if (pieceCount[0] < 2 && pieceCount[1] < 2) {
        printf("Draw by insufficient material\n");
        return 1;
    }

    if (isSquareVulnerable(kingsRelatedSquares[turnOf ^ 0b01].kingPosition, turnOf))
        return 0;

    for (unsigned char square = 0; square < 64; square++)
    {
        char piece = getPieceAt(square);
        if (piece == EMPTY || (piece & 0x01) == turnOf)
            continue;

        MovesList moves = getPieceMovement(&kingsRelatedSquares[turnOf ^ 0b01], piece, square, 1);
        if (moves.size > 0)
        {
            free(moves.moves);
            return 0;
        }
        free(moves.moves);
    }

    printf("Draw by stalemate\n");
    return 1;
}

void handleTurnCountReset(char *selectedPiece, char *origin, char *destination)
{
    // reset the turn count if a pawn is moved or a piece is captured
    if ((*selectedPiece & 0b1110) == PAWN || getPieceAt(*destination) != EMPTY)
        turnCount = 0;
}

char handleEnPassant(char *selectedPiece, char *destination)
{
    // return true if it is an en passant, false otherwise
    if (lastPushedPawn != -1 && (*selectedPiece & 0b1110) == PAWN && *destination == (((2 + ((*selectedPiece & 0x01) * 3)) << 3) | lastPushedPawn)) {
        setPiece(EMPTY, (((3 + ((*selectedPiece & 0x01) * 2)) << 3) | lastPushedPawn));
        return 1;
    }

    return 0;
}

char handleKingMovement(char *selectedPiece, char *origin, char *destination)
{
    // return true if a castle occured, false otherwise

    if ((*selectedPiece & 0b1110) != KING) {
        return 0;
    }

    if ((*selectedPiece & 0b1110) == KING)
    {
        setKingPosition(&kingsRelatedSquares[turnOf], *destination);

        if (!hasKingMoved[turnOf] && (abs(*destination - *origin)) == 2)
        {
            char castleSide = (*destination > *origin); // 0 for left, 1 for right

            setPiece(EMPTY, *destination + (castleSide ? 1 : -2));
            setPiece(ROOK | turnOf, *destination + (castleSide ? -1 : 1));

            hasRookMoved[(turnOf * 2) + castleSide] = 1;
            hasKingMoved[turnOf] = 1;

            return 1;
        }

        hasKingMoved[turnOf] = 1;
    }
    return 0;
}

void handleRookMovement(char *origin, char *destination)
{
    if (*origin == 0 || *destination == 0)
        hasRookMoved[2] = 1;
    if (*origin == 7 || *destination == 7)
        hasRookMoved[3] = 1;
    if (*origin == 56 || *destination == 56)
        hasRookMoved[0] = 1;
    if (*origin == 63 || *destination == 63)
        hasRookMoved[1] = 1;
}

void handlePushedPawn(char *selectedPiece, char *selectedPosition, char *destinationPosition)
{
    if ((*selectedPiece & 0b1110) == PAWN && abs(*selectedPosition - *destinationPosition) == 16)
        lastPushedPawn = (*destinationPosition & 0b0111);
    else
        lastPushedPawn = -1;
}

void handlePromotion(char *selectedPiece, char *destinationPosition)
{
    if ((*selectedPiece & 0b1110) == PAWN && (*destinationPosition >> 3) == (turnOf ? 7 : 0))
    {
        printf("Promote to:\n");
        printf("1. Queen\n");
        printf("2. Rook\n");
        printf("3. Bishop\n");
        printf("4. Knight\n");

        char promotion;
        scanf("%c", &promotion);

        while (getchar() != '\n')
            ;

        char newPiece = QUEEN;
        if (promotion == '2')
            newPiece = ROOK;
        else if (promotion == '3')
            newPiece = BISHOP;
        else if (promotion == '4')
            newPiece = KNIGHT;

        newPiece |= turnOf;

        setPiece(newPiece, *destinationPosition);
    }
}

void endTurn()
{
    turnOf = (~turnOf) & 0x01;
    if (turnOf == 0)
        turnCount++;
}

void handleTakingPiece(char *destination, char *isEnPassant)
{
    char takenPiece = (*isEnPassant) ? getPieceAt(*destination + (turnOf ? 8 : -8)) : getPieceAt(*destination);

    if (takenPiece == EMPTY)
        return;

    if ((takenPiece & 0b1110) == BISHOP || (takenPiece & 0b1110) == KNIGHT)
    {
        pieceCount[turnOf ^ 0b01] -= 1;
        return;
    }

    pieceCount[turnOf ^ 0b01] -= 2;
}

char nextTurn()
{
    printf("Turn of Player %d\n", turnOf);

    unsigned char selectedPosition;
    char selectedPiece = selectPiece(&selectedPosition);

    MovesList moves = getPieceMovement(&kingsRelatedSquares[turnOf], selectedPiece, selectedPosition, 1);
    printBoard(&moves, &selectedPosition);
    free(moves.moves);

    printf("Select a destination:\n");

    unsigned char destinationPosition = askForPosition();

    char enPassant = handleEnPassant(&selectedPiece, &destinationPosition);
    handleTurnCountReset(&selectedPiece, &selectedPosition, &destinationPosition);
    handleTakingPiece(&destinationPosition, &enPassant);

    setPiece(EMPTY, selectedPosition);
    setPiece(selectedPiece, destinationPosition);

    char castle = handleKingMovement(&selectedPiece, &selectedPosition, &destinationPosition);
    if (!castle)
    {
        handleRookMovement(&selectedPosition, &destinationPosition);
        handlePushedPawn(&selectedPiece, &selectedPosition, &destinationPosition);
    }

    handlePromotion(&selectedPiece, &destinationPosition);

    printBoard(NULL, NULL);

    if (handleMovementRelationWithOpposingKing(&selectedPosition, &destinationPosition, &enPassant)) {
        return !(handleChecks() || handleDraw());
    }

    checked = 0;
    return !handleDraw();
}

int main()
{
    setupBasePosition();
    printBoard(NULL, NULL);

    while (nextTurn())
        endTurn();

    return 0;
}
