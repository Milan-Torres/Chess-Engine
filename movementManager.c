#include "movementManager.h"

#include <unistd.h>
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>

#define BLACKS_FORWARD 2 * 4

extern char board[32];
extern char checked;
extern char lastPushedPawn;
extern char hasKingMoved[2];
extern char hasRookMoved[4];

char checkSlidingPieceMovement(char direction, char color, char *checkedPosition)
{
    // return codes: 0b00 - movement allowed, 0b10 - block next movement but allow this one, 0b11 - movement blocked
    char checkedPiece = getPieceAt(*checkedPosition);

    if (((checkedPiece & 0x0E) != EMPTY) && (color == ((checkedPiece & 0x01))))
    {
        return 0b11;
    }

    if (isASide(*checkedPosition, direction))
    {
        return 0b10;
    }

    if ((checkedPiece & 0x0E) == EMPTY)
    {
        return 0b00;
    }

    return 0b10;
}

char knightWentThroughtTheBoard(char *previousPosition, char *currentPosition)
{
    if (*currentPosition < 0)
    {
        return 1;
    }
    if ((((*previousPosition & 0b00111000) >> 3) < 2) && (((*currentPosition & 0b00111000) >> 3) >= 6))
    {
        return 1;
    }
    if ((((*previousPosition & 0b00111000) >> 3) >= 6) && (((*currentPosition & 0b00111000) >> 3) < 2))
    {
        return 1;
    }
    if (((*previousPosition & 0b00000111) < 2) && ((*currentPosition & 0b00000111) >= 6))
    {
        return 1;
    }
    if (((*previousPosition & 0b00000111) >= 6) && ((*currentPosition & 0b00000111) < 2))
    {
        return 1;
    }

    return 0;
}

char isSquareVulnerable(unsigned char position, char color)
{
    // BISHOP CHECKS
    for (char index = 0; index < 4; index++)
    {
        if (isASide(position, index | 0b100))
            continue;

        for (char i = 1; i < 8; i++)
        {
            char checkedPosition;
            if (index & 0b10)
                checkedPosition = (position + i) + (((index - 2) * 2) - 1) * (i * 8);
            else
                checkedPosition = (position - i) + ((index * 2) - 1) * (i * 8);

            char returnCode = checkSlidingPieceMovement(index | 0b100, (color ^ 0b01), &checkedPosition);
            if (returnCode ^ 0b011)
            {
                if ((getPieceAt(checkedPosition) & 0b01011) == (0b01010 | color))
                    return 1;
            }

            if (returnCode & 0b10)
                break;
        }
    }

    // ROOK CHECKS
    for (char index = 0; index < 4; index++)
    {
        if (isASide(position, index))
            continue;

        for (char i = 1; i < 8; i++)
        {
            {
                char checkedPosition;
                if (index < 2)
                    checkedPosition = position + ((index * 2) - 1) * i;
                else
                    checkedPosition = position + (((index - 2) * 2) - 1) * (i * 8);

                char returnCode = checkSlidingPieceMovement(index, (color ^ 0b01), &checkedPosition);

                if (returnCode ^ 0b011)
                {
                    if ((getPieceAt(checkedPosition) & 0b01101) == (0b01100 | color))
                        return 1;
                }

                if (returnCode & 0b10)
                    break;
            }
        }
    }

    // KNIGHT CHECKS
    for (char direction = 0; direction < 4; direction++)
    {
        for (char c = 0; c < 2; c++)
        {
            char possibleJump = position + 8 * (1 - (2 * ((int)floor(direction / 2)))) * (c % 2 + 1) - (2 - c) * (2 * (direction % 2) - 1);
            if (knightWentThroughtTheBoard(&position, &possibleJump))
                continue;

            char pieceAtJump = getPieceAt(possibleJump);
            if (pieceAtJump == (KNIGHT | (color)))
                return 1;
        }
    }

    // KING CHECKS
    for (char direction = 0; direction < 8; direction++)
    {

        if (isASide(position, direction))
            continue;

        if (direction & 0b100)
        {
            if (getPieceAt((((direction & 0b001) * 2) - 1) * 8 + ((((direction >> 1) & 0x01) * 2) - 1)) == (KING | (color)))
                return 1;
        }
        else
        {
            if (getPieceAt((((direction & 0b001) * 2) - 1) * (1 + (7 * ((direction >> 1) & 0x01)))) == (KING | color))
                return 1;
        }
    }

    // PAWN CHECKS
    char forward = ((color * 2) - 1) * -BLACKS_FORWARD;
    unsigned char firstSquareForward = position + forward;

    char firstRightDiagonal = firstSquareForward + 1;

    if (!isASide(position, 0b01) && (getPieceAt(firstRightDiagonal)) == (PAWN | color))
        return 1;

    char firstLeftDiagonal = firstSquareForward - 1;
    if (!isASide(position, 0b0) && (getPieceAt(firstLeftDiagonal)) == (PAWN | color))
        return 1;

    return 0;
}

