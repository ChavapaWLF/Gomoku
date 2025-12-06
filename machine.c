// Compile with: gcc -O3 -fopenmp -o machine machine.c

/**
 * @file     machine.c
 * @brief    机器落子策略
 * @details  机器通过Min-Max搜索实现最佳落子策略
 */

#include "windows.h"
#undef SIZE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "machine.h"
#include "check.h"  
#include "chess.h"   // 定义 CHESSSTATE，none，black，white
#include <omp.h>
#include "time.h"

extern clock_t start_time;
extern int count;

/*----------------------------宏定义部分--------------------------*/

#define SIZE 15
// 各种形状的价值
#define VALUE_RIGHT_5 50000
#define VALUE_LIVING_4 4320
#define VALUE_RUSH_4 720
#define VALUE_LIVING_3 720
#define VALUE_RUSH_3 100
#define VALUE_LIVING_2 120
#define VALUE_RUSH_2 20

// 定义搜索深度
int DEPTH = DEPTH_HARD;

#define INF 100000000

/*------------------------------end--------------------------------*/

/*--------------------------结构体定义部分--------------------------*/

// 1: 棋子的落点位置信息
typedef struct {       // 记录点的位置
    int row;
    int column;
} Point, *PTR_To_Point;

// 2: 棋子落点处的连子信息
typedef struct Point_Info
{
    int too_long;     // 长连
    int right_5;      // 合法的5连
    int living_4;      // 合法活4
    int rush_4;        // 合法冲4
    int living_3;      // 合法活3
    int rush_3;        // 合法冲3
    int living_2;      // 合法活2
    int rush_2;        // 合法冲2
} Point_Info;

// 3: 对于单颗棋子的位置信息+价值
typedef struct {
    Point position;
    int score;
    int min;
    int max;
} Points, *PTR_To_Points;

// 4: 棋盘上某点的信息
typedef struct Board_info
{
    Point_Info direction[2];  // [0] for black; [1] for white
    int score[2];             // [0] for black; [1] for white
    int all_score;            // score[0] + score[1]
} Board_info;

/*------------------------------end------------------------------*/

// 特定开局：花月&浦月局（黑有利）
// 7 种开局 × 8 种对称变换 × 5 步落子 × (r, c)
static const Point BlackOpeningPatterns[7][8][5] = 
{
    // 1-3为花月局
    // ============ Opening #1: H8 H9 I9 G7 J10 ============
    {
        // 1) Identity
        { {7,7}, {8,7}, {8,8}, {6,6}, {9,9} },
        // 2) Rotate90
        { {7,7}, {7,6}, {8,6}, {6,8}, {9,5} },
        // 3) Rotate180
        { {7,7}, {6,7}, {6,6}, {8,8}, {5,5} },
        // 4) Rotate270
        { {7,7}, {7,8}, {6,8}, {8,6}, {5,9} },
        // 5) Reflect horizontally (左右翻转)
        { {7,7}, {8,7}, {8,6}, {6,8}, {9,5} },
        // 6) Reflect vertically (上下翻转)
        { {7,7}, {6,7}, {6,8}, {8,6}, {5,9} },
        // 7) Reflect main diagonal (主对角线)
        { {7,7}, {7,8}, {8,8}, {6,6}, {9,9} },
        // 8) Reflect anti-diagonal (反对角线)
        { {7,7}, {7,6}, {6,6}, {8,8}, {5,5} },
    },

    // ============ Opening #2: H8 H9 I9 I8 G7 ============
    {
        // 1) Identity
        { {7,7}, {8,7}, {8,8}, {7,8}, {6,6} },
        // 2) Rotate90
        { {7,7}, {7,6}, {8,6}, {8,7}, {6,8} },
        // 3) Rotate180
        { {7,7}, {6,7}, {6,6}, {7,6}, {8,8} },
        // 4) Rotate270
        { {7,7}, {7,8}, {6,8}, {6,7}, {8,6} },
        // 5) Reflect horizontally
        { {7,7}, {8,7}, {8,6}, {7,6}, {6,8} },
        // 6) Reflect vertically
        { {7,7}, {6,7}, {6,8}, {7,8}, {8,6} },
        // 7) Reflect main diagonal
        { {7,7}, {7,8}, {8,8}, {8,7}, {6,6} },
        // 8) Reflect anti-diagonal
        { {7,7}, {7,6}, {6,6}, {6,7}, {8,8} },
    },

    // ============ Opening #3: H8 H9 I9 J7 G7 ============
    {
        // 1) Identity
        { {7,7}, {8,7}, {8,8}, {6,9}, {6,6} },
        // 2) Rotate90
        { {7,7}, {7,6}, {8,6}, {9,8}, {6,8} },
        // 3) Rotate180
        { {7,7}, {6,7}, {6,6}, {8,5}, {8,8} },
        // 4) Rotate270
        { {7,7}, {7,8}, {6,8}, {5,6}, {8,6} },
        // 5) Reflect horizontally
        { {7,7}, {8,7}, {8,6}, {6,5}, {6,8} },
        // 6) Reflect vertically
        { {7,7}, {6,7}, {6,8}, {8,9}, {8,6} },
        // 7) Reflect main diagonal
        { {7,7}, {7,8}, {8,8}, {9,6}, {6,6} },
        // 8) Reflect anti-diagonal
        { {7,7}, {7,6}, {6,6}, {5,8}, {8,8} },
    },

    // 4-7为浦月局
    // ============ Opening #4: H8 I9 I7 G9 J7 ============
    {
        // 1) Identity
        { {7,7}, {8,8}, {6,8}, {8,6}, {6,9} },
        // 2) Rotate90
        { {7,7}, {8,6}, {8,8}, {6,6}, {9,8} },
        // 3) Rotate180
        { {7,7}, {6,6}, {8,6}, {6,8}, {8,5} },
        // 4) Rotate270
        { {7,7}, {6,8}, {6,6}, {8,8}, {5,6} },
        // 5) Reflect horizontally
        { {7,7}, {8,6}, {6,6}, {8,8}, {6,5} },
        // 6) Reflect vertically
        { {7,7}, {6,8}, {8,8}, {6,6}, {8,9} },
        // 7) Reflect main diagonal
        { {7,7}, {8,8}, {8,6}, {6,8}, {9,6} },
        // 8) Reflect anti-diagonal
        { {7,7}, {6,6}, {6,8}, {8,6}, {5,8} },
    },

    // ============ Opening #5: H8 I9 I7 H7 G9 ============
    {
        // 1) Identity
        { {7,7}, {8,8}, {6,8}, {6,7}, {8,6} },
        // 2) Rotate90
        { {7,7}, {8,6}, {8,8}, {7,8}, {6,6} },
        // 3) Rotate180
        { {7,7}, {6,6}, {8,6}, {8,7}, {6,8} },
        // 4) Rotate270
        { {7,7}, {6,8}, {6,6}, {7,6}, {8,8} },
        // 5) Reflect horizontally
        { {7,7}, {8,6}, {6,6}, {6,7}, {8,8} },
        // 6) Reflect vertically
        { {7,7}, {6,8}, {8,8}, {8,7}, {6,6} },
        // 7) Reflect main diagonal
        { {7,7}, {8,8}, {8,6}, {7,6}, {6,8} },
        // 8) Reflect anti-diagonal
        { {7,7}, {6,6}, {6,8}, {7,8}, {8,6} },
    },

    // ============ Opening #6: H8 I9 I7 H6 G9 ============
    {
        // 1) Identity
        { {7,7}, {8,8}, {6,8}, {5,7}, {8,6} },
        // 2) Rotate90
        { {7,7}, {8,6}, {8,8}, {7,9}, {6,6} },
        // 3) Rotate180
        { {7,7}, {6,6}, {8,6}, {9,7}, {6,8} },
        // 4) Rotate270
        { {7,7}, {6,8}, {6,6}, {7,5}, {8,8} },
        // 5) Reflect horizontally
        { {7,7}, {8,6}, {6,6}, {5,7}, {8,8} },
        // 6) Reflect vertically
        { {7,7}, {6,8}, {8,8}, {9,7}, {6,6} },
        // 7) Reflect main diagonal
        { {7,7}, {8,8}, {8,6}, {7,5}, {6,8} },
        // 8) Reflect anti-diagonal
        { {7,7}, {6,6}, {6,8}, {7,9}, {8,6} },
    },

    // ============ Opening #7: H8 I9 I7 J6 G8 ============
    {
        // 1) Identity
        { {7,7}, {8,8}, {6,8}, {5,9}, {7,6} },
        // 2) Rotate90
        { {7,7}, {8,6}, {8,8}, {9,9}, {6,7} },
        // 3) Rotate180
        { {7,7}, {6,6}, {8,6}, {9,5}, {7,8} },
        // 4) Rotate270
        { {7,7}, {6,8}, {6,6}, {5,5}, {8,7} },
        // 5) Reflect horizontally
        { {7,7}, {8,6}, {6,6}, {5,5}, {7,8} },
        // 6) Reflect vertically
        { {7,7}, {6,8}, {8,8}, {9,9}, {7,6} },
        // 7) Reflect main diagonal
        { {7,7}, {8,8}, {8,6}, {9,5}, {6,7} },
        // 8) Reflect anti-diagonal
        { {7,7}, {6,6}, {6,8}, {5,9}, {8,7} },
    },
};

