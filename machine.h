#ifndef MACHINE_H
#define MACHINE_H

#include "chess.h"  // 使用CHESSSTATE中的none, black, white
#include <stdbool.h> // 使用bool类型

#define SIZE 15

// 定义不同棋力的搜索深度
#define DEPTH_EASY 6
#define DEPTH_MEDIUM 8
#define DEPTH_HARD 10
#define DEPTH_SUPER 12

// 定义初级棋力下固定的选点宽度
#define BASE_WIDTH 15
// 定义高级棋力的最大选点宽度
#define MAX_WIDTH 16

//定义思考限时（毫秒）
#define TimeLimit 14850

// AI使用的结构
typedef struct {
    int row;
    int col;
} MovePoint;

// 函数声明
int AI_Play(CHESSSTATE** board, CHESSSTATE machine_color, char able, MovePoint* final_move);

#endif // MACHINE_H