void setKingPosition(KingRelatedSquares *kingRelatedSquares, char position)
{
    kingRelatedSquares->kingPosition = position;

    for (char i = 0; i < 8; i++)
    {
        updateKingRelatedPosition(kingRelatedSquares, i);
    }
}

void updateKingRelatedPosition(KingRelatedSquares *kingRelatedSquares, char index)
{
    char linePosition = (kingRelatedSquares->kingPosition & 0b00111000);
    char columnPosition = (kingRelatedSquares->kingPosition & 0b00000111);
    if (index == 0)
    {
        for (char column = columnPosition - 1; column >= 1; column--)
        {
            if (column < 1)
                break;

            if (getPieceAt(linePosition | column) != EMPTY)
            {
                store3BitsValue(kingRelatedSquares->linesRelated, 0, column);
                return;
            }
        }
        store3BitsValue(kingRelatedSquares->linesRelated, 0, 0);
        return;
    }
    if (index == 1)
    {
        for (char column = columnPosition + 1; column < 7; column++)
        {
            if (column >= 7)
                break;

            if (getPieceAt(linePosition | column) != EMPTY)
            {
                store3BitsValue(kingRelatedSquares->linesRelated, 1, column);
                return;
            }
        }
        store3BitsValue(kingRelatedSquares->linesRelated, 1, 7);
        return;
    }
    if (index == 2)
    {
        for (char lines = linePosition - 8; lines >= 8; lines -= 8)
        {
            if (lines < 8)
                break;

            if (getPieceAt(lines | columnPosition) != EMPTY)
            {
                store3BitsValue(kingRelatedSquares->linesRelated, 2, (lines >> 3));
                return;
            }
        }
        store3BitsValue(kingRelatedSquares->linesRelated, 2, 0);
        return;
    }
    if (index == 3)
    {
        for (char lines = linePosition + 8; lines < 56; lines += 8)
        {
            if (lines >= 56)
                break;
            if (getPieceAt(lines | columnPosition) != EMPTY)
            {
                store3BitsValue(kingRelatedSquares->linesRelated, 3, (lines >> 3));
                return;
            }
        }
        store3BitsValue(kingRelatedSquares->linesRelated, 3, 7);
        return;
    }
    if (index == 4)
    {
        for (char i = (linePosition >> 3) - 1; i >= 1; i--)
        {
            if (i < 1)
                break;

            if (getPieceAt((i << 3) | (columnPosition - i)) != EMPTY)
            {
                store3BitsValue(kingRelatedSquares->linesRelated, 4, i);
                return;
            }
        }
        store3BitsValue(kingRelatedSquares->linesRelated, 4, 0);
        return;
    }
    if (index == 5)
    {
        for (char i = (linePosition >> 3) + 1; i < 7; i++)
        {
            if (i >= 7)
                break;

            if (getPieceAt((i << 3) | (columnPosition + i)) != EMPTY)
            {
                store3BitsValue(kingRelatedSquares->linesRelated, 5, i);
                return;
            }
        }
        store3BitsValue(kingRelatedSquares->linesRelated, 5, 7);
        return;
    }
    if (index == 6)
    {
        for (char i = (linePosition >> 3) - 1; i >= 1; i--)
        {
            if (i < 1)
                break;

            if (getPieceAt((i << 3) | (columnPosition + i)) != EMPTY)
            {
                store3BitsValue(kingRelatedSquares->linesRelated, 6, i);
                return;
            }
        }
        store3BitsValue(kingRelatedSquares->linesRelated, 6, 0);
        return;
    }
    if (index == 7)
    {
        for (char i = (linePosition >> 3) + 1; i < 7; i++)
        {
            if (i >= 7)
                break;

            if (getPieceAt((i << 3) | (columnPosition - i)) != EMPTY)
            {
                store3BitsValue(kingRelatedSquares->linesRelated, 7, i);
                return;
            }
        }
        store3BitsValue(kingRelatedSquares->linesRelated, 7, 7);
        return;
    }
}

