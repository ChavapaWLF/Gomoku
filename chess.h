#ifndef __CHESS_H
#define __CHESS_H

#define ROW 15
#define COLUMN 15
#define SIZE 15
//ÆåÅÌ×Ö·û
#define LEFT_TOP "©° "
#define LEFT_BOTTOM "©¸ "
#define LEFT_CENTER "©À "
#define RIGHT_TOP "©´"
#define RIGHT_BOTTOM "©¼"
#define RIGHT_CENTER "©È"
#define TOP_CENTER "©Ð "
#define BOTTOM_CENTER "©Ø "
#define INTERNAL "©à "
//Æå×Ó×Ö·û
#define BLACK "¡ñ"
#define WHITE "¡ð"
#define BLACK_1 "¡ñ "
#define WHITE_1 "¡ð "
#define LASTBLACK "¡ø"
#define LASTWHITE "¡÷"
#define LASTBLACK_1 "¡ø "
#define LASTWHITE_1 "¡÷ "

//Æå×ÓµÄ×´Ì¬
//Ã»Æå:0; ºÚÆå:1; °×Æå:2;
typedef enum {
    none = 0, black = 1, white = 2
}CHESSSTATE;

#endif // __CHESS_H