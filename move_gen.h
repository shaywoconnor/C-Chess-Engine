#ifndef MOVE_GENERATION_H
#define MOVE_GENERATION_H

#include "board.h"

struct Move
{
  board_pos_t start;
  board_pos_t end;
};
typedef struct Move Move;

struct Legal_Moves
{
  Move* moves;
  int move_count;
};
typedef struct Legal_Moves Legal_Moves;

Legal_Moves move_gen_get_legal(Board* board);

#endif //MOVE_GENERATION_H
