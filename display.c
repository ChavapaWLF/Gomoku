/**
* @file     display.c
* @brief    显示棋盘文件
* @details  在终端显示棋盘信息
*/

#include "display.h"
#include <stdlib.h>

/**
 * @brief   动态分配数组的棋盘显示
 */
void draw(CHESSSTATE** state, char mode, char able1, char able2, int row, int column, int r, int c) {
	system("cls"); //清屏
	printf("Gomoku V1.0 by ChavapaWLF Copyright 2024\n");
	if (mode == 'A'){
		printf("当前模式：人人对战（有禁手）\n");
	}
	if (mode == 'B'){
		printf("当前模式：人机对战  机器棋力：");
		if (able1 == 'J' || able1 == 'j') printf("初级（Junior）\n");
		else if (able1 == 'M' || able1 == 'm') printf("中级（Medium）\n");
		else if (able1 == 'A' || able1 == 'a') printf("高级（Advanced）\n");
		else if (able1 == 'S' || able1 == 's') printf("超级（Super）\n");
	}
	if (mode == 'C'){
		printf("当前模式：机器自对弈  黑方棋力：");
		if (able1 == 'J' || able1 == 'j') printf("初级（Junior）");
		else if (able1 == 'M' || able1 == 'm') printf("中级（Medium）");
		else if (able1 == 'A' || able1 == 'a') printf("高级（Advanced）");
		else if (able1 == 'S' || able1 == 's') printf("超级（Super）");
		printf(" 白方棋力：");
		if (able2 == 'J' || able2 == 'j') printf("初级（Junior）\n");
		else if (able2 == 'M' || able2 == 'm') printf("中级（Medium）\n");
		else if (able2 == 'A' || able2 == 'a') printf("高级（Advanced）\n");
		else if (able2 == 'S' || able2 == 's') printf("超级（Super）\n");
	}
	int i, j;

	for (i = 0; i < row; i++) {
		printf("%2d", SIZE - i);  //画行序号
		for (j = 0; j < column; j++) {
			switch (state[SIZE - i - 1][j]) {
			case 0:               //画棋盘
				if (j > 0 && j < column - 1)
					printf("%s", (i == 0 ? TOP_CENTER : i == row - 1 ? BOTTOM_CENTER : INTERNAL));
				else if (j == 0)
					printf("%s", (i == 0 ? LEFT_TOP : i == row - 1 ? LEFT_BOTTOM : LEFT_CENTER));
				else if (j == column - 1)
					printf("%s", (i == 0 ? RIGHT_TOP : i == row - 1 ? RIGHT_BOTTOM : RIGHT_CENTER));
				break;
			case 1:               //画黑棋
				if (SIZE - i - 1 == r && j == c) printf("%s", j == column-1 ? LASTBLACK: LASTBLACK_1);
				else printf("%s", j == column-1 ? BLACK: BLACK_1);
				break;
			case 2:               //画白棋
				if (SIZE - i - 1 == r && j == c) printf("%s", j == column-1 ? LASTWHITE: LASTWHITE_1);
				else printf("%s", j == column-1 ? WHITE: WHITE_1);
				break;
			default:
				break;
			}
		}
		printf("\n");
	}
	printf("  ");
	for (i = 0; i < column; i++)      //画列序号
		printf("%c ", (char)(i + 65));
	printf("\n");
}