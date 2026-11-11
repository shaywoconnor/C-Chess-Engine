#include <stdlib.h>

#include "move_gen.h"

void init_legal_moves(Legal_Moves* legal_moves)
{
  legal_moves->move_count = 0;
  legal_moves->move_buf_count = 20; //avoids unnecessary early reallocs 
  legal_moves->moves = (board_move_t*)malloc(sizeof(board_move_t) * legal_moves->move_buf_count);
}
void add_move(Legal_Moves* legal_moves, board_move_t move)
{
  if (legal_moves->move_count == legal_moves->move_buf_count-1)
  {
    legal_moves->move_buf_count *= 2;
    legal_moves->moves = (board_move_t*)realloc(legal_moves->moves, sizeof(board_move_t) * legal_moves->move_buf_count);
  }
  legal_moves->moves[legal_moves->move_count] = move;
  legal_moves->move_count++;
}
void clamp_move_buf(Legal_Moves* legal_moves)
{
  if (legal_moves->move_count == 0) return;
  legal_moves->moves = (board_move_t*)realloc(legal_moves->moves, sizeof(board_move_t) * legal_moves->move_count);
  legal_moves->move_buf_count = legal_moves->move_count;
}

//chessprogramming wiki is my source for almost all of move generation logic
static const bit_board_t zero = 0;
static const bit_board_t full = UINT64_MAX;
#define WHITE_PAWNS   board->pieces[BB_WHITE][BB_PAWN]
#define WHITE_KNIGHTS board->pieces[BB_WHITE][BB_KNIGHT]
#define WHITE_BISHOPS board->pieces[BB_WHITE][BB_BISHOP]
#define WHITE_ROOKS   board->pieces[BB_WHITE][BB_ROOK]
#define WHITE_QUEENS  board->pieces[BB_WHITE][BB_QUEEN]
#define WHITE_KING    board->pieces[BB_WHITE][BB_KING]

#define BLACK_PAWNS   board->pieces[BB_BLACK][BB_PAWN]
#define BLACK_KNIGHTS board->pieces[BB_BLACK][BB_KNIGHT]
#define BLACK_BISHOPS board->pieces[BB_BLACK][BB_BISHOP]
#define BLACK_ROOKS   board->pieces[BB_BLACK][BB_ROOK]
#define BLACK_QUEENS  board->pieces[BB_BLACK][BB_QUEEN]
#define BLACK_KING    board->pieces[BB_BLACK][BB_KING]

#define NOT_A_FILE 0xfefefefefefefefeULL
#define NOT_H_FILE 0x7f7f7f7f7f7f7f7fULL

#define RANK_1 0x00000000000000ffULL
#define RANK_3 0x0000000000ff0000ULL
#define RANK_6 0x0000ff0000000000ULL
#define RANK_8 0xff00000000000000ULL

bit_board_t up_one(bit_board_t b)         { return b << 8; }
bit_board_t down_one(bit_board_t b)       { return b >> 8; }
bit_board_t right_one(bit_board_t b)      { return (b << 1) & NOT_A_FILE; }
bit_board_t left_one(bit_board_t b)       { return (b >> 1) & NOT_H_FILE; }
bit_board_t up_right_one(bit_board_t b)   { return (b << 9) & NOT_A_FILE; }
bit_board_t down_right_one(bit_board_t b) { return (b >> 7) & NOT_A_FILE; }
bit_board_t up_left_one(bit_board_t b)    { return (b << 7) & NOT_H_FILE; }
bit_board_t down_left_one(bit_board_t b)  { return (b >> 9) & NOT_H_FILE; }

typedef bit_board_t (*shift_func_t)(bit_board_t);

static const shift_func_t directions[8] =
{
  up_one, down_one, right_one, left_one,
  up_right_one, down_left_one, up_left_one, down_right_one
};
#define ROOK_DIRECTIONS   (directions)
#define BISHOP_DIRECTIONS (directions + 4)

static uint8_t tables_initialised = 0;
static bit_board_t knight_attacks[64];
static bit_board_t king_attacks[64];
static bit_board_t pawn_attacks[2][64];
static bit_board_t between[64][64];
static bit_board_t line[64][64];

static bit_board_t rook_masks[64];
static bit_board_t bishop_masks[64];
static uint8_t rook_shifts[64];
static uint8_t bishop_shifts[64];
static bit_board_t* rook_tables[64];
static bit_board_t* bishop_tables[64];
static bit_board_t rook_table_buf[102400];
static bit_board_t bishop_table_buf[5248];

