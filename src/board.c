#include "board.h"

static board_info_t get_info(const char* info);

static inline board_pos_t get_pos(uint8_t rank, uint8_t file)
{
  return ((rank & 0b0111) << 3) | (file & 0b0111);
}
static inline void decrement_rank(board_pos_t* pos)
{
  if (*pos < 0b00001000) return;
  *pos -= 0b00001000;
}

/*
 *    All FEN-related functions, for now, assume a perfectly valid FEN
 *    I will implement validations after the engine is implemented
 */
void board_build_pos(Board* board, const char* FEN)
{
  *board = (Board){0};

  board_pos_t read_pos = get_pos(7, 0);
  while(*FEN != ' ')
  {
    char c = *FEN;
    uint8_t colour = BB_BLACK;
    if (c == '/')
    {
      //dec 2 times as reading h file increments rank
      decrement_rank(&read_pos);
      decrement_rank(&read_pos);
      FEN++;
      continue;
    }
    if ('0' <= c && c <= '8')
    {
      uint8_t count = c - '0';
      board->empty |= ((1ULL << count) - 1) << read_pos;
      read_pos += count;
      FEN++;
      continue;
    }
    if ('A' <= c && c <= 'Z')
    {
      colour = BB_WHITE;
      c |= 0x20; //ascii tolower
    }
    read_pos &= 0b00111111;
    switch (c)
    {
      case 'p':
        board->pieces[colour][BB_PAWN] |= 1ULL << read_pos;
        break;
      case 'n':
        board->pieces[colour][BB_KNIGHT] |= 1ULL << read_pos;
        break;
      case 'b':
        board->pieces[colour][BB_BISHOP] |= 1ULL << read_pos;
        break;
      case 'r':
        board->pieces[colour][BB_ROOK] |= 1ULL << read_pos;
        break;
      case 'q':
        board->pieces[colour][BB_QUEEN] |= 1ULL << read_pos;
        break;
      case 'k':
        board->pieces[colour][BB_KING] |= 1ULL << read_pos;
        break;
    }
    read_pos++;
    FEN++;
  }
  FEN++;
  board->info = get_info(FEN);
}

static board_info_t get_info(const char* FEN)
{
  board_info_t r_info = 0;
  r_info |= (1u & (*FEN == 'b')) << 7;
  FEN+=2;
  while (*FEN != ' ')
  {
    switch (*FEN)
    {
      case '-':
        break;
      case 'K':
        r_info |= (1u << 0);
        break;
      case 'k':
        r_info |= (1u << 2);
        break;
      case 'Q':
        r_info |= (1u << 1);
        break;
      case 'q':
        r_info |= (1u << 3);
        break;
    }
    FEN++;
  }
  FEN++;
  if (*FEN == '-')
  {
    FEN+=2;
  }
  else
  {
    r_info |= (1u << 8);
    r_info |= ((*FEN - 'a') & 0b0111) << 4;
    FEN+=3;
  }

  uint64_t half_move = 0;
  while (*FEN != ' ')
  {
    half_move = half_move * 10 + (*FEN - '0');
    FEN++;
  }
  r_info |= (half_move & 0xFFFFULL) << 9;
  FEN++;

  uint64_t full_move = 0;
  while (*FEN >= '0' && *FEN <= '9')
  {
    full_move = full_move * 10 + (*FEN - '0');
    FEN++;
  }
  r_info |= (full_move & 0xFFFFFFFFULL) << 25;

  return r_info;
}