// 特定开局：斜指开局（为白增加胜率）
// 12 种开局 × 8 种对称变换 × 4 步落子 × (r, c)
static const Point WhiteOpeningPatterns[12][8][4] =
{
    // 长星局
    //=================== #1: H8 I9 J10 H10 ===================
    {
        // 1) Identity
        { {7,7}, {8,8}, {9,9}, {9,7} },
        // 2) Rotate90
        { {7,7}, {8,6}, {9,5}, {7,5} },
        // 3) Rotate180
        { {7,7}, {6,6}, {5,5}, {5,7} },
        // 4) Rotate270
        { {7,7}, {6,8}, {5,9}, {7,9} },
        // 5) Horizontal flip
        { {7,7}, {8,6}, {9,5}, {9,7} },
        // 6) Vertical flip
        { {7,7}, {6,8}, {5,9}, {5,7} },
        // 7) Main diagonal
        { {7,7}, {8,8}, {9,9}, {7,9} },
        // 8) Anti-diagonal
        { {7,7}, {6,6}, {5,5}, {5,7} },
    },

    // 峡月局
    //=================== #2: H8 I9 J9 I8 =====================
    {
        // Identity
        { {7,7}, {8,8}, {8,9}, {7,8} },
        // Rotate90
        { {7,7}, {8,6}, {9,6}, {8,7} },
        // Rotate180
        { {7,7}, {6,6}, {6,5}, {7,6} },
        // Rotate270
        { {7,7}, {6,8}, {5,8}, {6,7} },
        // Horizontal flip
        { {7,7}, {8,6}, {8,5}, {7,6} },
        // Vertical flip
        { {7,7}, {6,8}, {6,9}, {7,8} },
        // Main diagonal
        { {7,7}, {8,8}, {9,8}, {8,7} },
        // Anti-diagonal
        { {7,7}, {6,6}, {5,6}, {6,7} },
    },

    // 水月局
    //=================== #3: H8 I9 J7 J8 =====================
    {
        // Identity
        { {7,7}, {8,8}, {6,9}, {7,9} },
        // Rotate90
        { {7,7}, {8,6}, {9,8}, {9,7} },
        // Rotate180
        { {7,7}, {6,6}, {8,5}, {7,5} },
        // Rotate270
        { {7,7}, {6,8}, {5,6}, {5,7} },
        // Horizontal flip
        { {7,7}, {8,6}, {6,5}, {7,5} },
        // Vertical flip
        { {7,7}, {6,8}, {8,9}, {7,9} },
        // Main diagonal
        { {7,7}, {8,8}, {9,6}, {9,7} },
        // Anti-diagonal
        { {7,7}, {6,6}, {5,8}, {5,7} },
    },

    // 恒星局
    //=================== #4: H8 I9 J8 I8 =====================
    {
        // Identity
        { {7,7}, {8,8}, {7,9}, {7,8} },
        // Rotate90
        { {7,7}, {8,6}, {9,7}, {8,7} },
        // Rotate180
        { {7,7}, {6,6}, {7,5}, {7,6} },
        // Rotate270
        { {7,7}, {6,8}, {5,7}, {6,7} },
        // Horizontal flip
        { {7,7}, {8,6}, {7,5}, {7,6} },
        // Vertical flip
        { {7,7}, {6,8}, {7,9}, {7,8} },
        // Main diagonal
        { {7,7}, {8,8}, {9,7}, {8,7} },
        // Anti-diagonal
        { {7,7}, {6,6}, {5,7}, {6,7} },
    },

    // 流星局
    //=================== #5: H8 I9 J6 I7 =====================
    {
        // Identity
        { {7,7}, {8,8}, {5,9}, {6,8} },
        // Rotate90
        { {7,7}, {8,6}, {9,9}, {8,8} },
        // Rotate180
        { {7,7}, {6,6}, {9,5}, {8,6} },
        // Rotate270
        { {7,7}, {6,8}, {5,5}, {6,6} },
        // Horizontal flip
        { {7,7}, {8,6}, {5,5}, {6,6} },
        // Vertical flip
        { {7,7}, {6,8}, {9,9}, {8,8} },
        // Main diagonal
        { {7,7}, {8,8}, {9,5}, {8,6} },
        // Anti-diagonal
        { {7,7}, {6,6}, {5,9}, {6,8} },
    },

    // 云月局
    //=================== #6: H8 I9 I8 J8 =====================
    {
        // Identity
        { {7,7}, {8,8}, {7,8}, {7,9} },
        // Rotate90
        { {7,7}, {8,6}, {8,7}, {9,7} },
        // Rotate180
        { {7,7}, {6,6}, {7,6}, {7,5} },
        // Rotate270
        { {7,7}, {6,8}, {5,7}, {5,7} }, 
        // Horizontal flip
        { {7,7}, {8,6}, {7,6}, {7,5} },
        // Vertical flip
        { {7,7}, {6,8}, {7,8}, {7,9} },
        // Main diagonal
        { {7,7}, {8,8}, {8,7}, {9,7} },
        // Anti-diagonal
        { {7,7}, {6,6}, {6,7}, {5,7} },
    },

    // 浦月局
    //=================== #7: H8 I9 I7 G9 =====================
    {
        // Identity
        { {7,7}, {8,8}, {6,8}, {8,6} },
        // Rotate90
        { {7,7}, {8,6}, {8,8}, {6,6} },
        // Rotate180
        { {7,7}, {6,6}, {8,6}, {6,8} },
        // Rotate270
        { {7,7}, {6,8}, {6,6}, {8,8} },
        // Horizontal flip
        { {7,7}, {8,6}, {6,6}, {8,8} },
        // Vertical flip
        { {7,7}, {6,8}, {8,8}, {6,6} },
        // Main diagonal
        { {7,7}, {8,8}, {8,6}, {6,8} },
        // Anti-diagonal
        { {7,7}, {6,6}, {6,8}, {8,6} },
    },

    // 岚月局
    //=================== #8: H8 I9 I6 G8 =====================
    {
        // Identity
        { {7,7}, {8,8}, {5,8}, {7,6} },
        // Rotate90
        { {7,7}, {8,6}, {8,9}, {6,7} },
        // Rotate180
        { {7,7}, {6,6}, {9,6}, {7,8} },
        // Rotate270
        { {7,7}, {6,8}, {6,5}, {8,7} },
        // Horizontal flip
        { {7,7}, {8,6}, {5,6}, {7,8} },
        // Vertical flip
        { {7,7}, {6,8}, {9,8}, {7,6} },
        // Main diagonal
        { {7,7}, {8,8}, {8,5}, {6,7} },
        // Anti-diagonal
        { {7,7}, {6,6}, {6,9}, {8,7} },
    },

    // 银月局
    //=================== #9: H8 I9 H7 H6 =====================
    {
        // Identity
        { {7,7}, {8,8}, {6,7}, {5,7} },
        // Rotate90
        { {7,7}, {8,6}, {7,8}, {7,9} },
        // Rotate180
        { {7,7}, {6,6}, {8,7}, {9,7} },
        // Rotate270
        { {7,7}, {6,8}, {7,6}, {7,5} },
        // Horizontal flip
        { {7,7}, {8,6}, {6,7}, {5,7} },
        // Vertical flip
        { {7,7}, {6,8}, {8,7}, {9,7} },
        // Main diagonal
        { {7,7}, {8,8}, {7,6}, {7,5} },
        // Anti-diagonal
        { {7,7}, {6,6}, {7,8}, {7,9} },
    },

    // 明星局
    //=================== #10: H8 I9 H6 H9 ====================
    {
        // Identity
        { {7,7}, {8,8}, {5,7}, {8,7} },
        // Rotate90
        { {7,7}, {8,6}, {7,9}, {7,6} },
        // Rotate180
        { {7,7}, {6,6}, {9,7}, {6,7} },
        // Rotate270
        { {7,7}, {6,8}, {7,5}, {7,6} },
        // Horizontal flip
        { {7,7}, {8,6}, {5,7}, {8,7} },
        // Vertical flip
        { {7,7}, {6,8}, {9,7}, {6,7} },
        // Main diagonal
        { {7,7}, {8,8}, {7,5}, {7,8} },
        // Anti-diagonal
        { {7,7}, {6,6}, {7,9}, {7,6} },
    },

    // 斜月局
    //=================== #11: H8 I9 G7 H9 ====================
    {
        // Identity
        { {7,7}, {8,8}, {6,6}, {8,7} },
        // Rotate90
        { {7,7}, {8,6}, {6,8}, {7,6} },
        // Rotate180
        { {7,7}, {6,6}, {8,8}, {6,7} },
        // Rotate270
        { {7,7}, {6,8}, {8,6}, {7,8} },
        // Horizontal flip
        { {7,7}, {8,6}, {6,8}, {8,7} },
        // Vertical flip
        { {7,7}, {6,8}, {8,6}, {6,7} },
        // Main diagonal
        { {7,7}, {8,8}, {6,6}, {7,8} },
        // Anti-diagonal
        { {7,7}, {6,6}, {8,8}, {6,7} },
    },

    // 名月局
    //=================== #12: H8 I9 G6 I8 ====================
    {
        // Identity
        { {7,7}, {8,8}, {5,6}, {7,8} },
        // Rotate90
        { {7,7}, {8,6}, {6,9}, {8,7} },
        // Rotate180
        { {7,7}, {6,6}, {9,8}, {7,6} },
        // Rotate270
        { {7,7}, {6,8}, {8,5}, {6,7} },
        // Horizontal flip
        { {7,7}, {8,6}, {5,8}, {7,6} },
        // Vertical flip
        { {7,7}, {6,8}, {9,6}, {7,8} },
        // Main diagonal
        { {7,7}, {8,8}, {6,5}, {8,7} },
        // Anti-diagonal
        { {7,7}, {6,6}, {8,9}, {6,7} },
    },
};