//i got claude to brute force these magic bitboards
static const bit_board_t rook_magics[64] =
{
  0x1080004008801020ULL, 0x0840092002c03000ULL, 0x1900200010400900ULL, 0x0880100008000480ULL,
  0x4200100420080200ULL, 0x8100020100080400ULL, 0x0200040110886200ULL, 0x0200008040220411ULL,
  0x0404800084400220ULL, 0x0000401000402000ULL, 0x0086001081220440ULL, 0x0408800800100280ULL,
  0x000a001201040820ULL, 0x8848800200840080ULL, 0x4001000100040200ULL, 0x0442000102105084ULL,
  0x9080010020804100ULL, 0x0040404000201009ULL, 0x0000808010002009ULL, 0x2200090021d00100ULL,
  0x0008008008040080ULL, 0x0004004002010040ULL, 0x0011040008015042ULL, 0x00000a0001768104ULL,
  0x0000800080204009ULL, 0x2010004140002001ULL, 0x9800200280100080ULL, 0x1000100080080080ULL,
  0x0442000a00049020ULL, 0x2100040080020080ULL, 0x0800120400900148ULL, 0x0010040a00128541ULL,
  0x2800804000800030ULL, 0x1010002000400041ULL, 0x4000200011004100ULL, 0x0610008410800800ULL,
  0x0400802402800800ULL, 0xc100020080800400ULL, 0x0002000802000401ULL, 0x0182085882000401ULL,
  0x0220204000808000ULL, 0x2860100040024022ULL, 0x0001002004110040ULL, 0x99101042000a0020ULL,
  0x0004080004008080ULL, 0x0010040002008080ULL, 0x2012004881020004ULL, 0x8300842444820011ULL,
  0x0088403882010200ULL, 0x0820400080210100ULL, 0x0110910040a00300ULL, 0x0801100280080480ULL,
  0x0242009008200600ULL, 0x1002000489500200ULL, 0x0040800200010080ULL, 0x0091800041000080ULL,
  0x0000209300488001ULL, 0x04c1002414824001ULL, 0x020020000b001041ULL, 0x7000100004200901ULL,
  0x8002002004100802ULL, 0x30010002084c0007ULL, 0x0888221800813004ULL, 0x4000002840840112ULL
};

static const bit_board_t bishop_magics[64] =
{
  0xa010041108003100ULL, 0x006082020a002900ULL, 0x6810010619200000ULL, 0x08281a0520000408ULL,
  0x0001104001000400ULL, 0x0018901008048400ULL, 0x00040a0210245280ULL, 0x000200210808a402ULL,
  0x9140048410821200ULL, 0x0800091010820041ULL, 0x20504804832202c0ULL, 0x0100091401081000ULL,
  0x8021011140000012ULL, 0x0810020804450400ULL, 0x208b0542109008a2ULL, 0x0080084a08040204ULL,
  0x0040e2a80811244cULL, 0x2505022008008108ULL, 0x0430220100420040ULL, 0x010a040420220040ULL,
  0x1105000290400000ULL, 0x0093001200822120ULL, 0x4000a62048043004ULL, 0x280120048a015004ULL,
  0x006090002a020814ULL, 0x44042000240800d0ULL, 0x01102800040a4400ULL, 0x1004080080220040ULL,
  0x0001001011004024ULL, 0x0010044000805040ULL, 0x0914041200820100ULL, 0x0004821012821480ULL,
  0x0024040500c05021ULL, 0x0088611002080200ULL, 0x0116080a00040020ULL, 0x4000020080080080ULL,
  0x2450450140840040ULL, 0x0000880201484100ULL, 0x0222020404020092ULL, 0x8081110600002e00ULL,
  0x2842101105000801ULL, 0x1100809008001025ULL, 0x00020202221c0400ULL, 0x0422014022009020ULL,
  0x0210046102100c00ULL, 0xc004008082029102ULL, 0x00aa461801101200ULL, 0x0404080080201108ULL,
  0x020542108c205002ULL, 0x0410544804100100ULL, 0x0040910841100000ULL, 0x0400200042021100ULL,
  0x00004204850400c0ULL, 0x0200100410a42102ULL, 0x1040020801210102ULL, 0x0805040410420000ULL,
  0x2884804130100200ULL, 0x800c262201242000ULL, 0x1058000194108800ULL, 0x0014221054420204ULL,
  0x0104000012a02200ULL, 0x0200881003300100ULL, 0x0140400202840100ULL, 0x0402020801010201ULL
};

