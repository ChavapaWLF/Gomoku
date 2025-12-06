#pragma once
#ifndef __CHECK_H
#define __CHECK_H

#include <stdio.h>
#include <string.h>
#include "chess.h"

int CheckLongChain(CHESSSTATE** board, int row, int col);
int CheckFourInRow(CHESSSTATE** board, int row, int col);
int CountFourPatterns(CHESSSTATE** board, char* dirState, int base_x, int base_y, int dx, int dy);
int CheckThreeInRow(CHESSSTATE** board, int row, int col);
int BanMove(CHESSSTATE** board, int row, int col);

#endif // __CHECK_H