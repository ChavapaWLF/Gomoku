/**
 * @file     check.c
 * @brief    禁手规则检查文件
 * @details  负责在棋盘上检查各类禁手规则
 */

#include "check.h"

/*-------------------------------------
 *  Function Prototypes
 *-------------------------------------*/

/**
 * @brief 长连禁手检查
 * @param[in] board 棋盘的二维指针
 * @param[in] row   要检查点的行坐标
 * @param[in] col   要检查点的列坐标
 * @return 0 表示无长连禁手，1 表示有长连禁手
 */
int CheckLongChain(CHESSSTATE** board, int row, int col);

/**
 * @brief 四四禁手检查
 * @param[in] board 棋盘的二维指针
 * @param[in] row   要检查点的行坐标
 * @param[in] col   要检查点的列坐标
 * @return 0 表示无四四禁手，1 表示有四四禁手
 */
int CheckFourInRow(CHESSSTATE** board, int row, int col);

/**
 * @brief 在特定方向上统计“四”的模式数量
 * @param[in] board   棋盘的二维指针
 * @param[in] dirState 当前方向的棋盘字符串
 * @param[in] base_x   当前方向起始行坐标（扫描左边/上边的起始点）
 * @param[in] base_y   当前方向起始列坐标
 * @param[in] dx       行方向增量
 * @param[in] dy       列方向增量
 * @return 检测到的有效“四”模式数量
 */
int CountFourPatterns(CHESSSTATE** board, char* dirState, int base_x, int base_y, int dx, int dy);

/**
 * @brief 三三禁手检查
 * @param[in] board 棋盘的二维指针
 * @param[in] row   要检查点的行坐标
 * @param[in] col   要检查点的列坐标
 * @return 0 表示无三三禁手，1 表示有三三禁手
 */
int CheckThreeInRow(CHESSSTATE** board, int row, int col);

/**
 * @brief 检查是否产生禁手
 * @param[in] board 棋盘的二维指针
 * @param[in] row   要检查点的行坐标
 * @param[in] col   要检查点的列坐标
 * @retval 0 无禁手
 * @retval 1 三三禁手
 * @retval 2 四四禁手
 * @retval 3 长连禁手
 */
int BanMove(CHESSSTATE** board, int row, int col);

/*-------------------------------------
 *  Macro for boundary checking
 *-------------------------------------*/
#define IN_RANGE(x, y) ((x) >= 0 && (x) < ROW && (y) >= 0 && (y) < COLUMN)

/*-------------------------------------
 *  Functions Implementations
 *-------------------------------------*/

/**
 * @brief 检查长连禁手（6颗及以上连续黑棋即为长连）
 * @param[in] board 棋盘的二维指针
 * @param[in] row   要检查点的行坐标
 * @param[in] col   要检查点的列坐标
 * @return 0 表示无长连禁手，1 表示有长连禁手
 */
int CheckLongChain(CHESSSTATE** board, int row, int col)
{
    int currentX, currentY;
    int i, consecutiveCount;

    /* '-' 横向检查 */
    currentX = row;          
    currentY = col - 4;   
    consecutiveCount = 0;
    for (i = 0; i < 9; i++, currentY++) {
        if (currentY < 0 || currentY >= COLUMN) {
            continue;
        }
        if (board[currentX][currentY] == black) {
            consecutiveCount++;
        } else {
            consecutiveCount = 0;
        }
        if (consecutiveCount == 6) {
            return 1;
        }
    }

    /* '|' 竖向检查 */
    currentX = row - 4;     
    currentY = col;      
    consecutiveCount = 0;
    for (i = 0; i < 9; i++, currentX++) {
        if (currentX < 0 || currentX >= ROW) {
            continue;
        }
        if (board[currentX][currentY] == black) {
            consecutiveCount++;
        } else {
            consecutiveCount = 0;
        }
        if (consecutiveCount == 6) {
            return 1;
        }
    }

    /* '/' 斜杠方向检查 */
    currentX = row - 4;       
    currentY = col + 4;    
    consecutiveCount = 0;
    for (i = 0; i < 9; i++, currentX++, currentY--) {
        if (currentX < 0 || currentX >= ROW || currentY < 0 || currentY >= COLUMN) {
            continue;
        }
        if (board[currentX][currentY] == black) {
            consecutiveCount++;
        } else {
            consecutiveCount = 0;
        }
        if (consecutiveCount == 6) {
            return 1;
        }
    }

    /* '\' 反斜杠方向检查 */
    currentX = row - 4;       
    currentY = col - 4;    
    consecutiveCount = 0;
    for (i = 0; i < 9; i++, currentX++, currentY++) {
        if (currentX < 0 || currentX >= ROW || currentY < 0 || currentY >= COLUMN) {
            continue;
        }
        if (board[currentX][currentY] == black) {
            consecutiveCount++;
        } else {
            consecutiveCount = 0;
        }
        if (consecutiveCount == 6) {
            return 1;
        }
    }

    return 0;
}

