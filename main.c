/*
特别提醒：1.本工程在windows环境下使用openmp指令集进行多线程优化，编译时请添加“-fopenmp”指令。
2.此外，请务必在编译时开启O3优化以最大化运算效率。

注：1.本工程已在Windows11环境下通过编译，如果在Linux环境下编译出现问题，请尝试删除所有与清屏和控制台设置相关的“system”函数。
2.“超级(Super)”模式并未对机器思考时间进行限制，仅供娱乐，并不作为五子棋比赛的参赛棋力。
*/

/**
 * @file     main.c
 * @brief    项目主函数文件
 * @details  主要包含协议应用栈程序框架，main函数入口
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "chess.h"
#include "display.h"
#include "check.h"
#include "machine.h"

// --------------------------------------------------
// 全局变量（将会在machine.c中被引用）
// --------------------------------------------------
clock_t start_time;  /**< 机器思考时的开始时间 */
int count = 0;       /**< 记录已落子数 */

// --------------------------------------------------
// 局部全局变量（仅在本文件内可见）
// --------------------------------------------------
static char mode;    /**< 当前对弈模式 */

// --------------------------------------------------
// 函数声明（供本文件使用）
// --------------------------------------------------
void showMenu(void);
void humanMode(CHESSSTATE **board, char *rowPtr, char *colPtr);
void machineMode(CHESSSTATE **board, char *rowPtr, char *colPtr, char modeOption);
void autoPlayAI(CHESSSTATE **board, char *rowPtr, char *colPtr);

void playerMove(CHESSSTATE **board, char *rowPtr, char *colPtr, CHESSSTATE currentPlayer);
CHESSSTATE checkResult(CHESSSTATE **board, char row, char col, CHESSSTATE currentPlayer);

// --------------------------------------------------
// 函数定义 - 辅助函数
// --------------------------------------------------

/**
 * @brief 玩家落子函数
 * @param[in]  board         棋盘(二维指针)
 * @param[out] *rowPtr       指向行坐标的指针
 * @param[out] *colPtr       指向列坐标的指针
 * @param[in]  currentPlayer 当前下棋方（black或white）
 */
void playerMove(CHESSSTATE **board, char *rowPtr, char *colPtr, CHESSSTATE currentPlayer)
{
    char inputStr[5] = {'\0'};
    printf("请【%s】玩家落子 (如: H8)：", (currentPlayer == black ? "黑方" : "白方"));

    while (1)
    {
        scanf("%s", inputStr);  
        *colPtr = inputStr[0] - 'A';
        *rowPtr = (char)(atoi(inputStr + 1) - 1);

        // 合法性判断
        if (*rowPtr >= 0 && *rowPtr <= ROW - 1 && *colPtr >= 0 && *colPtr <= COLUMN - 1)
        {
            if (board[*rowPtr][*colPtr])
            {
                printf("该位置已经有棋子了，请下别的位置：");
                continue;
            }
            else
            {
                board[*rowPtr][*colPtr] = currentPlayer;
                break;
            }
        }
        else
        {
            printf("非法输入，请重新输入：");
            continue;
        }
    }
}

/**
 * @brief 检查输赢结果函数
 * @param[in] board         棋盘(二维指针)
 * @param[in] row           刚落子的行坐标
 * @param[in] col           刚落子的列坐标
 * @param[in] currentPlayer 当前下棋方（black或white）
 * @retval none  无结果
 * @retval black 黑方赢
 * @retval white 白方赢
 */
