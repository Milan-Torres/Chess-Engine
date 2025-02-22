#include "UIManager.h"

#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>

#define EMPTY 0x00 //0000
#define KNIGHT 0x02 //0010
#define OOB 0x04 //0100
#define KING 0x06 //0110

#define PAWN 0x08 //1000
#define BISHOP 0x0A //1010
#define ROOK 0x0C //1100
#define QUEEN 0x0E //1110

extern char board[32];

void getPieceName(char* binary, char* name) {
    if ((*binary & 0x01) == 0x01) {
        *binary = *binary - 1;
        *(name + 1) = 'd'; //D
    } else {
        *(name + 1) = 'w'; //W
    }

    switch (*binary) {
        case EMPTY:
            *name = '*'; //*
            *(name + 1) = '*'; //*
            return;
        case PAWN:
            *name = 'p'; //p
            break;
        case KNIGHT:
            *name = 'k'; //k
            break;
        case BISHOP:
            *name = 'b'; //b
            break;
        case ROOK:
            *name = 'R'; //r
            break;
        case QUEEN:
            *name = 'Q'; //q
            break;
        case KING:
            *name = 'K'; //K
            break;
    }
}

void writeSelectionCharacter(unsigned char* possibleMoves, unsigned char possibleMovesLenght, unsigned char* selectedPiece, unsigned char position) {
    if (position == *selectedPiece) {
        write(1, ">", 1);
        return;
    }

    for (unsigned char c = 0; c < possibleMovesLenght; c++) {
        if (possibleMoves[c] == position) {
            write(1, "x", 1);
            return;
        }
    }

    write(1, " ", 1);
}

void printBoard(MovesList *moves, unsigned char* selectedPiece) {
    printf("Board:\n");
    if (selectedPiece == NULL) {
        for (unsigned char line = 0; line < 8; line++) {
            for (unsigned char column = 0; column < 4; column++) {
                unsigned char plot = board[(line * 4) + column];

                char firstPiece = plot >> 4;
                char secondPiece = plot & 0x0F;

                char name[2] = {};

                getPieceName(&firstPiece, &name[0]);
                write(1, &name, 2);
                write(1, ", ", 2);
                getPieceName(&secondPiece, &name[0]);
                write(1, &name, 2);
                write(1, ", ", 2);

            }
            write(1, "\n", 1);
        }
    } else {
        char boardPossibleMoves[moves->size];
        
        unsigned char c;
        for (c = 0; c < moves->size; c++) {
            boardPossibleMoves[c] = retrieve6BitsValue(moves->moves, c);
        }

        for (unsigned char line = 0; line < 8; line++) {
            for (unsigned char column = 0; column < 4; column++) {
                unsigned char plotIndex = (line * 4) + column;
                unsigned char plot = board[plotIndex];

                char firstPiece = plot >> 4;
                char secondPiece = plot & 0x0F;

                char name[2] = {};

                writeSelectionCharacter(boardPossibleMoves, moves->size, selectedPiece, (line * 8) + (column * 2));
                getPieceName(&firstPiece, &name[0]);
                write(1, &name, 2);
                write(1, ", ", 2);
                writeSelectionCharacter(boardPossibleMoves, moves->size, selectedPiece, (line * 8) + (column * 2) + 1);
                getPieceName(&secondPiece, &name[0]);
                write(1, &name, 2);
                write(1, ", ", 2);

            }
            write(1, "\n", 1);
        }
    }
}