char isSquareRelatedToKing(KingRelatedSquares *kingRelatedSquares, char position)
{
    // returns 0 (false) if the square is unrelated, returns the index of the related line + 1 otherwise

    char linePosition = ((position & 0b00111000) >> 3);
    char columnPosition = (position & 0b00000111);

    char kingLinePosition = ((kingRelatedSquares->kingPosition & 0b00111000) >> 3);
    char kingColumnPosition = (kingRelatedSquares->kingPosition & 0b0111);

    if (linePosition == kingLinePosition)
    {
        if (columnPosition >= retrieve3BitsValue(kingRelatedSquares->linesRelated, 0) && columnPosition <= retrieve3BitsValue(kingRelatedSquares->linesRelated, 1))
        {
            if (columnPosition < kingColumnPosition)
                return 0 + 1;
            return 1 + 1;
        }
        return 0;
    }

    if (columnPosition == kingColumnPosition)
    {
        if (linePosition >= retrieve3BitsValue(kingRelatedSquares->linesRelated, 2) && linePosition <= retrieve3BitsValue(kingRelatedSquares->linesRelated, 3))
        {
            if (linePosition < kingLinePosition)
                return 2 + 1;
            return 3 + 1;
        }
        return 0;
    }

    if ((linePosition - columnPosition) == (kingLinePosition - kingColumnPosition))
    {
        if (linePosition >= retrieve3BitsValue(kingRelatedSquares->linesRelated, 4) && linePosition <= retrieve3BitsValue(kingRelatedSquares->linesRelated, 5))
        {
            if (linePosition < kingLinePosition)
                return 4 + 1;
            return 5 + 1;
        }
        return 0;
    }

    if ((columnPosition + linePosition) == (kingColumnPosition + kingLinePosition))
    {
        if (linePosition >= retrieve3BitsValue(kingRelatedSquares->linesRelated, 6) && linePosition <= retrieve3BitsValue(kingRelatedSquares->linesRelated, 7))
        {
            if (linePosition > kingLinePosition)
                return 6 + 1;
            return 7 + 1;
        }
        return 0;
    }

    return 0;
}

void tryToAddPosition(KingRelatedSquares *kingRelatedSquares, MovesList *moves, char origininIsRelatedToKing, char origin, char destination, char pieceAtOrigin, char pieceAtDestination, char *checkVulnerability)
{
    if (checked)
    {
        if (!isSquareRelatedToKing(kingRelatedSquares, destination))
            return;
    }

    if ((checked || origininIsRelatedToKing || isSquareRelatedToKing(kingRelatedSquares, destination)) && *checkVulnerability)
    {
        setPiece(getPieceAt(origin), destination);
        setPiece(EMPTY, origin);

        char isAnEnPassant = lastPushedPawn != -1 && (pieceAtOrigin & 0b1110) == PAWN && (pieceAtDestination & 0b1110) == EMPTY && (destination - origin) % 8 != 0;

        if (isAnEnPassant)
            setPiece(EMPTY, destination + (8 * (((pieceAtOrigin & 0x01) * 2) - 1)));

        if (!isSquareVulnerable(kingRelatedSquares->kingPosition, (pieceAtOrigin & 0x01) ^ 0b01))
        {
            store6BitsValue(moves->moves, moves->size, destination);
            moves->size++;
        }

        if (isAnEnPassant)
            setPiece(PAWN, destination + (8 * (((pieceAtOrigin & 0x01) * 2) - 1)));

        setPiece(pieceAtOrigin, origin);
        setPiece(pieceAtDestination, destination);

        return;
    }

    store6BitsValue(moves->moves, moves->size, destination);
    moves->size++;
}

char isASide(unsigned char position, char side)
{
    // side: 0b000 - left, 0b001 - right, 0b010 - top, 0b011 - bottom, 0b100 - top left, 0b101 - bottom left, 0b110 - top right, 0b111 - bottom right
    if (side & 0b0100)
        return isASide(position, (side & 0x02) >> 1) || isASide(position, (side & 0x01) | 0x02);
    if (side & 0x02)
        return ((position & 0b00111000) >> 3) == (side & 0x01) * 7;
    return (position & 0b00000111) == (side * 7);
}