/*-------------------------------------*/

int CountFourPatterns(CHESSSTATE** board, char* dirState, int base_x, int base_y, int dx, int dy)
{
    /**
     * @note 
     * patterns[] 中存储的是可能形成“四”的子串模式（例如 "011110" 等），
     * 这里用 '0' 表示可能再次落黑棋的位置。
     */
    const char* patterns[] = {
        "011110", "10111", "11011", "11101",
        "011112", "011114", "211110", "411110"
    };
    int patternCount = sizeof(patterns) / sizeof(patterns[0]);
    int totalValidFours = 0;

    for(int pIndex = 0; pIndex < patternCount; pIndex++) {
        const char* pat = patterns[pIndex];
        const char* pos  = dirState;

        // 寻找所有匹配到 pat 的位置
        while((pos = strstr(pos, pat)) != NULL) {
            int offset   = (int)(pos - dirState);
            int pat_len  = (int)strlen(pat);

            // 尝试在匹配到的模式中每个 '0' 处落子，检查是否真的能形成有效“四”
            for(int idx = 0; idx < pat_len; idx++) {
                if(pat[idx] == '0') {
                    int testX = base_x + (offset + idx) * dx;
                    int testY = base_y + (offset + idx) * dy;

                    if(IN_RANGE(testX, testY) && board[testX][testY] == none) {
                        CHESSSTATE backup = board[testX][testY];
                        board[testX][testY] = black;  // 临时落子

                        int banStatus = BanMove(board, testX, testY);

                        // 恢复状态
                        board[testX][testY] = backup;

                        if(banStatus == 0) {
                            // 如果此落子点不构成任何禁手，则有效“四”计数加 1
                            totalValidFours++;
                            break; // 同一个模式下只要有一个 '0' 能产生有效“四”，就算一次
                        }
                    }
                }
            }
            // 检查下一个可能的匹配位置
            pos += 1;
        }
    }

    return totalValidFours;
}

/*-------------------------------------*/

/**
 * @brief 四四禁手检查
 * @param[in] board 棋盘的二维指针
 * @param[in] row   要检查点的行坐标
 * @param[in] col   要检查点的列坐标
 * @return 0 表示无四四禁手，1 表示有四四禁手
 */
int CheckFourInRow(CHESSSTATE** board, int row, int col)
{
    char dirState[10];
    int fourCount = 0;

    // 四个方向
    int directions[4][2] = {
        {0, 1},   // 横向
        {1, 0},   // 竖向
        {1, -1},  // 斜向 '/'
        {1, 1}    // 反斜向 '\'
    };

    // 分别在四个方向上统计“四”的模式数量
    for(int d = 0; d < 4; d++) {
        int dx = directions[d][0];
        int dy = directions[d][1];

        // 初始化 dirState 为 '4'（表示超出边界）
        memset(dirState, '4', sizeof(dirState));
        for(int i = 0; i < 9; i++) {
            int currentX = row + (i - 4) * dx;
            int currentY = col + (i - 4) * dy;
            if(IN_RANGE(currentX, currentY)) {
                dirState[i] = board[currentX][currentY] + '0';
            }
        }

        // 计算基准点 base_x / base_y
        int base_x = row - 4 * dx;
        int base_y = col - 4 * dy;

        // 调用辅助函数 CountFourPatterns
        fourCount += CountFourPatterns(board, dirState, base_x, base_y, dx, dy);
    }

    // 如果四的数量超过 1，说明出现了四四禁手
    return (fourCount > 1) ? 1 : 0;
}

/*-------------------------------------*/

/**
 * @brief 三三禁手检查
 * @param[in] board 棋盘的二维指针
 * @param[in] row   要检查点的行坐标
 * @param[in] col   要检查点的列坐标
 * @return 0 表示无三三禁手，1 表示有三三禁手
 */