CHESSSTATE checkResult(CHESSSTATE **board, char row, char col, CHESSSTATE currentPlayer)
{
    int i = 0, flag = 0;
    int checkRow = 0, checkCol = 0;

    // ----------- 横向检测 '-----' -----------
    checkRow = row;
    checkCol = col - 4;  
    flag = 0;
    for (i = 0; i < 9; i++, checkCol++)
    {
        if (checkCol < 0 || checkCol >= COLUMN)
            continue;

        if (board[checkRow][checkCol] == currentPlayer)
        {
            flag++;
        }
        else if (flag >= 5)
        {
            break;
        }
        else
        {
            flag = 0;
        }
    }
    if ((currentPlayer == white && flag >= 5) ||
        (currentPlayer == black && flag == 5))
    {
        return currentPlayer;
    }

    // ----------- 纵向检测 '|' -----------
    checkRow = row - 4;
    checkCol = col;
    flag = 0;
    for (i = 0; i < 9; i++, checkRow++)
    {
        if (checkRow < 0 || checkRow >= ROW)
            continue;

        if (board[checkRow][checkCol] == currentPlayer)
        {
            flag++;
        }
        else if (flag >= 5)
        {
            break;
        }
        else
        {
            flag = 0;
        }
    }
    if ((currentPlayer == white && flag >= 5) ||
        (currentPlayer == black && flag == 5))
    {
        return currentPlayer;
    }

    // ----------- '/' 方向检测 -----------
    checkRow = row - 4;
    checkCol = col + 4;
    flag = 0;
    for (i = 0; i < 9; i++, checkRow++, checkCol--)
    {
        if (checkRow < 0 || checkRow >= ROW || checkCol < 0 || checkCol >= COLUMN)
            continue;

        if (board[checkRow][checkCol] == currentPlayer)
        {
            flag++;
        }
        else if (flag >= 5)
        {
            break;
        }
        else
        {
            flag = 0;
        }
    }
    if ((currentPlayer == white && flag >= 5) ||
        (currentPlayer == black && flag == 5))
    {
        return currentPlayer;
    }

    // ----------- '\' 方向检测 -----------
    checkRow = row - 4;
    checkCol = col - 4;
    flag = 0;
    for (i = 0; i < 9; i++, checkRow++, checkCol++)
    {
        if (checkRow < 0 || checkRow >= ROW || checkCol < 0 || checkCol >= COLUMN)
            continue;

        if (board[checkRow][checkCol] == currentPlayer)
        {
            flag++;
        }
        else if (flag >= 5)
        {
            break;
        }
        else
        {
            flag = 0;
        }
    }
    if ((currentPlayer == white && flag >= 5) ||
        (currentPlayer == black && flag == 5))
    {
        return currentPlayer;
    }

    // 无结果则返回 none
    return none;
}

// --------------------------------------------------
// 函数定义 - 主要流程函数
// --------------------------------------------------

/**
 * @brief 显示开头提示和规则说明
 */
void showMenu(void)
{
    printf("Gomoku V1.0 by ChavapaWLF Copyright 2024\n");
    printf("1. 黑方为先手，白方为后手\n");
    printf("2. 黑方有三三禁手、四四禁手、长连禁手规则。若黑方落于禁手位置则判负\n");
    printf("   ·三三禁手：黑棋一子落下同时形成两个或以上活三\n");
    printf("   ·四四禁手：黑棋一子落下同时形成两个或以上冲四或活四\n");
    printf("   ·长连禁手：黑棋一子落下形成一个或以上长连\n");
    printf("   (注：五连的优先级大于禁手规则；三三禁手不包含假活三)\n");
    printf("\n");
    printf("请选择模式 (输入字母)：\n");
    printf("A: 人人对战   B: 人机对战   C: 机器自对弈\n");
}

/**
 * @brief 人人对战模式
 * @param[in]  **board 二维棋盘指针
 * @param[in]  *rowPtr 行坐标指针
 * @param[in]  *colPtr 列坐标指针
 */