/*--------------------------函数声明部分--------------------------*/

// 评估单个点的价值
void Evaluate_Point_Value(Board_info** Evaluated_Board, int row, int column);

// 禁手判断中的递归函数
bool Check_Ban_Nextpoint(CHESSSTATE** board, int row, int column, int color);

// 判断当前点是否为禁手
bool Check_Ban(Board_info** Evaluated_Board, int row,int column,int color);

// 检查禁手并评估当前落子的价值到数组中
void Point_Evaluate(CHESSSTATE** board, Board_info** Evaluated_Board, int row, int column, int color);

// 重置 Evaluated_Board 中指定点的评估信息
void Remake_Evaluated_Board(Board_info** Evaluated_Board, int row, int column, int index);

// 计算指定点在各个方向上的连子和空位信息
void Calculate_array(CHESSSTATE** board, int row, int column, int color,
                    int *same, int *same_empty,
                    int *same_empty_same, int *same_empty_same_empty,
                    int *same_empty_same_empty_same);

// 递归判断是否为禁手
bool Check_Ban_Point(CHESSSTATE** board, int row, int column, int distance, int direction, int color);

// 评估棋盘的整体价值
int Evaluate_Board_Value(CHESSSTATE** board, Board_info** Evaluated_Board, int color);

// AI 的主函数，用于选择最佳落子
int AI_Play(CHESSSTATE** board, CHESSSTATE machine_color, char able, MovePoint* final_move);

// 查找棋盘上的候选点
int FindPoints(CHESSSTATE** board, Board_info** Evaluated_Board, PTR_To_Points All_Points, int color, bool flag, bool *flag_if_win, PTR_To_Point winning_move, bool *flag_will_lose, PTR_To_Point blocking_move);

// 快速排序函数
void quicksort(PTR_To_Points s, int left, int right);

// 查找基准点
int FindBasePoint(CHESSSTATE** board, Board_info** Evaluated_Board, int color);

// 选择最佳落子
int BestChoice(CHESSSTATE** board, Board_info** Evaluated_Board, int color, PTR_To_Point final, int depth, int a, int b, char able);

/*------------------------------end------------------------------*/

/*--------------------------函数实现部分--------------------------*/

/**
 * @brief 重置 Evaluated_Board 中指定点的评估信息
 * 
 * @param row 行坐标
 * @param column 列坐标
 * @param index 颜色索引（0 for black, 1 for white）
 */
void Remake_Evaluated_Board(Board_info** Evaluated_Board, int row, int column, int index)
{
    Evaluated_Board[row][column].score[index] = 0;
    Evaluated_Board[row][column].direction[index].too_long = 0;
    Evaluated_Board[row][column].direction[index].right_5 = 0;
    Evaluated_Board[row][column].direction[index].living_4 = 0;
    Evaluated_Board[row][column].direction[index].rush_4 = 0;
    Evaluated_Board[row][column].direction[index].living_3 = 0;
    Evaluated_Board[row][column].direction[index].rush_3 = 0;
    Evaluated_Board[row][column].direction[index].living_2 = 0;
    Evaluated_Board[row][column].direction[index].rush_2 = 0;
}

/**
 * @brief 计算指定点在各个方向上的连子和空位信息
 * 
 * @param board 当前棋盘状态
 * @param row 行坐标
 * @param column 列坐标
 * @param color 棋子颜色（black 或 white）
 * @param same 连续同色子数
 * @param same_empty 连续同色子后的空位数
 * @param same_empty_same 连续同色子后的第二段同色子数
 * @param same_empty_same_empty 连续同色子后的第二段空位数
 * @param same_empty_same_empty_same 连续同色子后的第三段同色子数
 */
void Calculate_array(CHESSSTATE** board, int row, int column, int color,
                    int *same, int *same_empty,
                    int *same_empty_same, int *same_empty_same_empty,
                    int *same_empty_same_empty_same)
{
    // 各个方向的偏移量
    int directions_offsets[8][2] = {
        {0, -1},    // 向上
        {1, -1},    // 向右上
        {1, 0},     // 向右
        {1, 1},     // 向右下
        {0, 1},     // 向下
        {-1, 1},    // 向左下
        {-1, 0},    // 向左
        {-1, -1}    // 向左上
    };

    for(int d = 0; d < 8; d++)
    {
        int x = column, y = row;

        // 第一段同色子
        while(true)
        {
            x += directions_offsets[d][0];
            y += directions_offsets[d][1];
            if(x < 0 || x >= SIZE || y < 0 || y >= SIZE || board[y][x] != color)
                break;
            same[d]++;
        }
        x -= directions_offsets[d][0];
        y -= directions_offsets[d][1];

        // 第一段空位
        while(true)
        {
            x += directions_offsets[d][0];
            y += directions_offsets[d][1];
            if(x < 0 || x >= SIZE || y < 0 || y >= SIZE || board[y][x] != none)
                break;
            same_empty[d]++;
        }
        x -= directions_offsets[d][0];
        y -= directions_offsets[d][1];

        // 第二段同色子
        while(true)
        {
            x += directions_offsets[d][0];
            y += directions_offsets[d][1];
            if(x < 0 || x >= SIZE || y < 0 || y >= SIZE || board[y][x] != color)
                break;
            same_empty_same[d]++;
        }
        x -= directions_offsets[d][0];
        y -= directions_offsets[d][1];

        // 第二段空位
        while(true)
        {
            x += directions_offsets[d][0];
            y += directions_offsets[d][1];
            if(x < 0 || x >= SIZE || y < 0 || y >= SIZE || board[y][x] != none)
                break;
            same_empty_same_empty[d]++;
        }
        x -= directions_offsets[d][0];
        y -= directions_offsets[d][1];

        // 第三段同色子
        while(true)
        {
            x += directions_offsets[d][0];
            y += directions_offsets[d][1];
            if(x < 0 || x >= SIZE || y < 0 || y >= SIZE || board[y][x] != color)
                break;
            same_empty_same_empty_same[d]++;
        }
    }
}

/**
 * @brief 递归判断是否为禁手
 * 
 * @param board 棋盘状态
 * @param row 行坐标
 * @param column 列坐标
 * @param distance 距离
 * @param direction 方向索引
 * @param color 棋子颜色
 * @return true 非禁手
 * @return false 禁手
 */
bool Check_Ban_Point(CHESSSTATE** board, int row, int column, int distance, int direction, int color)
{    
    if(color == white)
    {         // 如果是白棋显然没有禁手问题
        return true;
    }          
    int next_row, next_column;
    distance++;    // 因为相隔一个，坐标数要加一
    if(direction >= 4)
    {
        distance = -distance;       // 方向相反，加减应随之相反
    }
    // 计算关键点坐标
    switch(direction % 4)
    {
        case 0:
            next_row = row - distance;
            next_column = column;
            break;
        case 1:
            next_row = row - distance;
            next_column = column + distance;
            break;
        case 2:
            next_row = row;
            next_column = column + distance;
            break;
        case 3:
            next_row = row + distance;
            next_column = column + distance;
            break;
        default:
            printf("Invalid direction\n");
            break;
    }

    bool is_evaluating = false;
    if(board[row][column] != none)
    {
        is_evaluating = true;
    }

    board[row][column] = color;        // 将刚才那个子落了

    bool flag = Check_Ban_Nextpoint(board, next_row, next_column, color);
    //printf("(%d, %d)",next_row, next_column);

    //flag = true;

    if(!is_evaluating)
    {         
        // 适应判断局面分的需要，在判断局面分时，原本这里就有子，不能擦除 
        board[row][column] = none;        // 恢复 
    }
    return flag;
}

/**
 * @brief 判断关键点是否是禁手
 * 
 * @param board 棋盘状态
 * @param row 行坐标
 * @param column 列坐标
 * @param color 棋子颜色
 * @return true 非禁手
 * @return false 禁手
 */