static inline board_move_t new_move(board_pos_t from, board_pos_t to, uint8_t flags)
{
  return from | (to << 6) | (flags << 12);
}

static inline bit_board_t rook_attacks(board_pos_t pos, bit_board_t occupied)
{
  return rook_tables[pos][((occupied & rook_masks[pos]) * rook_magics[pos]) >> rook_shifts[pos]];
}
static inline bit_board_t bishop_attacks(board_pos_t pos, bit_board_t occupied)
{
  return bishop_tables[pos][((occupied & bishop_masks[pos]) * bishop_magics[pos]) >> bishop_shifts[pos]];
}

static bit_board_t ray_attacks(board_pos_t pos, bit_board_t occupied, shift_func_t shift)
{
  bit_board_t attacks = 0;
  bit_board_t square = 1ULL << pos;
  while ((square = shift(square)))
  {
    attacks |= square;
    if (square & occupied) break;
  }
  return attacks;
}
static bit_board_t slow_slider_attacks(board_pos_t pos, bit_board_t occupied, const shift_func_t* dirs)
{
  return ray_attacks(pos, occupied, dirs[0]) | ray_attacks(pos, occupied, dirs[1])
       | ray_attacks(pos, occupied, dirs[2]) | ray_attacks(pos, occupied, dirs[3]);
}
static bit_board_t slider_mask(board_pos_t pos, const shift_func_t* dirs)
{
  bit_board_t mask = 0;
  for (uint8_t i = 0; i < 4; i++)
  {
    bit_board_t square = dirs[i](1ULL << pos);
    while (square && dirs[i](square))
    {
      mask |= square;
      square = dirs[i](square);
    }
  }
  return mask;
}
static void init_slider_tables(bit_board_t* masks, uint8_t* shifts, bit_board_t** tables, bit_board_t* buf, const bit_board_t* magics, const shift_func_t* dirs)
{
  for (board_pos_t pos = 0; pos < 64; pos++)
  {
    masks[pos] = slider_mask(pos, dirs);
    shifts[pos] = 64 - __builtin_popcountll(masks[pos]);
    tables[pos] = buf;
    bit_board_t blockers = 0;
    do
    {
      tables[pos][(blockers * magics[pos]) >> shifts[pos]] = slow_slider_attacks(pos, blockers, dirs);
      blockers = (blockers - masks[pos]) & masks[pos];
    } while (blockers);
    buf += 1ULL << (64 - shifts[pos]);
  }
}

static void init_tables(void)
{
  for (board_pos_t pos = 0; pos < 64; pos++)
  {
    bit_board_t b = 1ULL << pos;
    king_attacks[pos] = up_one(b) | down_one(b) | right_one(b) | left_one(b)
                      | up_right_one(b) | down_right_one(b) | up_left_one(b) | down_left_one(b);
    knight_attacks[pos] = up_one(up_right_one(b)) | up_one(up_left_one(b))
                        | down_one(down_right_one(b)) | down_one(down_left_one(b))
                        | right_one(up_right_one(b)) | right_one(down_right_one(b))
                        | left_one(up_left_one(b)) | left_one(down_left_one(b));
    pawn_attacks[BB_WHITE][pos] = up_right_one(b) | up_left_one(b);
    pawn_attacks[BB_BLACK][pos] = down_right_one(b) | down_left_one(b);

    for (uint8_t dir = 0; dir < 8; dir++)
    {
      bit_board_t full_line = ray_attacks(pos, 0, directions[dir]) | ray_attacks(pos, 0, directions[dir ^ 1]) | b;
      bit_board_t path = 0;
      bit_board_t square = directions[dir](b);
      while (square)
      {
        board_pos_t to = __builtin_ctzll(square);
        between[pos][to] = path;
        line[pos][to] = full_line;
        path |= square;
        square = directions[dir](square);
      }
    }
  }
  init_slider_tables(rook_masks, rook_shifts, rook_tables, rook_table_buf, rook_magics, ROOK_DIRECTIONS);
  init_slider_tables(bishop_masks, bishop_shifts, bishop_tables, bishop_table_buf, bishop_magics, BISHOP_DIRECTIONS);
  tables_initialised = 1;
}