void tryToAddPositionIfEmpty(KingRelatedSquares *kingRelatedSquares, MovesList *moves, char origininIsRelatedToKing, char origin, char destination, char pieceAtOrigin, char sideToCheck, char *checkVulnerability)
{
    if (isASide(origin, sideToCheck))
    {
        return;
    }

    char pieceAtDestination = getPieceAt(destination);
    if (pieceAtDestination == EMPTY)
    {
        tryToAddPosition(kingRelatedSquares, moves, origininIsRelatedToKing, origin, destination, pieceAtOrigin, pieceAtDestination, checkVulnerability);
    }
}

void tryToAddPositionIfEnemy(KingRelatedSquares *kingRelatedSquares, MovesList *moves, char origininIsRelatedToKing, char origin, char destination, char pieceAtOrigin, char sideToCheck, char *checkVulnerability)
{
    if (isASide(origin, sideToCheck))
    {
        return;
    }

    char pieceAtDestination = getPieceAt(destination);
    if (pieceAtDestination != EMPTY && ((pieceAtDestination & 0x01) == ((pieceAtOrigin ^ 0x01) & 0x01)))
    {
        tryToAddPosition(kingRelatedSquares, moves, origininIsRelatedToKing, origin, destination, pieceAtOrigin, pieceAtDestination, checkVulnerability);
    }
}

void tryToAddPositionIfEnemyOrEmpty(KingRelatedSquares *kingRelatedSquares, MovesList *moves, char origininIsRelatedToKing, char origin, char destination, char pieceAtOrigin, char sideToCheck, char *checkVulnerability)
{
    if (sideToCheck != -1 && isASide(origin, sideToCheck))
    {
        return;
    }

    char pieceAtDestination = getPieceAt(destination);
    if (pieceAtDestination == EMPTY || ((pieceAtDestination & 0x01) == ((pieceAtOrigin ^ 0x01) & 0x01)))
    {
        tryToAddPosition(kingRelatedSquares, moves, origininIsRelatedToKing, origin, destination, pieceAtOrigin, pieceAtDestination, checkVulnerability);
    }
}

char canCastle(char *piece, char *position, char side)
{
    // side: 0 left, 1 right

    if (isSquareVulnerable(*position, (*piece & 0x01) ^ 0b01))
        return 0;

    if (hasKingMoved[(*piece & 0x01)] || hasRookMoved[((*piece & 0x01) * 2) + side])
        return 0;

    if (side == 0)
    {
        for (char i = 1; i < 4; i++)
        {
            if (getPieceAt(*position - i) != EMPTY || isSquareVulnerable(*position - i, (*piece & 0x01) ^ 0b01))
                return 0;
        }

        return 1;
    }

    for (char i = 1; i < 3; i++)
    {
        if (getPieceAt(*position + i) != EMPTY || isSquareVulnerable(*position + i, (*piece & 0x01) ^ 0b01))
            return 0;
    }
}