bool Check_Ban_Nextpoint(CHESSSTATE** board, int row, int column, int color)
{
    int same[8] = {0};      
    int same_empty[8] = {0};   
    int same_empty_same[8] = {0}; 
    int same_empty_same_empty[8] = {0};
    int same_empty_same_empty_same[8] = {0};

    // 计算各个方向的信息
    Calculate_array(board, row, column, color,
                    same, same_empty, same_empty_same,
                    same_empty_same_empty, same_empty_same_empty_same);

    // 统计活四、冲四和活三的数量
    int living_4 = 0, rush_4 = 0, living_3 = 0;

    for(int i = 0; i < 4; i++)
    {
        // 判断是否构成不禁手的五连
        if(same[i] + same[i+4] == 4 || (same[i] == 4 && same[i+4] == 4))
        {
            return true;  // 非禁手
        }

        // 判断长连禁手
        if(same[i] + same[i+4] >= 5)
        {
            return false; // 禁手
        }

        // 判断4连
        if(same[i] + same[i+4] == 3)
        {
            int flag = 0;
            if(same_empty[i] > 0 && Check_Ban_Point(board, row, column, same[i], i, color))
                flag++;
            if(same_empty[i+4] > 0 && Check_Ban_Point(board, row, column, same[i+4], i+4, color))
                flag++;

            if(flag == 2)
                living_4 += 1;
            else if(flag == 1)
                rush_4 += 1;
        }

        // 判断3连
        if(same[i] + same[i+4] == 2)
        {
            int flag = 0;
            if(same_empty[i] == 1 && same_empty_same[i] == 1 && Check_Ban_Point(board, row, column, same[i], i, color))
                flag++;
            if(same_empty[i+4] == 1 && same_empty_same[i+4] == 1 && Check_Ban_Point(board, row, column, same[i+4], i+4, color))
                flag++;

            if(flag == 2)
                living_4 += 1;
            else if(flag == 1)
                rush_4 += 1;
            else
            {
                // 检查是否有活三
                bool flag_living_3 = false;
                if((same_empty[i] > 2 || (same_empty[i] == 2 && same_empty_same[i] == 0)) &&
                   (same_empty[i+4] > 1 || (same_empty[i+4] == 1 && same_empty_same[i+4] == 0)) &&
                   Check_Ban_Point(board, row, column, same[i], i, color))
                {
                    flag_living_3 = true;
                }
                if((same_empty[i+4] > 2 || (same_empty[i+4] == 2 && same_empty_same[i+4] == 0)) &&
                   (same_empty[i] > 1 || (same_empty[i] == 1 && same_empty_same[i] == 0)) &&
                   Check_Ban_Point(board, row, column, same[i+4], i+4, color))
                {
                    flag_living_3 = true;
                }

                if(flag_living_3)
                    living_3 += 1;
            }
        }

        // 判断2连
        if(same[i] + same[i+4] == 1)
        {
            int flag = 0;
            if(same_empty[i] == 1 && same_empty_same[i] == 2 && Check_Ban_Point(board, row, column, same[i], i, color))
                flag++;
            if(same_empty[i+4] == 1 && same_empty_same[i+4] == 2 && Check_Ban_Point(board, row, column, same[i+4], i+4, color))
                flag++;

            if(flag == 2)
                living_4 += 1;
            else if(flag == 1)
                rush_4 += 1;
            else
            {
                // 检查是否有活三
                bool flag_living_3 = false;
                if((same_empty[i] == 1 && same_empty_same[i] == 1 &&
                   (same_empty_same_empty[i] > 1 || (same_empty_same_empty[i] == 1 && same_empty_same_empty_same[i] == 0)) &&
                   (same_empty[i+4] > 1 || (same_empty[i+4] == 1 && same_empty_same[i+4] == 0)) &&
                   Check_Ban_Point(board, row, column, same[i], i, color))
                )
                {
                    flag_living_3 = true;
                }
                if((same_empty[i+4] == 1 && same_empty_same[i+4] == 1 &&
                   (same_empty_same_empty[i+4] > 1 || (same_empty_same_empty[i+4] == 1 && same_empty_same_empty_same[i+4] == 0)) &&
                   (same_empty[i] > 1 || (same_empty[i] == 1 && same_empty_same[i] == 0)) &&
                   Check_Ban_Point(board, row, column, same[i+4], i+4, color))
                )
                {
                    flag_living_3 = true;
                }

                if(flag_living_3)
                    living_3 += 1;
            }
        }
        //单独一子
        if(same[i]+same[i+4]==0){
            //活四冲四判断
            //形如“oxoxxx”且中间o可落子位活四
            bool flag=false;
            if(same_empty[i]==1 && same_empty_same[i]==3){
                if(Check_Ban_Point(board,row,column,same[i],i,color)){    //需要判断的点是“oxoxxx”中间的o
                    rush_4+=1;
                    flag=true;
                }
            }
            if(same_empty[i+4]==1 && same_empty_same[i+4]==3){
                if(Check_Ban_Point(board,row,column,same[i+4],i+4,color)){
                    rush_4+=1;
                    flag=true;
                }
            }
            //活三判断
            //形如“ooxoxxoo”或“ooxoxxo”或“oxoxxoo”的且中间o可落子为活三

            if(!flag){
                bool flag_living_3=false;

                if(same_empty[i]==1 && same_empty_same[i]==2
                    && (same_empty_same_empty[i]>1 || (same_empty_same_empty[i]==1 && same_empty_same_empty_same[i]==0))
                    && (same_empty[i+4]>1 || (same_empty[i+4]==1 && same_empty_same[i+4]==0)))
                {
                    if(Check_Ban_Point(board,row,column,same[i],i,color)){   //需要判断的点是中间的o
                        flag_living_3=true;
                    }
                }
                if(same_empty[i+4]==1 && same_empty_same[i+4]==2
                    && (same_empty_same_empty[i+4]>1 || (same_empty_same_empty[i+4]==1 && same_empty_same_empty_same[i+4]==0))
                    && (same_empty[i]>1 || (same_empty[i]==1 && same_empty_same[i]==0)))
                {
                    if(Check_Ban_Point(board,row,column,same[i+4],i+4,color)){   //需要判断的点是中间的o
                        flag_living_3=true;
                    }
                }
                if(flag_living_3){
                    living_3+=1;
                }
            }
        }
    }

    // 判断禁手
    if(living_3 == 1 && (living_4 + rush_4) == 1)
    {
        // 先考虑特殊情况，即四三不是禁手。
        return true;            
    }

    if(living_3 + living_4 + rush_4 >= 2)
    {             
        // 除了一个四一个三以外，其余情况下只要和大于等于2，一定是禁手。
        return false;
    }
    else{
        return true;
    }
} 

/**
 * @brief 评估当前落子的价值
 * 
 * @param board 当前棋盘状态，二维数组
 * @param row 行坐标
 * @param column 列坐标
 * @param color 棋子颜色（black 或 white）
 */