static bit_board_t attackers_to(Board* board, board_pos_t pos, uint8_t by, bit_board_t occupied)
{
  bit_board_t* pieces = board->pieces[by];
  return (pawn_attacks[by ^ 1][pos] & pieces[BB_PAWN])
       | (knight_attacks[pos] & pieces[BB_KNIGHT])
       | (king_attacks[pos] & pieces[BB_KING])
       | (bishop_attacks(pos, occupied) & (pieces[BB_BISHOP] | pieces[BB_QUEEN]))
       | (rook_attacks(pos, occupied) & (pieces[BB_ROOK] | pieces[BB_QUEEN]));
}

static void add_moves(Legal_Moves* legal_moves, board_pos_t from, bit_board_t targets, bit_board_t enemy)
{
  while (targets)
  {
    board_pos_t to = __builtin_ctzll(targets);
    add_move(legal_moves, new_move(from, to, ((1ULL << to) & enemy) ? CAPTURE : QUITE_MOVE));
    targets &= targets - 1;
  }
}

static void add_pawn_moves(Legal_Moves* legal_moves, bit_board_t targets, int8_t offset, uint8_t flags, bit_board_t pinned, board_pos_t king_pos)
{
  while (targets)
  {
    board_pos_t to = __builtin_ctzll(targets);
    board_pos_t from = to - offset;
    targets &= targets - 1;
    if ((pinned & (1ULL << from)) && !(line[king_pos][from] & (1ULL << to))) continue;
    if ((1ULL << to) & (RANK_1 | RANK_8))
    {
      for (uint8_t piece = 0; piece < 4; piece++)
      {
        add_move(legal_moves, new_move(from, to, PROMOTION | flags | piece));
      }
    }
    else
    {
      add_move(legal_moves, new_move(from, to, flags));
    }
  }
}