int CheckThreeInRow(CHESSSTATE** board, int row, int col)
{
    // 'dirState' 用来存储当前方向周围 9 个格子的棋盘状态：
    // '0' 表示空，'1' 表示黑棋，'2' 表示白棋，'4' 表示越界（无棋盘）。
    char dirState[10];
    int i, currentX, currentY, patternPos;
    int threeCount = 0;
    int isPseudoLiveThree;  // 原 fakeflag

    /*-------------------------------------
     * '-' 横向方向三三检查
     *-------------------------------------*/
    currentX = row;
    currentY = col - 4;
    strcpy_s(dirState, sizeof(dirState), "444444444");
    for (i = 0; i < 9; i++, currentY++) {
        if (currentY < 0 || currentY >= COLUMN) {
            continue;
        }
        dirState[i] = board[currentX][currentY] + '0';
    }

    // 检查 "活三" => "01110"
    patternPos = (strstr(dirState, "01110") != NULL) 
                 ? (int)(strstr(dirState, "01110") - dirState) 
                 : -1;
    isPseudoLiveThree = 0;

    if (patternPos >= 0 && patternPos <= 4) {
        // 放置第一个 '0' 为黑棋
        if (IN_RANGE(row, col - 4 + patternPos)) {
            CHESSSTATE backup1 = board[row][col - 4 + patternPos];
            board[row][col - 4 + patternPos] = black;
            if (BanMove(board, row, col - 4 + patternPos) != 0) {
                isPseudoLiveThree = 1;
            }
            board[row][col - 4 + patternPos] = backup1; // 恢复
        } else {
            isPseudoLiveThree = 1;
        }

        // 放置第二个 '0' 为黑棋
        if (IN_RANGE(row, col + patternPos)) {
            CHESSSTATE backup2 = board[row][col + patternPos];
            board[row][col + patternPos] = black;
            if (BanMove(board, row, col + patternPos) != 0) {
                isPseudoLiveThree = 1;
            }
            board[row][col + patternPos] = backup2; // 恢复
        } else {
            isPseudoLiveThree = 1;
        }

        // 检查 "01110" 左侧是否为空
        if (!isPseudoLiveThree &&
            IN_RANGE(row, col - 5 + patternPos) &&
            board[row][col - 5 + patternPos] == none)
        {
            CHESSSTATE backup3 = board[row][col - 5 + patternPos];
            board[row][col - 5 + patternPos] = black;
            if (BanMove(board, row, col - 5 + patternPos) == 0) {
                threeCount++;
            }
            board[row][col - 5 + patternPos] = backup3;
        }
        // 检查 "01110" 右侧是否为空
        else if (!isPseudoLiveThree &&
                 IN_RANGE(row, col + 1 + patternPos) &&
                 board[row][col + 1 + patternPos] == none)
        {
            CHESSSTATE backup4 = board[row][col + 1 + patternPos];
            board[row][col + 1 + patternPos] = black;
            if (BanMove(board, row, col + 1 + patternPos) == 0) {
                threeCount++;
            }
            board[row][col + 1 + patternPos] = backup4;
        }
    }

    // 检查 "跳三" => "010110"
    patternPos = (strstr(dirState, "010110") != NULL)
                 ? (int)(strstr(dirState, "010110") - dirState)
                 : -1;
    if (patternPos >= 0 && patternPos <= 3) {
        if (IN_RANGE(row, col - 2 + patternPos) &&
            board[row][col - 2 + patternPos] == none)
        {
            CHESSSTATE backup5 = board[row][col - 2 + patternPos];
            board[row][col - 2 + patternPos] = black;
            if (BanMove(board, row, col - 2 + patternPos) == 0) {
                threeCount++;
            }
            board[row][col - 2 + patternPos] = backup5;
        }
    }

    // 检查 "跳三" => "011010"
    patternPos = (strstr(dirState, "011010") != NULL)
                 ? (int)(strstr(dirState, "011010") - dirState)
                 : -1;
    if (patternPos >= 0 && patternPos <= 4) {
        if (IN_RANGE(row, col - 1 + patternPos) &&
            board[row][col - 1 + patternPos] == none)
        {
            CHESSSTATE backup6 = board[row][col - 1 + patternPos];
            board[row][col - 1 + patternPos] = black;
            if (BanMove(board, row, col - 1 + patternPos) == 0) {
                threeCount++;
            }
            board[row][col - 1 + patternPos] = backup6;
        }
    }

    /*-------------------------------------
     * '|' 竖向方向三三检查
     *-------------------------------------*/
    currentX = row - 4;
    currentY = col;
    strcpy_s(dirState, sizeof(dirState), "444444444");
    for (i = 0; i < 9; i++, currentX++) {
        if (currentX < 0 || currentX >= ROW) {
            continue;
        }
        dirState[i] = board[currentX][currentY] + '0';
    }

    // 重复类似的 "活三" / "跳三" 检查逻辑（同上）
    patternPos = (strstr(dirState, "01110") != NULL)
                 ? (int)(strstr(dirState, "01110") - dirState)
                 : -1;
    isPseudoLiveThree = 0;

    if (patternPos >= 0 && patternPos <= 4) {
        if (IN_RANGE(row - 4 + patternPos, col)) {
            CHESSSTATE backup1 = board[row - 4 + patternPos][col];
            board[row - 4 + patternPos][col] = black;
            if (BanMove(board, row - 4 + patternPos, col) != 0) {
                isPseudoLiveThree = 1;
            }
            board[row - 4 + patternPos][col] = backup1;
        } else {
            isPseudoLiveThree = 1;
        }

        if (IN_RANGE(row + patternPos, col)) {
            CHESSSTATE backup2 = board[row + patternPos][col];
            board[row + patternPos][col] = black;
            if (BanMove(board, row + patternPos, col) != 0) {
                isPseudoLiveThree = 1;
            }
            board[row + patternPos][col] = backup2;
        } else {
            isPseudoLiveThree = 1;
        }

        if (!isPseudoLiveThree &&
            IN_RANGE(row - 5 + patternPos, col) &&
            board[row - 5 + patternPos][col] == none)
        {
            CHESSSTATE backup3 = board[row - 5 + patternPos][col];
            board[row - 5 + patternPos][col] = black;
            if (BanMove(board, row - 5 + patternPos, col) == 0) {
                threeCount++;
            }
            board[row - 5 + patternPos][col] = backup3;
        }
        else if (!isPseudoLiveThree &&
                 IN_RANGE(row + 1 + patternPos, col) &&
                 board[row + 1 + patternPos][col] == none)
        {
            CHESSSTATE backup4 = board[row + 1 + patternPos][col];
            board[row + 1 + patternPos][col] = black;
            if (BanMove(board, row + 1 + patternPos, col) == 0) {
                threeCount++;
            }
            board[row + 1 + patternPos][col] = backup4;
        }
    }

    patternPos = (strstr(dirState, "010110") != NULL)
                 ? (int)(strstr(dirState, "010110") - dirState)
                 : -1;
    if (patternPos >= 0 && patternPos <= 3) {
        if (IN_RANGE(row - 2 + patternPos, col) &&
            board[row - 2 + patternPos][col] == none)
        {
            CHESSSTATE backup5 = board[row - 2 + patternPos][col];
            board[row - 2 + patternPos][col] = black;
            if (BanMove(board, row - 2 + patternPos, col) == 0) {
                threeCount++;
            }
            board[row - 2 + patternPos][col] = backup5;
        }
    }

    patternPos = (strstr(dirState, "011010") != NULL)
                 ? (int)(strstr(dirState, "011010") - dirState)
                 : -1;
    if (patternPos >= 0 && patternPos <= 4) {
        if (IN_RANGE(row - 1 + patternPos, col) &&
            board[row - 1 + patternPos][col] == none)
        {
            CHESSSTATE backup6 = board[row - 1 + patternPos][col];
            board[row - 1 + patternPos][col] = black;
            if (BanMove(board, row - 1 + patternPos, col) == 0) {
                threeCount++;
            }
            board[row - 1 + patternPos][col] = backup6;
        }
    }

    /*-------------------------------------
     * '/' 斜杠方向三三检查
     *-------------------------------------*/
    currentX = row - 4;
    currentY = col + 4;
    strcpy_s(dirState, sizeof(dirState), "444444444");
    for (i = 0; i < 9; i++, currentX++, currentY--) {
        if (currentX < 0 || currentX >= ROW || currentY < 0 || currentY >= COLUMN) {
            continue;
        }
        dirState[i] = board[currentX][currentY] + '0';
    }

    // 同样重复 "01110" / "010110" / "011010" 检查...
    patternPos = (strstr(dirState, "01110") != NULL)
                 ? (int)(strstr(dirState, "01110") - dirState)
                 : -1;
    isPseudoLiveThree = 0;

    if (patternPos >= 0 && patternPos <= 4) {
        if (IN_RANGE(row - 4 + patternPos, col + 4 - patternPos)) {
            CHESSSTATE backup1 = board[row - 4 + patternPos][col + 4 - patternPos];
            board[row - 4 + patternPos][col + 4 - patternPos] = black;
            if (BanMove(board, row - 4 + patternPos, col + 4 - patternPos) != 0) {
                isPseudoLiveThree = 1;
            }
            board[row - 4 + patternPos][col + 4 - patternPos] = backup1;
        } else {
            isPseudoLiveThree = 1;
        }

        if (IN_RANGE(row + patternPos, col - patternPos)) {
            CHESSSTATE backup2 = board[row + patternPos][col - patternPos];
            board[row + patternPos][col - patternPos] = black;
            if (BanMove(board, row + patternPos, col - patternPos) != 0) {
                isPseudoLiveThree = 1;
            }
            board[row + patternPos][col - patternPos] = backup2;
        } else {
            isPseudoLiveThree = 1;
        }

        if (!isPseudoLiveThree &&
            IN_RANGE(row - 5 + patternPos, col + 5 - patternPos) &&
            board[row - 5 + patternPos][col + 5 - patternPos] == none)
        {
            CHESSSTATE backup3 = board[row - 5 + patternPos][col + 5 - patternPos];
            board[row - 5 + patternPos][col + 5 - patternPos] = black;
            if (BanMove(board, row - 5 + patternPos, col + 5 - patternPos) == 0) {
                threeCount++;
            }
            board[row - 5 + patternPos][col + 5 - patternPos] = backup3;
        }
        else if (!isPseudoLiveThree &&
                 IN_RANGE(row + 1 + patternPos, col - 1 - patternPos) &&
                 board[row + 1 + patternPos][col - 1 - patternPos] == none)
        {
            CHESSSTATE backup4 = board[row + 1 + patternPos][col - 1 - patternPos];
            board[row + 1 + patternPos][col - 1 - patternPos] = black;
            if (BanMove(board, row + 1 + patternPos, col - 1 - patternPos) == 0) {
                threeCount++;
            }
            board[row + 1 + patternPos][col - 1 - patternPos] = backup4;
        }
    }

    patternPos = (strstr(dirState, "010110") != NULL)
                 ? (int)(strstr(dirState, "010110") - dirState)
                 : -1;
    if (patternPos >= 0 && patternPos <= 3) {
        if (IN_RANGE(row - 2 + patternPos, col + 2 - patternPos) &&
            board[row - 2 + patternPos][col + 2 - patternPos] == none)
        {
            CHESSSTATE backup5 = board[row - 2 + patternPos][col + 2 - patternPos];
            board[row - 2 + patternPos][col + 2 - patternPos] = black;
            if (BanMove(board, row - 2 + patternPos, col + 2 - patternPos) == 0) {
                threeCount++;
            }
            board[row - 2 + patternPos][col + 2 - patternPos] = backup5;
        }
    }

    patternPos = (strstr(dirState, "011010") != NULL)
                 ? (int)(strstr(dirState, "011010") - dirState)
                 : -1;
    if (patternPos >= 0 && patternPos <= 4) {
        if (IN_RANGE(row - 1 + patternPos, col + 1 - patternPos) &&
            board[row - 1 + patternPos][col + 1 - patternPos] == none)
        {
            CHESSSTATE backup6 = board[row - 1 + patternPos][col + 1 - patternPos];
            board[row - 1 + patternPos][col + 1 - patternPos] = black;
            if (BanMove(board, row - 1 + patternPos, col + 1 - patternPos) == 0) {
                threeCount++;
            }
            board[row - 1 + patternPos][col + 1 - patternPos] = backup6;
        }
    }

    /*-------------------------------------
     * '\' 反斜杠方向三三检查
     *-------------------------------------*/
    currentX = row - 4;
    currentY = col - 4;
    strcpy_s(dirState, sizeof(dirState), "444444444");
    for (i = 0; i < 9; i++, currentX++, currentY++) {
        if (currentX < 0 || currentX >= ROW || currentY < 0 || currentY >= COLUMN) {
            continue;
        }
        dirState[i] = board[currentX][currentY] + '0';
    }

    // 同样重复 "01110" / "010110" / "011010" 检查...
    patternPos = (strstr(dirState, "01110") != NULL)
                 ? (int)(strstr(dirState, "01110") - dirState)
                 : -1;
    isPseudoLiveThree = 0;

    if (patternPos >= 0 && patternPos <= 4) {
        if (IN_RANGE(row - 4 + patternPos, col - 4 + patternPos)) {
            CHESSSTATE backup1 = board[row - 4 + patternPos][col - 4 + patternPos];
            board[row - 4 + patternPos][col - 4 + patternPos] = black;
            if (BanMove(board, row - 4 + patternPos, col - 4 + patternPos) != 0) {
                isPseudoLiveThree = 1;
            }
            board[row - 4 + patternPos][col - 4 + patternPos] = backup1;
        } else {
            isPseudoLiveThree = 1;
        }

        if (IN_RANGE(row + patternPos, col + patternPos)) {
            CHESSSTATE backup2 = board[row + patternPos][col + patternPos];
            board[row + patternPos][col + patternPos] = black;
            if (BanMove(board, row + patternPos, col + patternPos) != 0) {
                isPseudoLiveThree = 1;
            }
            board[row + patternPos][col + patternPos] = backup2;
        } else {
            isPseudoLiveThree = 1;
        }

        if (!isPseudoLiveThree &&
            IN_RANGE(row - 5 + patternPos, col - 5 + patternPos) &&
            board[row - 5 + patternPos][col - 5 + patternPos] == none)
        {
            CHESSSTATE backup3 = board[row - 5 + patternPos][col - 5 + patternPos];
            board[row - 5 + patternPos][col - 5 + patternPos] = black;
            if (BanMove(board, row - 5 + patternPos, col - 5 + patternPos) == 0) {
                threeCount++;
            }
            board[row - 5 + patternPos][col - 5 + patternPos] = backup3;
        }
        else if (!isPseudoLiveThree &&
                 IN_RANGE(row + 1 + patternPos, col + 1 + patternPos) &&
                 board[row + 1 + patternPos][col + 1 + patternPos] == none)
        {
            CHESSSTATE backup4 = board[row + 1 + patternPos][col + 1 + patternPos];
            board[row + 1 + patternPos][col + 1 + patternPos] = black;
            if (BanMove(board, row + 1 + patternPos, col + 1 + patternPos) == 0) {
                threeCount++;
            }
            board[row + 1 + patternPos][col + 1 + patternPos] = backup4;
        }
    }

    patternPos = (strstr(dirState, "010110") != NULL)
                 ? (int)(strstr(dirState, "010110") - dirState)
                 : -1;
    if (patternPos >= 0 && patternPos <= 3) {
        if (IN_RANGE(row - 2 + patternPos, col - 2 + patternPos) &&
            board[row - 2 + patternPos][col - 2 + patternPos] == none)
        {
            CHESSSTATE backup5 = board[row - 2 + patternPos][col - 2 + patternPos];
            board[row - 2 + patternPos][col - 2 + patternPos] = black;
            if (BanMove(board, row - 2 + patternPos, col - 2 + patternPos) == 0) {
                threeCount++;
            }
            board[row - 2 + patternPos][col - 2 + patternPos] = backup5;
        }
    }

    patternPos = (strstr(dirState, "011010") != NULL)
                 ? (int)(strstr(dirState, "011010") - dirState)
                 : -1;
    if (patternPos >= 0 && patternPos <= 4) {
        if (IN_RANGE(row - 1 + patternPos, col + 1 - patternPos) &&
            board[row - 1 + patternPos][col + 1 - patternPos] == none)
        {
            CHESSSTATE backup6 = board[row - 1 + patternPos][col + 1 - patternPos];
            board[row - 1 + patternPos][col + 1 - patternPos] = black;
            if (BanMove(board, row - 1 + patternPos, col + 1 - patternPos) == 0) {
                threeCount++;
            }
            board[row - 1 + patternPos][col + 1 - patternPos] = backup6;
        }
    }

    // 若超过 1 个活三 / 跳三，则说明存在三三禁手
    return (threeCount > 1) ? 1 : 0;
}

/*-------------------------------------
 *  BanMove (主函数) - 禁手规则最终判定
 *-------------------------------------*/

/**
 * @brief 禁手规则检查（主入口）
 * @param[in] board 棋盘的二维指针
 * @param[in] row   要检查点的行坐标
 * @param[in] col   要检查点的列坐标
 * @retval 0 无禁手
 * @retval 1 三三禁手
 * @retval 2 四四禁手
 * @retval 3 长连禁手
 */
int BanMove(CHESSSTATE** board, int row, int col)
{
    // 判断长连禁手
    if (CheckLongChain(board, row, col) == 1) {
        return 3;
    }

    // 判断四四禁手
    if (CheckFourInRow(board, row, col) == 1) {
        return 2;
    }

    // 判断三三禁手
    if (CheckThreeInRow(board, row, col) == 1) {
        return 1;
    }

    // 无禁手
    return 0;
}