void Point_Evaluate(CHESSSTATE** board, Board_info** Evaluated_Board, int row, int column, int color)
{
    int index = (color == black) ? 0 : 1;  // 0 for black, 1 for white

    // 重置 Evaluated_Board 中当前点的评估信息
    Remake_Evaluated_Board(Evaluated_Board, row, column, index);

    // 初始化数组
    int same[8] = {0};
    int same_empty[8] = {0};
    int same_empty_same[8] = {0};
    int same_empty_same_empty[8] = {0};
    int same_empty_same_empty_same[8] = {0};

    // 计算各个方向的信息
    Calculate_array(board, row, column, color,
                    same, same_empty, same_empty_same,
                    same_empty_same_empty, same_empty_same_empty_same);

    for(int i = 0; i < 4; i++)
    {
        // 判断是否构成不禁手的五连
        if(same[i] + same[i+4] == 4 || (same[i] == 4 && same[i+4] == 4))
        {
            Evaluated_Board[row][column].direction[index].right_5 = 1;
            return;
        }
        // 如果是白棋，则长连也算赢 
        if(same[i] + same[i+4] >= 5 && color == white){    // 只有黑棋有禁手
            Evaluated_Board[row][column].direction[index].right_5 = 1;
            return;
        }
        // 判断长连禁手
        if(same[i] + same[i+4] >= 5 && color == black)
        {
            Evaluated_Board[row][column].direction[index].too_long = 1;
            return;
        }

        // 判断4连
        if(same[i] + same[i+4] == 3)
        {
            int flag = 0;
            if(same_empty[i] > 0 && Check_Ban_Point(board, row, column, same[i], i, color))
                flag++;
            if(same_empty[i+4] > 0 && Check_Ban_Point(board, row, column, same[i+4], i+4, color))
                flag++;

            if(flag == 2)
                Evaluated_Board[row][column].direction[index].living_4 += 1;
            else if(flag == 1)
                Evaluated_Board[row][column].direction[index].rush_4 += 1;
        }

        // 判断3连
        if(same[i] + same[i+4] == 2)
        {
            int flag = 0;
            if(same_empty[i] == 1 && same_empty_same[i] == 1 && Check_Ban_Point(board, row, column, same[i], i, color))
                flag++;
            if(same_empty[i+4] == 1 && same_empty_same[i+4] == 1 && Check_Ban_Point(board, row, column, same[i+4], i+4, color))
                flag++;

            if(flag == 2)
                Evaluated_Board[row][column].direction[index].living_4 += 1;
            else if(flag == 1)
                Evaluated_Board[row][column].direction[index].rush_4 += 1;
            else
            {
                //检查上方是否有活三，即下一步落子在上方第一个空位上能否形成活四；如果没有活三，判断是否有冲3
                //例如（以右为上）"ooxxxooo" / "ooxxxoo|" / "|oxxxooo" 下一步落子在右边第一个o处(也是Check_Ban_Point应传入的参数都能形成活四，故为活三
                //而形如"xoxxxooo"  / "ooxxxoox" 下一步落子在右边第一个o处时是无法形成活四的（禁手限制）
        
                bool flag1_1=(same_empty[i]>2 || (same_empty[i]==2 && same_empty_same[i]==0))? true : false;
                bool flag1_2=(same_empty[i]==1 && same_empty_same[i]==0)? true : false;
                bool flag1_3=(same_empty[i+4]>1 || (same_empty[i+4]==1 && same_empty_same[i+4]==0))? true : false;

                bool flag2_1=(same_empty[i+4]>2 || (same_empty[i+4]==2 && same_empty_same[i+4]==0))? true : false;
                bool flag2_2=(same_empty[i+4]==1 && same_empty_same[i+4]==0)? true : false;
                bool flag2_3=(same_empty[i]>1 || (same_empty[i]==1 && same_empty_same[i]==0))? true : false;

                if((flag1_1 && flag1_3 && Check_Ban_Point(board,row,column,same[i],i,color)) || (flag2_1 && flag2_3 && Check_Ban_Point(board,row,column,same[i+4],i+4,color))){
                    Evaluated_Board[row][column].direction[index].living_3+=1;
                }
                else if( (((flag1_2 && flag1_3) || (flag1_1 && !flag1_3)) && Check_Ban_Point(board,row,column,same[i],i,color))
                        || (((flag2_2 && flag2_3) || (flag2_1 && !flag2_3)) && Check_Ban_Point(board,row,column,same[i+4],i+4,color)) )
                {
                    Evaluated_Board[row][column].direction[index].rush_3+=1;
                }
            }
        }

        // 判断2连
        if(same[i] + same[i+4] == 1)
        {
            int flag = 0;
            if(same_empty[i] == 1 && same_empty_same[i] == 2 && Check_Ban_Point(board, row, column, same[i], i, color))
                flag++;
            if(same_empty[i+4] == 1 && same_empty_same[i+4] == 2 && Check_Ban_Point(board, row, column, same[i+4], i+4, color))
                flag++;

            if(flag == 2)
                Evaluated_Board[row][column].direction[index].living_4 += 1;
            else if(flag == 1)
                Evaluated_Board[row][column].direction[index].rush_4 += 1;
            else
            {
                //活三判断
                bool flag1_1=(same_empty[i]==1 && same_empty_same[i]==1)? true : false;
                bool flag1_2=(same_empty_same_empty[i]>1 || (same_empty_same_empty[i]==1 && same[i]==0))? true : false;
                bool flag1_3=(same_empty[i+4]>1 || (same_empty[i+4]==1 && same_empty_same[i+4]==0))? true : false;

                bool flag2_1=(same_empty[i+4]==1 && same_empty_same[i+4]==1)? true : false;
                bool flag2_2=(same_empty_same_empty[i+4]>1 || (same_empty_same_empty[i+4]==1 && same[i+4]==0))? true : false;
                bool flag2_3=(same_empty[i]>1 || (same_empty[i]==1 && same_empty_same[i]==0))? true : false;

                if((flag1_1 && flag1_2 && flag1_3 && Check_Ban_Point(board,row,column,same[i],i,color)) || (flag2_1 && flag2_2 && flag2_3 && Check_Ban_Point(board,row,column,same[i+4],i+4,color))){
                    Evaluated_Board[row][column].direction[index].living_3+=1;
                }
                else if( (flag1_1 && (flag1_2 || flag1_3) && Check_Ban_Point(board,row,column,same[i],i,color)) || (flag2_1 && (flag2_2 || flag2_3) && Check_Ban_Point(board,row,column,same[i+4],i+4,color)) ){
                    Evaluated_Board[row][column].direction[index].rush_3+=1;
                }
            }
            //判断活2（计算价值用）
            bool flag1_1=(same_empty[i]>2 || (same_empty[i]==2 && same_empty_same[i]==0))? true : false;
            bool flag1_2=(same_empty[i]>3 || (same_empty[i]==3 && same_empty_same[i]==0))? true : false;
            bool flag1_3=(same_empty[i+4]>1 || (same_empty[i+4]==1 && same_empty_same[i+4]==0))? true : false;

            bool flag2_1=(same_empty[i+4]>2 || (same_empty[i+4]==2 && same_empty_same[i+4]==0))? true : false;
            bool flag2_2=(same_empty[i+4]>3 || (same_empty[i+4]==3 && same_empty_same[i+4]==0))? true : false;
            bool flag2_3=(same_empty[i]>1 || (same_empty[i]==1 && same_empty_same[i]==0))? true : false;

            if( (flag1_1 && flag1_2 && flag1_3 && Check_Ban_Point(board,row,column,same[i],i,color) && Check_Ban_Point(board,row,column,same[i]+1,i,color)) || (flag2_1 && flag2_2 && flag2_3 && Check_Ban_Point(board,row,column,same[i+4],i+4,color) && Check_Ban_Point(board,row,column,same[i+4]+1,i+4,color)) ){
                Evaluated_Board[row][column].direction[index].living_2+=1;
            }
            else if( (flag1_1 && (flag1_2 || flag1_3) && Check_Ban_Point(board,row,column,same[i],i,color) && Check_Ban_Point(board,row,column,same[i]+1,i,color)) || (flag2_1 && (flag2_2 || flag2_3) && Check_Ban_Point(board,row,column,same[i+4],i+4,color) && Check_Ban_Point(board,row,column,same[i+4]+1,i+4,color)) ){
                Evaluated_Board[row][column].direction[index].rush_2+=1;
            }
        }

        // 判断单一子
        if(same[i] + same[i+4] == 0)
        {
            int flag = 0;
            if(same_empty[i] == 1 && same_empty_same[i] == 3 && Check_Ban_Point(board, row, column, same[i], i, color))
                flag++;
            if(same_empty[i+4] == 1 && same_empty_same[i+4] == 3 && Check_Ban_Point(board, row, column, same[i+4], i+4, color))
                flag++;

            if(flag >= 1)
                Evaluated_Board[row][column].direction[index].rush_4 += flag;
            else // 有冲四了就不用判断三
            {
                // 活三判断
                // 形如“ooxoxxoo”或“ooxoxxo”或“oxoxxoo”的且中间o可落子为活三
                bool flag1_1 = (same_empty[i] == 1 && same_empty_same[i] == 2) ? true : false;
                bool flag1_2 = (same_empty_same_empty[i] > 1 || (same_empty_same_empty[i] == 1 && same[i] == 0)) ? true : false;
                bool flag1_3 = (same_empty[i+4] > 1 || (same_empty[i+4] == 1 && same_empty_same[i+4] == 0)) ? true : false;

                bool flag2_1 = (same_empty[i+4] == 1 && same_empty_same[i+4] == 2) ? true : false;
                bool flag2_2 = (same_empty_same_empty[i+4] > 1 || (same_empty_same_empty[i+4] == 1 && same[i+4] == 0)) ? true : false;
                bool flag2_3 = (same_empty[i] > 1 || (same_empty[i] == 1 && same_empty_same[i] == 0)) ? true : false;

                if( (flag1_1 && flag1_2 && flag1_3 && Check_Ban_Point(board, row, column, same[i], i, color)) || 
                    (flag2_1 && flag2_2 && flag2_3 && Check_Ban_Point(board, row, column, same[i+4], i+4, color)) ){
                    Evaluated_Board[row][column].direction[index].living_3 += 1;
                    flag = 1;
                }
                else if( (flag1_1 && (flag1_2 || flag1_3) && Check_Ban_Point(board, row, column, same[i], i, color)) || 
                         (flag2_1 && (flag2_2 || flag2_3) && Check_Ban_Point(board, row, column, same[i+4], i+4, color)) ){
                    Evaluated_Board[row][column].direction[index].rush_3 += 1;
                    flag = 1;
                }
            }
            if(flag == 0){
                bool flag1_1 = (same_empty[i] == 1 && same_empty_same[i] == 1 && same_empty_same_empty[i] >= 1) ? true : false;
                bool flag1_2 = (same_empty_same_empty[i] > 1 || (same_empty_same_empty[i] == 1 && same[i] == 0)) ? true : false;
                bool flag1_3 = (same_empty[i+4] > 1 || (same_empty[i+4] == 1 && same_empty_same[i+4] == 0)) ? true : false;

                bool flag2_1 = (same_empty[i+4] == 1 && same_empty_same[i+4] == 1 && same_empty_same_empty[i+4] >= 1) ? true : false;
                bool flag2_2 = (same_empty_same_empty[i+4] > 1 || (same_empty_same_empty[i+4] == 1 && same[i+4] == 0)) ? true : false;
                bool flag2_3 = (same_empty[i] > 1 || (same_empty[i] == 1 && same_empty_same[i] == 0)) ? true : false;

                if( (flag1_1 && flag1_2 && flag1_3 && Check_Ban_Point(board, row, column, same[i], i, color) && 
                     Check_Ban_Point(board, row, column, same[i]+2, i, color)) || 
                    (flag2_1 && flag2_2 && flag2_3 && Check_Ban_Point(board, row, column, same[i+4], i+4, color) && 
                     Check_Ban_Point(board, row, column, same[i+4]+2, i+4, color)) ){
                    Evaluated_Board[row][column].direction[index].living_2 += 1;
                }
                else if( (flag1_1 && (flag1_2 || flag1_3) && Check_Ban_Point(board, row, column, same[i], i, color) && 
                           Check_Ban_Point(board, row, column, same[i]+2, i, color)) || 
                         (flag2_1 && (flag2_2 || flag2_3) && Check_Ban_Point(board, row, column, same[i+4], i+4, color) && 
                           Check_Ban_Point(board, row, column, same[i+4]+2, i+4, color)) ){
                    Evaluated_Board[row][column].direction[index].rush_2 += 1;
                }
            }
        }
    }
}

/**
 * @brief 评估棋盘的整体价值
 * 
 * @param board 当前棋盘状态
 * @param color 评估的视角颜色（black 或 white）
 * @return int 评估分数
 */