void generate_legal_moves(Legal_Moves* legal_moves, Board* board)
{
  uint8_t us = (board->info >> 7) & 1;
  uint8_t them = us ^ 1;

  bit_board_t own = zero;
  bit_board_t enemy = zero;
  for (uint8_t piece = BB_PAWN; piece <= BB_KING; piece++)
  {
    own |= board->pieces[us][piece];
    enemy |= board->pieces[them][piece];
  }
  bit_board_t occupied = ~board->empty;
  board_pos_t king_pos = __builtin_ctzll(board->pieces[us][BB_KING]);

  bit_board_t targets = king_attacks[king_pos] & ~own;
  while (targets)
  {
    board_pos_t to = __builtin_ctzll(targets);
    if (!attackers_to(board, to, them, occupied ^ (1ULL << king_pos)))
    {
      add_move(legal_moves, new_move(king_pos, to, ((1ULL << to) & enemy) ? CAPTURE : QUITE_MOVE));
    }
    targets &= targets - 1;
  }

  bit_board_t checkers = attackers_to(board, king_pos, them, occupied);
  if (__builtin_popcountll(checkers) > 1) return;

  bit_board_t check_mask = full;
  if (checkers) check_mask = between[king_pos][__builtin_ctzll(checkers)] | checkers;

  bit_board_t pinned = zero;
  bit_board_t snipers = (rook_attacks(king_pos, enemy) & (board->pieces[them][BB_ROOK] | board->pieces[them][BB_QUEEN]))
                      | (bishop_attacks(king_pos, enemy) & (board->pieces[them][BB_BISHOP] | board->pieces[them][BB_QUEEN]));
  while (snipers)
  {
    bit_board_t blockers = between[king_pos][__builtin_ctzll(snipers)] & occupied;
    if (__builtin_popcountll(blockers) == 1) pinned |= blockers & own;
    snipers &= snipers - 1;
  }

  bit_board_t pieces = board->pieces[us][BB_KNIGHT] & ~pinned;
  while (pieces)
  {
    board_pos_t from = __builtin_ctzll(pieces);
    add_moves(legal_moves, from, knight_attacks[from] & ~own & check_mask, enemy);
    pieces &= pieces - 1;
  }

  pieces = board->pieces[us][BB_BISHOP] | board->pieces[us][BB_QUEEN];
  while (pieces)
  {
    board_pos_t from = __builtin_ctzll(pieces);
    targets = bishop_attacks(from, occupied) & ~own & check_mask;
    if (pinned & (1ULL << from)) targets &= line[king_pos][from];
    add_moves(legal_moves, from, targets, enemy);
    pieces &= pieces - 1;
  }

  pieces = board->pieces[us][BB_ROOK] | board->pieces[us][BB_QUEEN];
  while (pieces)
  {
    board_pos_t from = __builtin_ctzll(pieces);
    targets = rook_attacks(from, occupied) & ~own & check_mask;
    if (pinned & (1ULL << from)) targets &= line[king_pos][from];
    add_moves(legal_moves, from, targets, enemy);
    pieces &= pieces - 1;
  }

  bit_board_t pawns = board->pieces[us][BB_PAWN];
  if (us == BB_WHITE)
  {
    bit_board_t single_push = up_one(pawns) & board->empty;
    add_pawn_moves(legal_moves, single_push & check_mask, 8, QUITE_MOVE, pinned, king_pos);
    add_pawn_moves(legal_moves, up_one(single_push & RANK_3) & board->empty & check_mask, 16, DOUBLE_PUSH, pinned, king_pos);
    add_pawn_moves(legal_moves, up_left_one(pawns) & enemy & check_mask, 7, CAPTURE, pinned, king_pos);
    add_pawn_moves(legal_moves, up_right_one(pawns) & enemy & check_mask, 9, CAPTURE, pinned, king_pos);
  }
  else
  {
    bit_board_t single_push = down_one(pawns) & board->empty;
    add_pawn_moves(legal_moves, single_push & check_mask, -8, QUITE_MOVE, pinned, king_pos);
    add_pawn_moves(legal_moves, down_one(single_push & RANK_6) & board->empty & check_mask, -16, DOUBLE_PUSH, pinned, king_pos);
    add_pawn_moves(legal_moves, down_left_one(pawns) & enemy & check_mask, -9, CAPTURE, pinned, king_pos);
    add_pawn_moves(legal_moves, down_right_one(pawns) & enemy & check_mask, -7, CAPTURE, pinned, king_pos);
  }

  if (board->info & (1ULL << 8))
  {
    uint8_t ep_file = (board->info >> 4) & 0b0111;
    board_pos_t ep_pos = ((us == BB_WHITE ? 5 : 2) << 3) | ep_file;
    board_pos_t captured_pos = (us == BB_WHITE) ? ep_pos - 8 : ep_pos + 8;
    bit_board_t attackers = pawn_attacks[them][ep_pos] & pawns;
    while (attackers)
    {
      board_pos_t from = __builtin_ctzll(attackers);
      bit_board_t after = (occupied ^ (1ULL << from) ^ (1ULL << captured_pos)) | (1ULL << ep_pos);
      if (!(attackers_to(board, king_pos, them, after) & ~(1ULL << captured_pos)))
      {
        add_move(legal_moves, new_move(from, ep_pos, EN_PASSANT));
      }
      attackers &= attackers - 1;
    }
  }

  if (checkers) return;
  if (us == BB_WHITE)
  {
    if ((board->info & 0b0001) && !(occupied & 0x0000000000000060ULL)
        && !attackers_to(board, 5, them, occupied) && !attackers_to(board, 6, them, occupied))
    {
      add_move(legal_moves, new_move(4, 6, KING_CASTLE));
    }
    if ((board->info & 0b0010) && !(occupied & 0x000000000000000eULL)
        && !attackers_to(board, 3, them, occupied) && !attackers_to(board, 2, them, occupied))
    {
      add_move(legal_moves, new_move(4, 2, QUEEN_CASTLE));
    }
  }
  else
  {
    if ((board->info & 0b0100) && !(occupied & 0x6000000000000000ULL)
        && !attackers_to(board, 61, them, occupied) && !attackers_to(board, 62, them, occupied))
    {
      add_move(legal_moves, new_move(60, 62, KING_CASTLE));
    }
    if ((board->info & 0b1000) && !(occupied & 0x0e00000000000000ULL)
        && !attackers_to(board, 59, them, occupied) && !attackers_to(board, 58, them, occupied))
    {
      add_move(legal_moves, new_move(60, 58, QUEEN_CASTLE));
    }
  }
}

Legal_Moves move_gen_get_legal(Board* board)
{
  if (!tables_initialised) init_tables();
  Legal_Moves legal_moves;
  init_legal_moves(&legal_moves);
  generate_legal_moves(&legal_moves, board);
  clamp_move_buf(&legal_moves);
  return legal_moves;
}
void move_gen_free(Legal_Moves* legal_moves)
{
  free(legal_moves->moves);
}