void humanMode(CHESSSTATE **board, char *rowPtr, char *colPtr)
{
    CHESSSTATE result, currentPlayer = black;
    int banCheck = 0;

    draw(board, mode, ' ', ' ', SIZE, SIZE, -1, -1); // 初次绘制空棋盘

    while (1)
    {
        playerMove(board, rowPtr, colPtr, currentPlayer);
        draw(board, mode, ' ', ' ', SIZE, SIZE, *rowPtr, *colPtr);

        result = checkResult(board, *rowPtr, *colPtr, currentPlayer);
        if (result == black)
        {
            printf("游戏结束，黑方获胜！\n");
            break;
        }
        else if (result == white)
        {
            printf("游戏结束，白方获胜！\n");
            break;
        }

        banCheck = BanMove(board, *rowPtr, *colPtr);
        if (banCheck == 3)
        {
            printf("（%c, %d）位置触发【长连禁手】，白方获胜！\n", 'A' + *colPtr, 1 + *rowPtr);
            break;
        }
        else if (banCheck == 1)
        {
            printf("（%c, %d）位置触发【三三禁手】，白方获胜！\n", 'A' + *colPtr, 1 + *rowPtr);
            break;
        }
        else if (banCheck == 2)
        {
            printf("（%c, %d）位置触发【四四禁手】，白方获胜！\n", 'A' + *colPtr, 1 + *rowPtr);
            break;
        }

        count++;
        if (count == SIZE * SIZE)
        {
            printf("游戏结束，平局！\n");
            break;
        }

        // 更换下棋方
        currentPlayer = (currentPlayer == black ? white : black);
    }
}

/**
 * @brief 人机对战模式
 * @param[in]  **board    二维棋盘指针
 * @param[in]  *rowPtr    行坐标指针
 * @param[in]  *colPtr    列坐标指针
 * @param[in]  modeOption 选择人机先后手 (A/B)
 */
void machineMode(CHESSSTATE **board, char *rowPtr, char *colPtr, char modeOption)
{
    CHESSSTATE result, currentPlayer, machineColor;
    char engineLevel;
    MovePoint finalMove;
    int banCheck = 0;
    int value = 0;

    printf("\n机器算力一览：\n");
    printf("初级(Junior)：搜索深度 %d，固定选点宽度 %d，思考快\n", DEPTH_EASY, BASE_WIDTH);
    printf("中级(Medium)：搜索深度 %d，固定选点宽度 %d，思考时间约束 15s\n", DEPTH_MEDIUM, BASE_WIDTH);
    printf("高级(Advanced)：搜索深度 %d，选点宽度(16->15->15->...)，思考时间约束 15s\n", DEPTH_HARD);
    printf("超级(Super)：搜索深度 %d，选点宽度(16->15->15->15->15->10)，思考极慢，无时间约束\n", DEPTH_SUPER);
    printf("\n");

    // 选择算力
    printf("请选择机器算力 (J:初级 M:中级 A:高级 S:超级)(J/M/A/S)：");
    fflush(stdin);
    scanf(" %c", &engineLevel);
    while (engineLevel != 'J' && engineLevel != 'j' &&
           engineLevel != 'M' && engineLevel != 'm' &&
           engineLevel != 'A' && engineLevel != 'a' &&
           engineLevel != 'S' && engineLevel != 's')
    {
        printf("输入错误，请重新输入 (J/M/A/S)：");
        scanf(" %c", &engineLevel);
    }

    // 根据选项确定先后手
    if (modeOption == 'A')
    {
        machineColor = black; 
        currentPlayer = black;

        // 机器先手第一步通常落在中心
        finalMove.row = SIZE / 2;
        finalMove.col = SIZE / 2;
        board[finalMove.row][finalMove.col] = machineColor;

        draw(board, mode, engineLevel, ' ', SIZE, SIZE, finalMove.row, finalMove.col);
        count++;
        currentPlayer = white;
    }
    else
    {
        machineColor = white; 
        currentPlayer = black;
        draw(board, mode, engineLevel, ' ', SIZE, SIZE, -1, -1);
    }

    start_time = clock();
    while (1)
    {
        if (currentPlayer != machineColor)
        {
            // 玩家落子
            if (count > 0)
            {
                // 输出机器上一手落子信息
                printf("\n第 %d 手：机器落子位置（%c, %d），", count,
                       (char)('A' + finalMove.col), finalMove.row + 1);

                // 输出思考用时和局面评价
                clock_t end_time = clock();
                printf("思考用时 %.2f s\n", (double)(end_time - start_time) / 1000);
                printf("当前局面评价(数字越大，对机器越有利)：%d", value);
                if (value == 200000)
                    printf(" :-)");
                else if (value == -200000)
                    printf(" :-(");
                printf("\n第 %d 手：", count + 1);
            }

            playerMove(board, rowPtr, colPtr, currentPlayer);
        }
        else
        {
            printf("\n        机器正在思考中……\n");
            start_time = clock();

            // 机器落子
            value = AI_Play(board, machineColor, engineLevel, &finalMove);
            *rowPtr = finalMove.row;
            *colPtr = finalMove.col;
        }

        draw(board, mode, engineLevel, ' ', SIZE, SIZE, *rowPtr, *colPtr);

        // 判断胜负
        result = checkResult(board, *rowPtr, *colPtr, currentPlayer);
        if (result == black)
        {
            printf("游戏结束，黑方获胜！\n");
            break;
        }
        else if (result == white)
        {
            printf("游戏结束，白方获胜！\n");
            break;
        }

        // 检查禁手
        banCheck = BanMove(board, *rowPtr, *colPtr);
        if (banCheck == 3)
        {
            printf("（%c, %d）位置触发【长连禁手】，白方获胜！\n", 'A' + *colPtr, 1 + *rowPtr);
            break;
        }
        else if (banCheck == 1)
        {
            printf("（%c, %d）位置触发【三三禁手】，白方获胜！\n", 'A' + *colPtr, 1 + *rowPtr);
            break;
        }
        else if (banCheck == 2)
        {
            printf("（%c, %d）位置触发【四四禁手】，白方获胜！\n", 'A' + *colPtr, 1 + *rowPtr);
            break;
        }

        count++;
        if (count == SIZE * SIZE)
        {
            printf("游戏结束，平局！\n");
            break;
        }

        currentPlayer = (currentPlayer == black ? white : black);
    }
}