int Evaluate_Board_Value(CHESSSTATE** board, Board_info** Evaluated_Board, int color)
{
    // 优先级是对方冲四 > 己方活四 > 对方活3 
    int self_value_living_4 = 100000;
    int rival_value_living_4 = 10000000; 
    int self_value_rush_4 = 720;
    int rival_value_rush_4 = 1000000;
    int self_value_living_3 = 720;
    int rival_value_living_3 = 50000;
    int self_value_rush_3 = 480;
    int rival_value_rush_3 = 720;
    int self_value_living_2 = 480;
    int rival_value_living_2 = 480;
    int self_value_rush_2 = 20;
    int rival_value_rush_2 = 100;
    // 这里关于己方活三，活二，对面冲三，等的赋值还有待商榷 

    int living_4[2] = {0, 0};
    int rush_4[2] = {0, 0};
    int living_3[2] = {0, 0};
    int rush_3[2] = {0, 0};
    int living_2[2] = {0, 0};
    int rush_2[2] = {0, 0};

    for(int i = 0; i < SIZE; i++)
    {
        for(int j = 0; j < SIZE; j++)
        {
            if(board[i][j] == black)
            {
                Point_Evaluate(board, Evaluated_Board, i, j, black);
                living_4[0] += Evaluated_Board[i][j].direction[0].living_4;
                rush_4[0] += Evaluated_Board[i][j].direction[0].rush_4;
                living_3[0] += Evaluated_Board[i][j].direction[0].living_3;
                rush_3[0] += Evaluated_Board[i][j].direction[0].rush_3;
                living_2[0] += Evaluated_Board[i][j].direction[0].living_2;
                rush_2[0] += Evaluated_Board[i][j].direction[0].rush_2;   
            }
            if(board[i][j] == white)
            {
                Point_Evaluate(board, Evaluated_Board, i, j, white);
                living_4[1] += Evaluated_Board[i][j].direction[1].living_4;
                rush_4[1] += Evaluated_Board[i][j].direction[1].rush_4;
                living_3[1] += Evaluated_Board[i][j].direction[1].living_3;
                rush_3[1] += Evaluated_Board[i][j].direction[1].rush_3;
                living_2[1] += Evaluated_Board[i][j].direction[1].living_2;
                rush_2[1] += Evaluated_Board[i][j].direction[1].rush_2;
            }
        }
    }

    int value = 0;
    if(color == black)
    {
        value += living_4[0] * self_value_living_4 / 4;
        value += rush_4[0] * self_value_rush_4 / 4;
        value += living_3[0] * self_value_living_3 / 3;
        value += rush_3[0] * self_value_rush_3 / 3;
        value += living_2[0] * self_value_living_2 / 2;
        value += rush_2[0] * self_value_rush_2 / 2;
        
        value -= living_4[1] * rival_value_rush_4 / 4;
        value -= rush_4[1] * rival_value_rush_4 / 4;
        value -= living_3[1] * rival_value_living_3 / 3;
        value -= rush_3[1] * rival_value_rush_3 / 3;
        value -= living_2[1] * rival_value_living_2 / 2;
        value -= rush_2[1] * rival_value_rush_2 / 2;
    }
    else
    {
        value += living_4[1] * self_value_living_4 / 4;
        value += rush_4[1] * self_value_rush_4 / 4;
        value += living_3[1] * self_value_living_3 / 3;
        value += rush_3[1] * self_value_rush_3 / 3;
        value += living_2[1] * self_value_living_2 / 2;
        value += rush_2[1] * self_value_rush_2 / 2;
        
        value -= living_4[0] * rival_value_rush_4 / 4;
        value -= rush_4[0] * rival_value_rush_4 / 4;
        value -= living_3[0] * rival_value_living_3 / 3;
        value -= rush_3[0] * rival_value_rush_3 / 3;
        value -= living_2[0] * rival_value_living_2 / 2;
        value -= rush_2[0] * rival_value_rush_2 / 2;
    }
    return value;
}

/**
 * @brief 检查禁手
 * 
 * @param row 行坐标
 * @param column 列坐标
 * @param color 棋子颜色（black 或 white）
 */
bool Check_Ban(Board_info** Evaluated_Board, int row, int column, int color){
    if (color == white) return true;
    if(Evaluated_Board[row][column].direction[color-1].too_long==true){
        return false;
    }
    if(Evaluated_Board[row][column].direction[color-1].living_3>=2 && Evaluated_Board[row][column].direction[color-1].living_4==0 && Evaluated_Board[row][column].direction[color-1].rush_4==0){
        return false;
    }
    if(Evaluated_Board[row][column].direction[color-1].living_4+Evaluated_Board[row][column].direction[color-1].rush_4>=2 && Evaluated_Board[row][column].direction[color-1].living_3==0){
        return false;
    }
    if(Evaluated_Board[row][column].direction[color-1].living_4+Evaluated_Board[row][column].direction[color-1].rush_4==1 && Evaluated_Board[row][column].direction[color-1].living_3>=2){
        return false;
    }
    if(Evaluated_Board[row][column].direction[color-1].living_4+Evaluated_Board[row][column].direction[color-1].rush_4==2 && Evaluated_Board[row][column].direction[color-1].living_3>=1){
        return false;
    }
    if(Evaluated_Board[row][column].direction[color-1].living_4+Evaluated_Board[row][column].direction[color-1].rush_4==3 && Evaluated_Board[row][column].direction[color-1].living_3==1){
        return false;
    }
    //注意43不是禁手
    return true;
}

/**
 * @brief 查找棋盘上的候选点
 * 
 * @param board 当前棋盘状态
 * @param All_Points 所有候选点数组
 * @param color 当前玩家颜色
 * @param flag 是否需要计算点值
 * @param flag_if_win 输出是否有终结手
 * @param winning_move 输出终结手的位置
 * @param flag_will_lose 输出是否有必堵的情况
 * @param blocking_move 输出必堵的位置
 * @return int 找到的点数量
 */
int FindPoints(CHESSSTATE** board, Board_info** Evaluated_Board, PTR_To_Points All_Points, int color, bool flag, bool *flag_if_win, PTR_To_Point winning_move, bool *flag_will_lose, PTR_To_Point blocking_move)
{
    int row, column;
    int index = 0;
    PTR_To_Point stored_points = (PTR_To_Point)malloc(sizeof(Point) * 225);
    if(stored_points == NULL){
        printf("Memory allocation error\n"); system("pause");
        exit(1);
    }
    int stored_index = 0;
    for(row = 0; row < SIZE; row++){
        for(column = 0; column < SIZE; column++){
            if(board[row][column] != none){
                int x1, y1, x2, y2;
                x1 = (row - 2 > 0)? row - 2 : 0;
                x2 = (row + 2 < SIZE)? row + 2 : SIZE - 1;
                y1 = (column - 2 > 0)? column - 2 : 0;
                y2 = (column + 2 < SIZE)? column + 2 : SIZE - 1;
                int i, j;
                for(i = x1; i <= x2; i++){
                    for(j = y1; j <= y2; j++){
                        if(board[i][j] == none){
                            bool already_evaluated = false;
                            for(int t = 0; t < stored_index; t++){
                                if(i == stored_points[t].row && j == stored_points[t].column){
                                    already_evaluated = true;
                                    break;
                                }
                            }
                            if(already_evaluated){  // 该点已判断过，跳过
                                continue;
                            }
                            else{
                                stored_points[stored_index].row = i;
                                stored_points[stored_index].column = j;
                                stored_index++;
                            }
                            
                            Point_Evaluate(board, Evaluated_Board, i, j, black);
                            Point_Evaluate(board, Evaluated_Board, i, j, white);

                            //board[i][j] = black;
                            //int is_ban = BanMove(board, i, j);
                            //board[i][j] = none;
                            bool is_ban = Check_Ban(Evaluated_Board, i, j, color);
                            if (color == black && !is_ban) continue;  //禁手点不能下

                            if(Evaluated_Board[i][j].direction[color - 1].right_5){   // 如果己方有终结手需要标记出来
                                winning_move->row = i;
                                winning_move->column = j;
                                *flag_if_win = true;
                                return ++index;
                            }
                            int opponent_color = (color == white ? black : white);
                            if(Evaluated_Board[i][j].direction[opponent_color - 1].right_5 && (opponent_color == white || is_ban) ){
                            	blocking_move->row = i;
                            	blocking_move->column = j;
                            	*flag_will_lose = true;
    						}
                            All_Points[index].position.row = i;
                            All_Points[index].position.column = j;
                            if(flag){
                                Evaluate_Point_Value(Evaluated_Board, i, j);
                                All_Points[index].score = Evaluated_Board[i][j].all_score;
                            }
                            index++;                            
                        }
                    }
                }
            }
        }
    }
    free(stored_points);
    return index;
}

/**
 * @brief 快速排序
 * 
 * @param s 待排序的点数组
 * @param left 左边界
 * @param right 右边界
 */
void quicksort(PTR_To_Points s, int left, int right){
    int i, j;
    if(left < right){
        i = left;
        j = right + 1;
        while(1){
            do{
                i++;
            }while(i < right && s[i].score <= s[left].score);
            do{
                j--;
            }while(j > left && s[j].score >= s[left].score);
            if(i < j){
                // 交换分数
                int temp_score = s[i].score;
                s[i].score = s[j].score;
                s[j].score = temp_score;
                // 交换位置
                Point temp_position = s[i].position;
                s[i].position = s[j].position;
                s[j].position = temp_position;
            }
            else{
                break;
            }
        }
        // 交换分数和位置
        int temp_score = s[j].score;
        s[j].score = s[left].score;
        s[left].score = temp_score;

        Point temp_position = s[j].position;
        s[j].position = s[left].position;
        s[left].position = temp_position;

        quicksort(s, j + 1, right);
        quicksort(s, left, j - 1);
    }
}

/**
 * @brief 查找基准点
 * 
 * @param board 当前棋盘状态
 * @param color 当前玩家颜色
 * @return int 评估的最大值
 */