MovesList getPieceMovement(KingRelatedSquares *kingRelatedSquares, char piece, unsigned char position, char checkVulnerability)
{
    char isKingConcerned = isSquareRelatedToKing(kingRelatedSquares, position);

    MovesList moveList;
    moveList.moves = allocateBytesForPositions(4);
    moveList.size = 0;

    if ((piece & 0b1000) == 0b1000)
    {
        if ((piece & 0x0E) == PAWN)
        {
            char forward = (((piece & 0x01) * 2) - 1) * BLACKS_FORWARD;
            unsigned char firstSquareForward = position + forward;

            tryToAddPositionIfEmpty(kingRelatedSquares, &moveList, isKingConcerned, position, firstSquareForward, piece, ((piece & 0x01) | 0b10), &checkVulnerability);
            tryToAddPositionIfEnemy(kingRelatedSquares, &moveList, isKingConcerned, position, firstSquareForward - 1, piece, 0b00, &checkVulnerability);
            tryToAddPositionIfEnemy(kingRelatedSquares, &moveList, isKingConcerned, position, firstSquareForward + 1, piece, 0b01, &checkVulnerability);

            if (((position & 0b111000) >> 3) == (3 + ((piece & 0x01))))
            {
                if (lastPushedPawn != -1)
                {
                    if ((lastPushedPawn - 1) == (position & 0b111) || (lastPushedPawn + 1) == (position & 0b111))
                        tryToAddPosition(kingRelatedSquares, &moveList, isKingConcerned, position, ((2 + (((piece & 0x01) * 3))) << 3) | lastPushedPawn, piece, EMPTY, &checkVulnerability);
                }
            }

            if (((position & 0b00111000) >> 3) == 1 + ((piece & 0x01) ^ 0x01) * 5)
            {
                tryToAddPositionIfEmpty(kingRelatedSquares, &moveList, isKingConcerned, position, position + (forward * 2), piece, ((piece & 0x01) | 0b10), &checkVulnerability);
            }

            return moveList;
        }

        unsigned char maxMovesLength = 0;

        // has bishop bit
        if ((piece & 0b10) == 0b10)
        {
            moveList.moves = allocateBytesForPositions(13);
            maxMovesLength = 13;

            for (char diagonal = 0; diagonal < 4; diagonal++)
            {
                if (isASide(position, diagonal | 0b100))
                    continue;

                for (char i = 1; i < 8; i++)
                {
                    char checkedPosition;
                    if (diagonal & 0b10)
                        checkedPosition = (position + i) + (((diagonal - 2) * 2) - 1) * (i * 8);
                    else
                        checkedPosition = (position - i) + ((diagonal * 2) - 1) * (i * 8);

                    char returnCode = checkSlidingPieceMovement((diagonal | 0b100), (piece & 0x01), &checkedPosition);
                    if (returnCode ^ 0b11)
                        tryToAddPosition(kingRelatedSquares, &moveList, isKingConcerned, position, checkedPosition, piece, getPieceAt(checkedPosition), &checkVulnerability);

                    if (returnCode & 0b10)
                        break;
                }
            }
        }

        // has rook bit
        if ((piece & 0b100) == 0b100)
        {
            moveList.moves = reAllocateBytesForPositions(moveList.moves, maxMovesLength, 14);

            for (char direction = 0; direction < 4; direction++)
            {
                if (isASide(position, direction))
                    continue;

                for (char i = 1; i < 8; i++)
                {
                    {
                        char checkedPosition;
                        if (direction & 0b10)
                            checkedPosition = position + (((direction - 2) * 2) - 1) * (i * 8);
                        else
                            checkedPosition = position + ((direction * 2) - 1) * i;

                        char returnCode = checkSlidingPieceMovement(direction, piece & 0x01, &checkedPosition);

                        if (returnCode ^ 0b11)
                            tryToAddPosition(kingRelatedSquares, &moveList, isKingConcerned, position, checkedPosition, piece, getPieceAt(checkedPosition), &checkVulnerability);

                        if (returnCode & 0b10)
                            break;
                    }
                }
            }
            return moveList;
        }
    }

    else
    {
        if ((piece & 0b1110) == KNIGHT)
        {
            moveList.moves = allocateBytesForPositions(8);

            for (char direction = 0; direction < 4; direction++)
            {
                for (char c = 0; c < 2; c++)
                {
                    char possibleJump = position + 8 * (1 - (2 * ((int)floor(direction / 2)))) * (c % 2 + 1) - (2 - c) * (2 * (direction % 2) - 1);

                    if (knightWentThroughtTheBoard(&position, &possibleJump))
                    {
                        continue;
                    }
                    tryToAddPositionIfEnemyOrEmpty(kingRelatedSquares, &moveList, isKingConcerned, position, possibleJump, piece, -1, &checkVulnerability);
                }
            }

            return moveList;
        }

        if ((piece & 0b1110) == KING)
        {
            moveList.moves = allocateBytesForPositions(8);

            for (char direction = 0; direction < 8; direction++)
            {
                if (isASide(position, direction))
                    continue;

                char possibleMove;
                if (direction & 0b100)
                    possibleMove = position + (((direction & 0b001) * 2) - 1) * 8 + ((((direction >> 1) & 0x01) * 2) - 1);
                else
                    possibleMove = position + (((direction & 0b001) * 2) - 1) * (1 + (7 * ((direction >> 1) & 0x01)));

                char pieceAtDestination = getPieceAt(possibleMove);

                if (pieceAtDestination != EMPTY && ((pieceAtDestination & 0x01) == (piece & 0x01)))
                    continue;

                if (checkVulnerability && isSquareVulnerable(possibleMove, (piece & 0x01) ^ 0b01))
                    continue;

                store6BitsValue(moveList.moves, moveList.size, possibleMove);
                moveList.size++;
            }

            if (canCastle(&piece, &position, 0))
            {
                store6BitsValue(moveList.moves, moveList.size, position - 2);
                moveList.size++;
            }

            if (canCastle(&piece, &position, 1))
            {
                store6BitsValue(moveList.moves, moveList.size, position + 2);
                moveList.size++;
            }

            return moveList;
        }
    }

    return moveList;
}
