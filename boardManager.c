#include "boardManager.h"

#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

extern char board[32];

char getPieceAt(char position)
{
    if (position > 63 || position < 0)
    {
        return OOB;
    }

    return (board[(int)floor(position / 2)] >> ((position % 2) - 1) * -4) & 0x0F;
}

void setPiece(char piece, unsigned char index)
{
    if (index % 2 == 0)
    {
        board[index / 2] &= 0x0F;
        board[index / 2] |= (piece & 0x0F) << 4;
    }
    else
    {
        board[(int)floor(index / 2)] &= 0xF0;
        board[(int)floor(index / 2)] |= (piece & 0x0F);
    }
}