int FindBasePoint(CHESSSTATE** board, Board_info** Evaluated_Board, int color){
    PTR_To_Points All_Points = (PTR_To_Points)malloc(sizeof(Points) * SIZE * SIZE);
    if(All_Points == NULL){
        printf("Memory allocation error\n"); system("pause");
        exit(1);
    }
    bool flag_if_win = false;
    bool flag_will_lose = false;
    Point winning_move, blocking_move;
    int n = FindPoints(board, Evaluated_Board, All_Points, color, 0, &flag_if_win, &winning_move, &flag_will_lose, &blocking_move);
    if(flag_if_win){
        // 说明底层轮己方下时有终结手，返回一个极大值
        free(All_Points);
        return 200000;
    }
    if(flag_will_lose){
    	// 说明底层轮己方下时只能去堵对面的，只有一种选择 
    	board[blocking_move.row][blocking_move.column] = color;
    	int only = Evaluate_Board_Value(board, Evaluated_Board, color);
    	board[blocking_move.row][blocking_move.column] = none; 
    	free(All_Points);
    	return only;
	}
    int i;
    int value, max = -100000;
    for(i = 0; i < n; i++){
        board[All_Points[i].position.row][All_Points[i].position.column] = color;
        value = Evaluate_Board_Value(board, Evaluated_Board, color);
        board[All_Points[i].position.row][All_Points[i].position.column] = none;
        if(value > max){
            max = value;
        }
    }
    free(All_Points);
    return max;
}

/**
 * @brief 选择最佳落子
 * 
 * @param board 当前棋盘状态
 * @param color 当前玩家颜色
 * @param final 最终落子点
 * @param depth 当前搜索深度
 * @param a Alpha 值
 * @param b Beta 值
 * @param able 难度模式
 * @return int 评估分数
 */
int BestChoice(CHESSSTATE** board, Board_info** Evaluated_Board, int color, PTR_To_Point final, int depth, int a, int b, char able){

    if (able != 'S' && able !='s'){  // 如果不是超级模式，则有思考限时
        clock_t current_time = clock();
        if (current_time - start_time > TimeLimit){    

            //printf("Reach time limit!\n"); system("pause");

            int value = FindBasePoint(board, Evaluated_Board, color);
            if (depth % 2 == 0) return value;
            else return -value;
        }
    }

    int WIDTH;
    if(able == 'J' || able == 'j' || able == 'M' || able == 'm'){
        WIDTH = BASE_WIDTH;
    }
    else{
        switch(depth){
            case 12:
                WIDTH = MAX_WIDTH;
                break;
            case 11:
                WIDTH = BASE_WIDTH;
                break;
            case 10:
                WIDTH = (able == 'A' || able == 'a')? MAX_WIDTH : BASE_WIDTH;
                break;
            case 9:
                WIDTH = (able == 'A' || able == 'a')? BASE_WIDTH : BASE_WIDTH;
                break;
            case 8:
                WIDTH = (able == 'A' || able == 'a')? BASE_WIDTH : BASE_WIDTH;
                break;
            default:
                WIDTH = (able == 'A' || able == 'a')? BASE_WIDTH : 10;
                break;
        } 
    }
    // 递归终止条件为到达底层，此时要根据局面分选点
    if(depth == 0){
        int value = FindBasePoint(board, Evaluated_Board, color);
        return value;
    }
    // 若非底层，则应该根据点分选点
    int next_color = (color == black ? white : black);   // 判断一下一步的棋子颜色
    PTR_To_Points All_Points = (PTR_To_Points)malloc(sizeof(Points) * 225);
    if(All_Points == NULL){
        printf("Memory allocation error\n"); system("pause");
        exit(1);
    }
    bool flag_if_win = false;
    bool flag_will_lose = false;
    Point winning_move, blocking_move;
    int n = FindPoints(board, Evaluated_Board, All_Points, color, 1, &flag_if_win, &winning_move, &flag_will_lose, &blocking_move);
    // 如果轮到color方，此步能终结比赛 
    if(flag_if_win){
        if(depth % 2 == 1){
            // 即己方落子后，发现对方下一步能终结比赛，说明己方该落子不合适，应返回一个极小值
            free(All_Points);
            return -200000;
        }
        else{
            // 即对方落子(或初始局面)时，我方下一步能终结比赛，说明对方该落子不合适，应返回一个极大值
            // 同时如果此时是根节点即初始局面，则应当返回己方终结手的坐标
            if(depth == DEPTH){
                final->row = winning_move.row;
                final->column = winning_move.column;
            }
            free(All_Points);
            return 200000;
        }
    }
	if(n < WIDTH){   // 即可找的点少于WIDTH个 
		WIDTH = n; 
	}
	int temp_width = WIDTH;
	
    quicksort(All_Points, 0, n - 1);
    int i, j;
    PTR_To_Points choice = (PTR_To_Points)malloc(sizeof(Points) * WIDTH);
    if(choice == NULL){
        printf("Memory allocation error\n"); system("pause");
        exit(1);
    }
    // 将All_Points中的最优的WIDTH个点复制到choice[]中去
    for(i = 0, j = n - 1; i < WIDTH; i++, j--){
        choice[i].position = All_Points[j].position;
        choice[i].score = All_Points[j].score;
        choice[i].min = a;    // 继承来自父节点的α
        choice[i].max = b;    // 继承来自父节点的β

        //printf("No.%d,(%d, %d),Score:%d, min:%d, max:%d\n", i, choice[i].position.row, choice[i].position.column, choice[i].score, choice[i].min, choice[i].max);
    }

    int *value = (int *)malloc(sizeof(int) * WIDTH);  // 记录每个子节点的回推值
    if(value == NULL){
        printf("Memory allocation error\n"); system("pause");
        exit(1);
    }

    // 如果在顶层，并行处理多个分支以最大化效率
    if(depth == DEPTH){
        // 当在顶层深度时(或任意深度都可以)，对候选点进行并行处理
        // 使用OpenMP并行加速
        // 每个线程处理一个候选点的落子、递归搜索和恢复操作
        CHESSSTATE*** tempboard;               //为每个进程创建棋盘副本
        tempboard = malloc(sizeof(CHESSSTATE**) * temp_width); //分配并行内存
        if(tempboard == NULL){
            printf("Memory allocation error\n"); system("pause");
            exit(1);
        }
        for (int i = 0; i < temp_width; i++){   
            tempboard[i] = malloc(sizeof(CHESSSTATE*) * SIZE);
            for (int j = 0; j < SIZE; j++)                 //分配第一列的每行内存
            {
                tempboard[i][j] = malloc(sizeof(CHESSSTATE) * SIZE);
                if(tempboard[i][j] == NULL){
                    printf("Memory allocation error\n"); system("pause");
                    exit(1);
                }
                for (int k = 0; k < SIZE; k++)
                    tempboard[i][j][k] = board[j][k];  //拷贝
            }                            
        }
        //Remake_Evaluated_Board(0,0,0); 
        printf("       ╭━━━━━━━━━━━━━━━━╮\n");
        printf("       ┃                ┃\n");
        printf("       ╰━━━━━━━━━━━━━━━━╯\n");
        COORD pos;
        pos.X = 8; //设置光标x为5
        pos.Y = 21; //设置光标y为5
        SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE),pos);

        #pragma omp parallel for schedule(dynamic) firstprivate(temp_width, color, choice, able, a, b, depth, next_color) shared(value, tempboard, start_time) default(none)
        for(i = 0; i < temp_width; i++){   

            tempboard[i][choice[i].position.row][choice[i].position.column] = color;
            
            if (color == black && BanMove(tempboard[i], choice[i].position.row, choice[i].position.column) != 0){
                // 释放临时棋盘
                for (int j = 0; j < SIZE; j++) {
                    free(tempboard[i][j]);
                }
                free(tempboard[i]);
                for (int j = 0; j < MAX_WIDTH / temp_width; j++) printf("%c%c", 0xa8, 0x80);  // 打印进度条
                value[i] = -INF;
                continue;
            }

            //printf("No.%d Thread(%d) started.\n", i, omp_get_thread_num());

            //if (i==5) Evaluated_Board[0][0].score[0] = 1;
            //printf("EvaluateBoard[0][0]:%d  ",Evaluated_Board[0][0].score[0]);
            //printf("No.%d : Point(%c,%d) Value = %d | ", i, 'A'+choice[i].position.column, 1+ choice[i].position.row, value[i]);

            Board_info** temp_Evaluated_Board;  //每个进程单独创建一个棋盘评估的副本
            temp_Evaluated_Board = (Board_info**)malloc(SIZE * sizeof(Board_info*));
            if (temp_Evaluated_Board == NULL) {
                printf("Memory allocation error\n"); system("pause");
                exit(1);
            }
            for (int j = 0; j < SIZE; j++) {
                temp_Evaluated_Board[j] = (Board_info*)malloc(SIZE * sizeof(Board_info));
                if (temp_Evaluated_Board[j] == NULL) {
                    printf("Memory allocation error\n"); system("pause");
                    exit(1);
                }
            }
            // 为每个线程使用独立的final_move变量，避免冲突
            Point thread_final;
            int tempvalue = BestChoice(tempboard[i], temp_Evaluated_Board, next_color, &thread_final, depth-1, a, b, able);
            //#pragma omp critical
            //{
                value[i] = tempvalue; // 对每个候选点进行计算
            //}            

            // 释放临时棋盘
            for (int j = 0; j < SIZE; j++) {
                free(tempboard[i][j]); free(temp_Evaluated_Board[j]);
            }
            free(tempboard[i]); free(temp_Evaluated_Board);

            //printf("No.%d Thread(%d) is finished.Value = %d\n", i, omp_get_thread_num(), value[i]);

            //printf("No.%d : Point(%c,%d) Value = %d\n", i, 'A'+choice[i].position.column, 1+ choice[i].position.row, value[i]);

            for (int j = 0; j < MAX_WIDTH / temp_width; j++) printf("%c%c", 0xa8, 0x80);  // 打印进度条
        }

        for (int j = 0; j < MAX_WIDTH - MAX_WIDTH / temp_width * temp_width; j++) printf("%c%c", 0xa8, 0x80);

        //system("pause");

        for(i = 0; i < temp_width; i++){
            // 根据回推值改变α和β
            // 如果是偶数层(己方落子, 取子节点回推值中的最大值，因此改变下限a)
            if(depth % 2 == 0){
                if(value[i] > a){
                    a = value[i];        // 若比下限大，则改变下限
                    // 顶层根节点时偶数层，需要知道落点位置 
                    final->row = choice[i].position.row;
                    final->column = choice[i].position.column;             

                    /*if (final->row == 7 && final->column == 10) {
                        printf("Ban:%d\n",Check_Ban(Evaluated_Board, final->row, final->column, color));                        
                        printf("living2:%d\n",Evaluated_Board[final->row][final->column].direction[color-1].living_2);
                        printf("living3:%d\n",Evaluated_Board[final->row][final->column].direction[color-1].living_3);
                        printf("living4:%d\n",Evaluated_Board[final->row][final->column].direction[color-1].living_4);
                        printf("right5:%d\n",Evaluated_Board[final->row][final->column].direction[color-1].right_5);
                        printf("rush2:%d\n",Evaluated_Board[final->row][final->column].direction[color-1].rush_2);
                        printf("rush3:%d\n",Evaluated_Board[final->row][final->column].direction[color-1].rush_3);
                        printf("rush4:%d\n",Evaluated_Board[final->row][final->column].direction[color-1].rush_4);
                        printf("too_long:%d\n",Evaluated_Board[final->row][final->column].direction[color-1].too_long);
                        system("pause");
                    }*/

                }
                if(a >= b){    // 即α>=β，不用再往下搜索了，本层给上层返回α
                    free(choice);
                    free(value);
                    free(All_Points);
                    return a;
                }
            }
            // 如果是奇数层(对方落子，取子节点回推值中的最小值，因此改变上限b)
            else{
                if(value[i] < b){
                    b = value[i];
                }
                if(a >= b){    // 即α>=β，不用往下搜索了，本层给上层返回b 
                    free(choice);
                    free(value);
                    free(All_Points);
                    return b;
                }
            }
        }
        free(choice);
        free(value);
        free(All_Points);
        if(depth % 2 == 0){
            return a;
        }
        else{
            return b;
        }
    }

    for(i = 0; i < temp_width; i++){
        // Step 1: 先把子落上去

        /*int hang = choice[i].position.row;
        int lie = choice[i].position.column;
        if (hang < 0 || hang > SIZE - 1 || lie < 0 || lie > SIZE - 1) 
        {
            printf("(%d, %d)\n",hang,lie);
            system("pause"); continue;
        }*/
        
        board[choice[i].position.row][choice[i].position.column] = color;

        // Step 2: 根据下面的值回推回来
        value[i] = BestChoice(board, Evaluated_Board, next_color, final, depth - 1, a, b, able);
        // Step 3: 根据回推值改变α和β
        // 如果是偶数层(己方落子, 取子节点回推值中的最大值，因此改变下限a)
        if(depth % 2 == 0){
            if(value[i] > a){
                a = value[i];        // 若比下限大，则改变下限
                // 顶层根节点时偶数层，需要知道落点位置
                if(depth == DEPTH){     
                    final->row = choice[i].position.row;
                    final->column = choice[i].position.column;
                }
            }
            if(a >= b){    // 即α>=β，不用再往下搜索了，本层给上层返回α
            	board[choice[i].position.row][choice[i].position.column] = none;
                free(choice);
                free(value);
                free(All_Points);
                return a;
            }
        }
        // 如果是奇数层(对方落子，取子节点回推值中的最小值，因此改变上限b)
        else{
            if(value[i] < b){
                b = value[i];
            }
            if(a >= b){    // 即α>=β，不用往下搜索了，本层给上层返回b 
            	board[choice[i].position.row][choice[i].position.column] = none;
                free(choice);
                free(value);
                free(All_Points);
                return b;
            }
        }
        // Step 4: 把落的子擦除
        board[choice[i].position.row][choice[i].position.column] = none;
    }
    free(choice);
    free(value);
    free(All_Points);
    if(depth % 2 == 0){
        return a;
    }
    else{
        return b;
    }
}

