#ifndef BOARD_H
#define BOARD_H

#include <stdint.h>

//board_pos_t
//bit  |
//-------------------------
//7    | 0
//6    | 0
//-------------------------
//5    | Rank
//4    | ..
//3    | ..
//-------------------------
//2    | File (a=0, h=7)
//1    | ..
//0    | ..
typedef uint8_t board_pos_t;

//board_info_t
//bit  |
//-------------------------
//63   | Unused
//..   | ..
//57   | Unused
//-------------------------
//56   | Full-Move Count (bits 56-25)
//..   | ..
//25   | ..
//-------------------------
//24   | Half-Move Count (bits 24-9)
//..   | ..
//9    | ..
//-------------------------
//8    | En Passant Available
//-------------------------
//7    | Player to Move White = 0 Black = 1
//-------------------------
//6    | En Passant File (bits 6-4)
//5    | ..
//4    | ..
//-------------------------
//3    | Black Queen Side Castle
//2    | Black King Side Castle
//1    | White Queen Side Castle
//0    | White King Side Castle
typedef uint64_t board_info_t;

//bit_board_t:
//64 bit representation of the board, where each square is 1 bit
//LERF (Little-Endian Rank-File) order:
//bit  | square
//0    | a1
//..   | ..
//7    | h1
//8    | a2
//..   | ..
//15   | h2
//..   | ..
//56   | a8
//..   | ..
//63   | h8
typedef uint64_t bit_board_t;

#define BB_WHITE 0
#define BB_BLACK 1
#define BB_PAWN   0
#define BB_KNIGHT 1
#define BB_BISHOP 2
#define BB_ROOK   3
#define BB_QUEEN  4
#define BB_KING   5

struct Board
{
  bit_board_t pieces[2][6];
  board_info_t info;
};
typedef struct Board Board;

void board_build_pos(Board* board, const char* FEN);

#endif //BOARD_H