/**
 * @brief 机器自对弈模式
 * @param[in]  **board 二维棋盘指针
 * @param[in]  *rowPtr 行坐标指针
 * @param[in]  *colPtr 列坐标指针
 */
void autoPlayAI(CHESSSTATE **board, char *rowPtr, char *colPtr)
{
    char engineLevel1, engineLevel2;
    printf("选择机器1（黑方）算力(J:初级 M:中级 A:高级 S:超级)(J/M/A/S)：");
    fflush(stdin);
    scanf(" %c", &engineLevel1);
    while (engineLevel1 != 'J' && engineLevel1 != 'j' &&
           engineLevel1 != 'M' && engineLevel1 != 'm' &&
           engineLevel1 != 'A' && engineLevel1 != 'a' &&
           engineLevel1 != 'S' && engineLevel1 != 's')
    {
        printf("输入错误，请重新输入 (J/M/A/S)：");
        scanf(" %c", &engineLevel1);
    }

    printf("选择机器2（白方）算力(J:初级 M:中级 A:高级 S:超级) (J/M/A/S)：");
    fflush(stdin);
    scanf(" %c", &engineLevel2);
    while (engineLevel2 != 'J' && engineLevel2 != 'j' &&
           engineLevel2 != 'M' && engineLevel2 != 'm' &&
           engineLevel2 != 'A' && engineLevel2 != 'a' &&
           engineLevel2 != 'S' && engineLevel2 != 's')
    {
        printf("输入错误，请重新输入 (J/M/A/S)：");
        scanf(" %c", &engineLevel2);
    }

    // 黑棋先手，在中心处下第一手
    board[SIZE / 2][SIZE / 2] = black;
    CHESSSTATE result, currentPlayer = white;
    int banCheck = 0, value = 0;
    count++;

    draw(board, mode, engineLevel1, engineLevel2, SIZE, SIZE, SIZE / 2, SIZE / 2);
    printf("第 1 手：黑方落子位置（%c, %d）\n", 'A' + (SIZE / 2), (SIZE / 2) + 1);

    while (1)
    {
        MovePoint finalMove;
        printf(currentPlayer == black ? "黑方正在思考中……\n" : "白方正在思考中……\n");
        start_time = clock();

        if (currentPlayer == black)
            value = AI_Play(board, currentPlayer, engineLevel1, &finalMove);
        else
            value = AI_Play(board, currentPlayer, engineLevel2, &finalMove);

        *rowPtr = finalMove.row;
        *colPtr = finalMove.col;
        draw(board, mode, engineLevel1, engineLevel2, SIZE, SIZE, *rowPtr, *colPtr);

        count++;
        printf("第 %d 手", count);
        printf(currentPlayer == black ? " 黑方落子位置（%c, %d），" : " 白方落子位置（%c, %d），",
               'A' + finalMove.col, finalMove.row + 1);
        clock_t end_time = clock();
        printf("思考用时 %.2f s，局面评价：%d\n", (double)(end_time - start_time) / 1000, value);

        // 检查是否胜负已分
        result = checkResult(board, *rowPtr, *colPtr, currentPlayer);
        if (result == black)
        {
            printf("游戏结束，黑方获胜！\n");
            break;
        }
        else if (result == white)
        {
            printf("游戏结束，白方获胜！\n");
            break;
        }

        // 禁手检查
        banCheck = BanMove(board, *rowPtr, *colPtr);
        if (banCheck == 3)
        {
            printf("（%c, %d）位置触发【长连禁手】，白方获胜！\n",
                   'A' + finalMove.col, finalMove.row + 1);
            break;
        }
        else if (banCheck == 1)
        {
            printf("（%c, %d）位置触发【三三禁手】，白方获胜！\n",
                   'A' + finalMove.col, finalMove.row + 1);
            break;
        }
        else if (banCheck == 2)
        {
            printf("（%c, %d）位置触发【四四禁手】，白方获胜！\n",
                   'A' + finalMove.col, finalMove.row + 1);
            break;
        }

        if (count == SIZE * SIZE)
        {
            printf("游戏结束，平局！\n");
            break;
        }

        // 更换下棋方
        currentPlayer = (currentPlayer == black ? white : black);
    }
}

