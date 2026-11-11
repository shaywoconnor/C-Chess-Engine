#ifndef MOVE_GENERATION_H
#define MOVE_GENERATION_H

#include "board.h"

//0-5 from, 6-11 to, 12-15 flags
typedef uint16_t board_move_t;
#define QUITE_MOVE 0
#define DOUBLE_PUSH 1
#define KING_CASTLE 2
#define QUEEN_CASTLE 3
#define CAPTURE 4
#define EN_PASSANT 5
#define PROMOTION 8

struct Legal_Moves
{
  board_move_t* moves;
  int move_count;
  int move_buf_count;
};
typedef struct Legal_Moves Legal_Moves;

Legal_Moves move_gen_get_legal(Board* board);
void move_gen_free(Legal_Moves* legal_moves);

#endif //MOVE_GENERATION_H