/**
 * @brief AI 的主函数，用于选择最佳落子
 * 
 * @param board 当前棋盘状态，二维数组
 * @param machine_color AI 的颜色（black 或 white）
 * @param able 难度模式（'J' / 'j' 表示初级，'M' / 'm' 表示中级，'A' / 'a' 表示高级，'S' / 's' 表示超级）
 * @param final_move 输出的最佳落子位置
 */
int AI_Play(CHESSSTATE** board, CHESSSTATE machine_color, char able, MovePoint* final_move)
{
    if (machine_color == black && count < 5){ // 开局阵法
        for (int i = 0; i < 7; i++)
            for (int j = 0; j < 8; j++){
                bool is_match = true;
                for (int k = 0; k < count; k += 2){
                    if (board[BlackOpeningPatterns[i][j][k].row][BlackOpeningPatterns[i][j][k].column] != black){
                        is_match = false; break;
                    }                        
                }
                for (int k = 1; k < count; k += 2){
                    if (board[BlackOpeningPatterns[i][j][k].row][BlackOpeningPatterns[i][j][k].column] != white){
                        is_match = false; break;
                    }                        
                }
                if (is_match){
                    final_move->row = BlackOpeningPatterns[i][j][count].row;
                    final_move->col = BlackOpeningPatterns[i][j][count].column;
                    board[final_move->row][final_move->col] = machine_color;
                    return 10001;
                }
            }
    }
    if (machine_color == white && count < 4){ // 开局阵法
        for (int i = 0; i < 12; i++)
            for (int j = 0; j < 8; j++){
                bool is_match = true;
                for (int k = 0; k < count; k += 2){
                    if (board[WhiteOpeningPatterns[i][j][k].row][WhiteOpeningPatterns[i][j][k].column] != black){
                        is_match = false; break;
                    }                        
                }
                for (int k = 1; k < count; k += 2){
                    if (board[WhiteOpeningPatterns[i][j][k].row][WhiteOpeningPatterns[i][j][k].column] != white){
                        is_match = false; break;
                    }                        
                }
                if (is_match){
                    final_move->row = WhiteOpeningPatterns[i][j][count].row;
                    final_move->col = WhiteOpeningPatterns[i][j][count].column;
                    board[final_move->row][final_move->col] = machine_color;
                    return 0;
                }
            }
    }
    if(able=='J' || able=='j'){
		DEPTH = DEPTH_EASY;
	}
	else if(able=='M' || able=='m'){
		DEPTH = DEPTH_MEDIUM;
	}
    else if(able=='A' || able=='a' || able=='S' || able=='s'){
		DEPTH = DEPTH_HARD;
	}
    PTR_To_Point final = (PTR_To_Point)malloc(sizeof(Point));
    if(final == NULL){
        printf("Memory allocation error\n"); system("pause");
        exit(1);
    }

    Board_info** Evaluated_Board;  //创建一个棋盘评估的数组
    Evaluated_Board = (Board_info**)malloc(SIZE * sizeof(Board_info*));
    if (Evaluated_Board == NULL) {
        printf("Memory allocation error\n"); system("pause");
        exit(1);
    }
    for (int j = 0; j < SIZE; j++) {
        Evaluated_Board[j] = (Board_info*)malloc(SIZE * sizeof(Board_info));
        if (Evaluated_Board[j] == NULL) {
            printf("Memory allocation error\n"); system("pause");
            exit(1);
        }
    }
    int value = BestChoice(board, Evaluated_Board, machine_color, final, DEPTH, -INF, INF, able);
    final_move->row = final->row;
    final_move->col = final->column;
    board[final_move->row][final_move->col] = machine_color;

    free(final);
    return value;
}

/**
 * @brief 评估棋子的价值
 * 
 * @param row 行坐标
 * @param column 列坐标
 */
void Evaluate_Point_Value(Board_info** Evaluated_Board, int row, int column){
    int i;
    int value[2] = {0, 0};
    for(i = 0; i < 2; i++){
        value[i] += Evaluated_Board[row][column].direction[i].right_5 * VALUE_RIGHT_5;
        value[i] += Evaluated_Board[row][column].direction[i].living_4 * VALUE_LIVING_4;
        value[i] += Evaluated_Board[row][column].direction[i].rush_4 * VALUE_RUSH_4;
        value[i] += Evaluated_Board[row][column].direction[i].living_3 * VALUE_LIVING_3;
        value[i] += Evaluated_Board[row][column].direction[i].rush_3 * VALUE_RUSH_3;
        value[i] += Evaluated_Board[row][column].direction[i].living_2 * VALUE_LIVING_2;
        value[i] += Evaluated_Board[row][column].direction[i].rush_2 * VALUE_RUSH_2;
        Evaluated_Board[row][column].score[i] = value[i];
    }
    Evaluated_Board[row][column].all_score = value[0] + value[1];
}

/*------------------------------end------------------------------*/