// --------------------------------------------------
// main函数 - 程序入口
// --------------------------------------------------
int main(void)
{
    system("color f0");  // 更换背景色和字体颜色

    // ----------------- 定义与初始化 -----------------
    CHESSSTATE **board;
    board = (CHESSSTATE **)malloc(sizeof(CHESSSTATE *) * ROW);
    for (int i = 0; i < ROW; i++)
        board[i] = (CHESSSTATE *)malloc(sizeof(CHESSSTATE) * COLUMN);

    for (int i = 0; i < ROW; i++)
        for (int j = 0; j < COLUMN; j++)
            board[i][j] = none;

    char rowChess = 0, colChess = 0;        // 用于记录落子的行、列
    char modeOption[5] = {'\0'};           // 用户输入的模式选项
    char *rowPtr = &rowChess, *colPtr = &colChess;

    // ----------------- 显示菜单并选择模式 -----------------
    showMenu();
    while (1)
    {
        fflush(stdin);
        scanf("%s", modeOption);
        if (modeOption[0] == 'A' || modeOption[0] == 'B' || modeOption[0] == 'C')
            break;
        printf("输入选项错误，请重新输入:\n");
    }
    mode = modeOption[0];

    // ----------------- 根据模式选择不同函数 -----------------
    if (mode == 'A')
    {
        humanMode(board, rowPtr, colPtr);
    }
    else if (mode == 'B')
    {
        printf("\n人机对战已选择，请选择【机器】下棋方:\n");
        printf("A: 先手黑棋(有禁手限制)   B: 后手白棋\n");

        while (1)
        {
            fflush(stdin);
            scanf("%s", modeOption);
            if (modeOption[0] == 'A' || modeOption[0] == 'B')
                break;
            printf("输入选项错误，请重新输入:\n");
        }
        machineMode(board, rowPtr, colPtr, modeOption[0]);
    }
    else
    {
        autoPlayAI(board, rowPtr, colPtr);
    }

    system("pause");

    // ----------------- 释放动态分配的内存 -----------------
    for (int i = 0; i < ROW; i++)
        free(board[i]);
    free(board);

    return 0;
}
