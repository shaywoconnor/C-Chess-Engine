#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "move_gen.h"
#include "board.h"

#define ASSERT(_a, _s) if(!(_a)) { fprintf(stderr, "%s\n", _s); exit(1); }

int main(int argc, char** argv)
{
  ASSERT((argc == 2), "Error: Program must take exactly 1 argument, the FEN of the position");

  struct timespec time_start, time_end;

  clock_gettime(CLOCK_MONOTONIC, &time_start);

  Board board;
  board_build_pos(&board, argv[1]);

  Legal_Moves moves = move_gen_get_legal(&board);

  clock_gettime(CLOCK_MONOTONIC, &time_end);

  double ms = (time_end.tv_sec - time_start.tv_sec) * 1000.0
            + (time_end.tv_nsec - time_start.tv_nsec) / 1e6;
  printf("%d Moves Found in %.3f ms(note, timing includes magic bitboard generation):\n", moves.move_count, ms);

  for (int i = 0; i < moves.move_count; i++)
  {
    board_move_t move = moves.moves[i];
    char start[3] =
    {
      'a' + (move & 0b00000111),
      '1' + ((move & 0b00111000) >> 3),
      '\0'
    };

    char end[3] =
    {
      'a' + ((move >> 6) & 0b00000111),
      '1' + (((move >> 6) & 0b00111000) >> 3),
      '\0'
    };
    printf("  Move %s to %s", start, end);
    if ((move >> 12) & PROMOTION) printf(" promoting to %c", "nbrq"[(move >> 12) & 0b0011]);
    printf("\n");
  }

  move_gen_free(&moves);
}
