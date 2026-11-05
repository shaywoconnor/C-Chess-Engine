#include <stdio.h>
#include <time.h>

#include "move_gen.h"
#include "board.h"

#define ASSERT(_a, _s) if(!_a) fprintf(stderr, _s);

int main(int argc, char** argv)
{
  ASSERT((argc == 2), "Error: Program must take exactly 1 argument, the FEN of the position");
  
  struct timespec time_start, time_end;

  clock_gettime(CLOCK_MONOTONIC, &time_start);

  Board board;
  board_build_pos(&board, argv[1]);

  Legal_Moves moves = move_gen_get_legal(&board);

  Move* best_move = eval_get_best_move(&board, &moves);

  clock_gettime(CLOCK_MONOTONIC, &time_end);

  double ms = (time_end.tv_sec - time_start.tv_sec) * 1000.0
            + (time_end.tv_nsec - time_start.tv_nsec) / 1e6;
  char start[3] =
  {
    'a' + (best_move->start & 0b00000111),
    '1' + ((best_move->start & 0b00111000) >> 3),
    '\0'
  };

  char end[3] =
  {
    'a' + (best_move->end & 0b00000111),
    '1' + ((best_move->end & 0b00111000) >> 3),
    '\0'
  };
  printf("Move Found in %.3f ms:\n  Move %s to %s", ms, start, end);
}
