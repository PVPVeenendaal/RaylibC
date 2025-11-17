/**********************************\
 ==================================

    GUI CHESS PASSTHROUGH

            by

        Peter Veenendaal

    USING ENGINE CODE FROM :

    BITBOARD CHESS ENGINE v1.2

                by

         Code Monkey King

    FUNCTIONS NOT USED ARE REMOVED

    BECAUSE THERE ARE 2 ENGINES NEEDED,
    ONE FOR THE LEFT BOARD AND ONE FOR THE RIGHT BOURD
    THE ORIGINAL CODE IS DOUBLED SO BOTH ENGINES USE THERE OWN MEMORY.
    FOR THE LEFT BOARD ALL FUNCTIONS AND VARIABLES HAVE THE PREFIX lb_
    FOR THE RIGHT BOARD ALL FUNCTIONS AND VARIABLES HAVE THE PREFIX rb_
    
    ADDING CODE FOR THE PASSTHOUGH AND ASSERT TAGS BETWEEN ///<ADD> and ///</ADD>

                by

        Peter Veenendaal

 ==================================
\**********************************/

// system headers
#include "raylib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>

///<ADD>

// define min max
#define Max(a, b) ((a) >= (b) ? (a) : (b))
#define Min(a, b) ((a) <= (b) ? (a) : (b))

#ifdef NDEBUG // release mode

#define PrintAssert(ignore) (void *) 0

#else

#define PrintAssert(expr)                       \
    if (!(expr))                                \
    {                                           \
        printf("%s%s\n%s%s\n%s%d\n\n",          \
               "Assertion is not true:", #expr, \
               "in file ", __FILE__,            \
               "on line ", __LINE__);           \
    }                                             
#endif

///</ADD>

// -----------------------------------------------------------------------
// BBC code
// @author Code Monkey King
// -----------------------------------------------------------------------

// define version BBC
#define version "1.2"

// define bitboard data type
#define U64 unsigned long long

// FEN dedug positions
#define empty_board "8/8/8/8/8/8/8/8 b - - "
#define start_position "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1 "
#define tricky_position "r3k2r/p1ppqpb1/bn2pnp1/3PN3/1p2P3/2N2Q1p/PPPBBPPP/R3K2R w KQkq - 0 1 "
#define killer_position "rnbqkb1r/pp1p1pPp/8/2p1pP2/1P1P4/3P3P/P1P1P3/RNBQKBNR w KQkq e6 0 1"
#define cmk_position "r2q1rk1/ppp2ppp/2n1bn2/2b1p3/3pP3/3P1NPP/PPP1NPB1/R1BQ1RK1 b - - 0 9 "
#define repetitions "2r3k1/R7/8/1R6/8/8/P4KPP/8 w - - 0 40 "

// set/get/pop bit macros
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

///<ADD>
/*
  8  1 1 1 1 1 1 1 1
  7  1 1 1 1 1 1 1 1
  6  1 1 1 1 1 1 1 1
  5  1 1 1 1 1 1 1 1
  4  1 1 1 1 1 1 1 1
  3  1 1 1 1 1 1 1 1
  2  1 1 1 1 1 1 1 1
  1  1 1 1 1 1 1 1 1

     a b c d e f g h

     Bitboard: 18446744073709551615
*/

#define piece_put_options 18446744073709551615ULL

/*
  8  0 0 0 0 0 0 0 0
  7  1 1 1 1 1 1 1 1
  6  1 1 1 1 1 1 1 1
  5  1 1 1 1 1 1 1 1
  4  1 1 1 1 1 1 1 1
  3  1 1 1 1 1 1 1 1
  2  1 1 1 1 1 1 1 1
  1  0 0 0 0 0 0 0 0

     a b c d e f g h

    Bitboard: 72057594037927680d
*/

#define pawn_put_options 72057594037927680ULL
///</ADD>

/*
To generate put options:
for the pawn: bitboard option = ~occupancies[both] & pawn_put_options
for the knight, bishop, rook or queen: bitboard option = ~occupancies[both] & piece_options
*/

/*
          binary move bits                               hexidecimal constants

    0000 0000 0000 0000 0011 1111    source square       0x3f
    0000 0000 0000 1111 1100 0000    target square       0xfc0
    0000 0000 1111 0000 0000 0000    piece               0xf000
    0000 1111 0000 0000 0000 0000    promoted piece      0xf0000
    0001 0000 0000 0000 0000 0000    capture flag        0x100000
    0010 0000 0000 0000 0000 0000    double push flag    0x200000
    0100 0000 0000 0000 0000 0000    enpassant flag      0x400000
    1000 0000 0000 0000 0000 0000    castling flag       0x800000
*/

///<ADD>
/*
    a put option move: source and target are the same,
    piece is pawn, knight, bishop, rook or queen,
    no flags are set
*/
///</ADD>

// encode move
#define encode_move(source, target, piece, promoted, capture, double, enpassant, castling) \
    (source) |                                                                             \
        ((target) << 6) |                                                                  \
        ((piece) << 12) |                                                                  \
        ((promoted) << 16) |                                                               \
        ((capture) << 20) |                                                                \
        ((double) << 21) |                                                                 \
        ((enpassant) << 22) |                                                              \
        ((castling) << 23)

// extract source square
#define get_move_source(move) ((move) & 0x3f)

// extract target square
#define get_move_target(move) (((move) & 0xfc0) >> 6)

// extract piece
#define get_move_piece(move) (((move) & 0xf000) >> 12)

// extract promoted piece
#define get_move_promoted(move) (((move) & 0xf0000) >> 16)

// extract capture flag
#define get_move_capture(move) ((move) & 0x100000)

// extract double pawn push flag
#define get_move_double(move) ((move) & 0x200000)

// extract enpassant flag
#define get_move_enpassant(move) ((move) & 0x400000)

// extract castling flag
#define get_move_castling(move) ((move) & 0x800000)

//move list structure
typedef struct
{
    // moves
    int moves[256];

    // move count
    int count;

} moves_t;

/*
     These are the score bounds for the range of the mating scores
   [-infinity, -mate_value ... -mate_score, ... score ... mate_score ... mate_value, infinity]
*/

#define infinity 50000
#define mate_value 49000
#define mate_score 48000

// no hash entry found constant
#define no_hash_entry 100000

// transposition table hash flags
#define hash_flag_exact 0
#define hash_flag_alpha 1
#define hash_flag_beta 2

// transposition table data structure
typedef struct
{
    U64 hash_key; // "almost" unique chess position identifier
    int depth;    // current search depth
    int flag;     // flag the type of node (fail-low/fail-high/PV)
    int score;    // score (alpha/beta/PV)
} tt;             // transposition table (TT aka hash table)

// max ply that we can reach within a search
#define max_ply 64

// board squares
enum
{
    a8,
    b8,
    c8,
    d8,
    e8,
    f8,
    g8,
    h8,
    a7,
    b7,
    c7,
    d7,
    e7,
    f7,
    g7,
    h7,
    a6,
    b6,
    c6,
    d6,
    e6,
    f6,
    g6,
    h6,
    a5,
    b5,
    c5,
    d5,
    e5,
    f5,
    g5,
    h5,
    a4,
    b4,
    c4,
    d4,
    e4,
    f4,
    g4,
    h4,
    a3,
    b3,
    c3,
    d3,
    e3,
    f3,
    g3,
    h3,
    a2,
    b2,
    c2,
    d2,
    e2,
    f2,
    g2,
    h2,
    a1,
    b1,
    c1,
    d1,
    e1,
    f1,
    g1,
    h1,
    no_sq
};

// encode pieces
enum
{
    P,
    N,
    B,
    R,
    Q,
    K,
    p,
    n,
    b,
    r,
    q,
    k
};

// sides to move (colors)
enum
{
    white,
    black,
    both
};

// bishop and rook
enum
{
    rook,
    bishop
};

// castling rights binary encoding
enum
{
    wk = 1,
    wq = 2,
    bk = 4,
    bq = 8
};

// move types
enum
{
    all_moves,
    only_captures
};

// game phases
enum
{
    opening,
    endgame,
    middlegame
};

// piece types
enum
{
    PAWN,
    KNIGHT,
    BISHOP,
    ROOK,
    QUEEN,
    KING
};

// -----------------------------------------------------------------------
// Left board BBC code (prefix lb_)
// Right board BBC code (prefix rb_)
// -----------------------------------------------------------------------

// convert squares to coordinates
const char *lb_square_to_coordinates[] = {
    "a8",
    "b8",
    "c8",
    "d8",
    "e8",
    "f8",
    "g8",
    "h8",
    "a7",
    "b7",
    "c7",
    "d7",
    "e7",
    "f7",
    "g7",
    "h7",
    "a6",
    "b6",
    "c6",
    "d6",
    "e6",
    "f6",
    "g6",
    "h6",
    "a5",
    "b5",
    "c5",
    "d5",
    "e5",
    "f5",
    "g5",
    "h5",
    "a4",
    "b4",
    "c4",
    "d4",
    "e4",
    "f4",
    "g4",
    "h4",
    "a3",
    "b3",
    "c3",
    "d3",
    "e3",
    "f3",
    "g3",
    "h3",
    "a2",
    "b2",
    "c2",
    "d2",
    "e2",
    "f2",
    "g2",
    "h2",
    "a1",
    "b1",
    "c1",
    "d1",
    "e1",
    "f1",
    "g1",
    "h1",
};
const char *rb_square_to_coordinates[] = {
    "a8",
    "b8",
    "c8",
    "d8",
    "e8",
    "f8",
    "g8",
    "h8",
    "a7",
    "b7",
    "c7",
    "d7",
    "e7",
    "f7",
    "g7",
    "h7",
    "a6",
    "b6",
    "c6",
    "d6",
    "e6",
    "f6",
    "g6",
    "h6",
    "a5",
    "b5",
    "c5",
    "d5",
    "e5",
    "f5",
    "g5",
    "h5",
    "a4",
    "b4",
    "c4",
    "d4",
    "e4",
    "f4",
    "g4",
    "h4",
    "a3",
    "b3",
    "c3",
    "d3",
    "e3",
    "f3",
    "g3",
    "h3",
    "a2",
    "b2",
    "c2",
    "d2",
    "e2",
    "f2",
    "g2",
    "h2",
    "a1",
    "b1",
    "c1",
    "d1",
    "e1",
    "f1",
    "g1",
    "h1",
};

// ASCII pieces
char lb_ascii_pieces[12] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'};
char rb_ascii_pieces[12] = {'P', 'N', 'B', 'R', 'Q', 'K', 'p', 'n', 'b', 'r', 'q', 'k'};

// unicode pieces
char *lb_unicode_pieces[12] = {"♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚"};
char *rb_unicode_pieces[12] = {"♙", "♘", "♗", "♖", "♕", "♔", "♟︎", "♞", "♝", "♜", "♛", "♚"};

// convert ASCII character pieces to encoded constants
int lb_char_pieces[] = {
    ['P'] = P,
    ['N'] = N,
    ['B'] = B,
    ['R'] = R,
    ['Q'] = Q,
    ['K'] = K,
    ['p'] = p,
    ['n'] = n,
    ['b'] = b,
    ['r'] = r,
    ['q'] = q,
    ['k'] = k};
int rb_char_pieces[] = {
    ['P'] = P,
    ['N'] = N,
    ['B'] = B,
    ['R'] = R,
    ['Q'] = Q,
    ['K'] = K,
    ['p'] = p,
    ['n'] = n,
    ['b'] = b,
    ['r'] = r,
    ['q'] = q,
    ['k'] = k};

// promoted pieces
char lb_promoted_pieces[] = {
    [Q] = 'q',
    [R] = 'r',
    [B] = 'b',
    [N] = 'n',
    [q] = 'q',
    [r] = 'r',
    [b] = 'b',
    [n] = 'n'};
char rb_promoted_pieces[] = {
    [Q] = 'q',
    [R] = 'r',
    [B] = 'b',
    [N] = 'n',
    [q] = 'q',
    [r] = 'r',
    [b] = 'b',
    [n] = 'n'};

/**********************************\
 ==================================
 
            Chess board
 
 ==================================
\**********************************/

/*
                            WHITE PIECES


        Pawns                  Knights              Bishops

  8  0 0 0 0 0 0 0 0    8  0 0 0 0 0 0 0 0    8  0 0 0 0 0 0 0 0
  7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0
  2  1 1 1 1 1 1 1 1    2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0
  1  0 0 0 0 0 0 0 0    1  0 1 0 0 0 0 1 0    1  0 0 1 0 0 1 0 0

     a b c d e f g h       a b c d e f g h       a b c d e f g h


         Rooks                 Queens                 King

  8  0 0 0 0 0 0 0 0    8  0 0 0 0 0 0 0 0    8  0 0 0 0 0 0 0 0
  7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0
  2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0
  1  1 0 0 0 0 0 0 1    1  0 0 0 1 0 0 0 0    1  0 0 0 0 1 0 0 0

     a b c d e f g h       a b c d e f g h       a b c d e f g h


                            BLACK PIECES


        Pawns                  Knights              Bishops

  8  0 0 0 0 0 0 0 0    8  0 1 0 0 0 0 1 0    8  0 0 1 0 0 1 0 0
  7  1 1 1 1 1 1 1 1    7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0
  2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0
  1  0 0 0 0 0 0 0 0    1  0 0 0 0 0 0 0 0    1  0 0 0 0 0 0 0 0

     a b c d e f g h       a b c d e f g h       a b c d e f g h


         Rooks                 Queens                 King

  8  1 0 0 0 0 0 0 1    8  0 0 0 1 0 0 0 0    8  0 0 0 0 1 0 0 0
  7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0
  2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 0 0 0
  1  0 0 0 0 0 0 0 0    1  0 0 0 0 0 0 0 0    1  0 0 0 0 0 0 0 0

     a b c d e f g h       a b c d e f g h       a b c d e f g h



                             OCCUPANCIES


     White occupancy       Black occupancy       All occupancies

  8  0 0 0 0 0 0 0 0    8  1 1 1 1 1 1 1 1    8  1 1 1 1 1 1 1 1
  7  0 0 0 0 0 0 0 0    7  1 1 1 1 1 1 1 1    7  1 1 1 1 1 1 1 1
  6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0    6  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 0 0 0
  2  1 1 1 1 1 1 1 1    2  0 0 0 0 0 0 0 0    2  1 1 1 1 1 1 1 1
  1  1 1 1 1 1 1 1 1    1  0 0 0 0 0 0 0 0    1  1 1 1 1 1 1 1 1



                            ALL TOGETHER

                        8  ♜ ♞ ♝ ♛ ♚ ♝ ♞ ♜
                        7  ♟︎ ♟︎ ♟︎ ♟︎ ♟︎ ♟︎ ♟︎ ♟︎
                        6  . . . . . . . .
                        5  . . . . . . . .
                        4  . . . . . . . .
                        3  . . . . . . . .
                        2  ♙ ♙ ♙ ♙ ♙ ♙ ♙ ♙
                        1  ♖ ♘ ♗ ♕ ♔ ♗ ♘ ♖

                           a b c d e f g h

*/

// piece bitboards
U64 lb_bitboards[12];
U64 rb_bitboards[12];

// occupancy bitboards
U64 lb_occupancies[3];
U64 rb_occupancies[3];

// side to move
int lb_side;
int rb_side;

// enpassant square
int lb_enpassant = no_sq;
int rb_enpassant = no_sq;

// castling rights
int lb_castle;
int rb_castle;

// "almost" unique position identifier aka hash key or position key
U64 lb_hash_key;
U64 rb_hash_key;

// positions repetition table
// 1000 is a number of plies (500 moves) in the entire game
U64 lb_repetition_table[1000]; 
U64 rb_repetition_table[1000];

// repetition index
int lb_repetition_index;
int rb_repetition_index;

// half move counter
int lb_ply;
int rb_ply;

// fifty move rule counter
int lb_fifty;
int rb_fifty;

///<ADD>
// passthrough varibales

// captured pieces from the other board [white + black]
int lb_cap_pieces[64];
int rb_cap_pieces[64];

// count captured pieces [white + black]
int lb_cap_pieces_count;
int rb_cap_pieces_count;

// captured pieces for passthrough to the other board
// preventing collision with the captured pieces [white + black]
int lb_pt_pieces[64];
int rb_pt_pieces[64];

// count pass through pieces; [white + black]
int lb_pt_pieces_count;
int rb_pt_pieces_count;

// keep track of promoted pawns, when a promoted pawn is captured then the passthrough will be a pawn not the piece
U64 lb_promoted_bitboards[2];
U64 rb_promoted_bitboards[2];

// flag that indicates the end of the program to force the thread to stop
int lb_stop_game_flag;
int rb_stop_game_flag;

// flag that indicates if the thread (calculating an AI move) is busy
int lb_thread_busy;
int rb_thread_busy;
///</ADD>

/**********************************\
 ==================================
 
       Time controls variables
 
 ==================================
\**********************************/

// exit from engine flag
int lb_quit = 0;
int rb_quit = 0;

// UCI "movestogo" command moves counter
int lb_movestogo = 30;
int rb_movestogo = 30;

// UCI "movetime" command time counter
int lb_movetime = -1;
int rb_movetime = -1;

// UCI "time" command holder (ms)
int lb_ucitime = -1;
int rb_ucitime = -1;

// UCI "inc" command's time increment holder
int lb_inc = 0;
int rb_inc = 0;

// UCI "starttime" command time holder
int lb_starttime = 0;
int rb_starttime = 0;

// UCI "stoptime" command time holder
int lb_stoptime = 0;
int rb_stoptime = 0;

// variable to flag time control availability
int lb_timeset = 0;
int rb_timeset = 0;

// variable to flag when the time is up
int lb_stopped = 0;
int rb_stopped = 0;

/**********************************\
 ==================================
 
       Miscellaneous functions
          forked from VICE
         by Richard Allbert
 
 ==================================
\**********************************/

// get time in milliseconds
int lb_get_time_ms()
{
    struct timeval time_value;
    gettimeofday(&time_value, NULL);
    return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
}

// get time in milliseconds
int rb_get_time_ms()
{
    struct timeval time_value;
    gettimeofday(&time_value, NULL);
    return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
}

// a bridge function to interact between search and GUI input
static void lb_communicate()
{
    // if time is up break here
    ///<ADD> or if the gui is stopped 
    if ((lb_timeset == 1 && lb_get_time_ms() > lb_stoptime) || lb_stop_game_flag) ///</ADD>
    {
        // tell engine to stop calculating
        lb_stopped = 1;
    }

    // not used
    /*
    // read GUI input
    lb_read_input();
    */
}

// a bridge function to interact between search and GUI input
static void rb_communicate()
{
    // if time is up break here
    ///<ADD> or if the gui is stopped 
    if ((rb_timeset == 1 && rb_get_time_ms() > rb_stoptime) || rb_stop_game_flag) ///</ADD>
    {
        // tell engine to stop calculating
        rb_stopped = 1;
    }

    // not used
    /*
    // read GUI input
    // rb_read_input();
    */
}

/**********************************\
 ==================================
 
           Random numbers
 
 ==================================
\**********************************/

// pseudo random number state
unsigned int lb_random_state = 1804289383;
unsigned int rb_random_state = 1804289383;

// generate 32-bit pseudo legal numbers
unsigned int lb_get_random_U32_number()
{
    // get current state
    unsigned int number = lb_random_state;

    // XOR shift algorithm
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    // update random number state
    lb_random_state = number;

    // return random number
    return number;
}

unsigned int rb_get_random_U32_number()
{
    // get current state
    unsigned int number = rb_random_state;

    // XOR shift algorithm
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    // update random number state
    rb_random_state = number;

    // return random number
    return number;
}

// generate 64-bit pseudo legal numbers
U64 lb_get_random_U64_number()
{
    // define 4 random numbers
    U64 n1, n2, n3, n4;

    // init random numbers slicing 16 bits from MS1B side
    n1 = (U64)(lb_get_random_U32_number()) & 0xFFFF;
    n2 = (U64)(lb_get_random_U32_number()) & 0xFFFF;
    n3 = (U64)(lb_get_random_U32_number()) & 0xFFFF;
    n4 = (U64)(lb_get_random_U32_number()) & 0xFFFF;

    // return random number
    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

U64 rb_get_random_U64_number()
{
    // define 4 random numbers
    U64 n1, n2, n3, n4;

    // init random numbers slicing 16 bits from MS1B side
    n1 = (U64)(rb_get_random_U32_number()) & 0xFFFF;
    n2 = (U64)(rb_get_random_U32_number()) & 0xFFFF;
    n3 = (U64)(rb_get_random_U32_number()) & 0xFFFF;
    n4 = (U64)(rb_get_random_U32_number()) & 0xFFFF;

    // return random number
    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

// generate magic number candidate
U64 lb_generate_magic_number()
{
    return lb_get_random_U64_number() & lb_get_random_U64_number() & lb_get_random_U64_number();
}

U64 rb_generate_magic_number()
{
    return rb_get_random_U64_number() & rb_get_random_U64_number() & rb_get_random_U64_number();
}

/**********************************\
 ==================================
 
          Bit manipulations
 
 ==================================
\**********************************/

// count bits within a bitboard (Brian Kernighan's way)
static inline int lb_count_bits(U64 bitboard)
{
    // bit counter
    int count = 0;

    // consecutively reset least significant 1st bit
    while (bitboard)
    {
        // increment count
        count++;

        // reset least significant 1st bit
        bitboard &= bitboard - 1;
    }

    // return bit count
    return count;
}

static inline int rb_count_bits(U64 bitboard)
{
    // bit counter
    int count = 0;

    // consecutively reset least significant 1st bit
    while (bitboard)
    {
        // increment count
        count++;

        // reset least significant 1st bit
        bitboard &= bitboard - 1;
    }

    // return bit count
    return count;
}

// get least significant 1st bit index
static inline int lb_get_ls1b_index(U64 bitboard)
{
    // make sure bitboard is not 0
    if (bitboard)
    {
        // count trailing bits before LS1B
        return lb_count_bits((bitboard & -bitboard) - 1);
    }
    // otherwise
    else
        // return illegal index
        return -1;
}

static inline int rb_get_ls1b_index(U64 bitboard)
{
    // make sure bitboard is not 0
    if (bitboard)
    {
        // count trailing bits before LS1B
        return rb_count_bits((bitboard & -bitboard) - 1);
    }
    // otherwise
    else
        // return illegal index
        return -1;
}

/**********************************\
 ==================================
 
            Zobrist keys
 
 ==================================
\**********************************/

// random piece keys [piece][square]
U64 lb_piece_keys[12][64];
U64 rb_piece_keys[12][64];

// random enpassant keys [square]
U64 lb_enpassant_keys[64];
U64 rb_enpassant_keys[64];

// random castling keys
U64 lb_castle_keys[16];
U64 rb_castle_keys[16];

// random side key
U64 lb_side_key;
U64 rb_side_key;

// init random hash keys
void lb_init_random_keys()
{
    // update pseudo random number state
    lb_random_state = 1804289383;

    // loop over piece codes
    for (int piece = P; piece <= k; piece++)
    {
        // loop over board squares
        for (int square = 0; square < 64; square++)
            // init random piece keys
            lb_piece_keys[piece][square] = lb_get_random_U64_number();
    }

    // loop over board squares
    for (int square = 0; square < 64; square++)
        // init random enpassant keys
        lb_enpassant_keys[square] = lb_get_random_U64_number();

    // loop over castling keys
    for (int index = 0; index < 16; index++)
        // init castling keys
        lb_castle_keys[index] = lb_get_random_U64_number();

    // init random side key
    lb_side_key = lb_get_random_U64_number();
}

void rb_init_random_keys()
{
    // update pseudo random number state
    rb_random_state = 1804289383;

    // loop over piece codes
    for (int piece = P; piece <= k; piece++)
    {
        // loop over board squares
        for (int square = 0; square < 64; square++)
            // init random piece keys
            rb_piece_keys[piece][square] = rb_get_random_U64_number();
    }

    // loop over board squares
    for (int square = 0; square < 64; square++)
        // init random enpassant keys
        rb_enpassant_keys[square] = rb_get_random_U64_number();

    // loop over castling keys
    for (int index = 0; index < 16; index++)
        // init castling keys
        rb_castle_keys[index] = rb_get_random_U64_number();

    // init random side key
    rb_side_key = rb_get_random_U64_number();
}

// generate "almost" unique position ID aka hash key from scratch
U64 lb_generate_hash_key()
{
    // final hash key
    U64 final_key = 0ULL;

    // temp piece bitboard copy
    U64 bitboard;

    // loop over piece bitboards
    for (int piece = P; piece <= k; piece++)
    {
        // init piece bitboard copy
        bitboard = lb_bitboards[piece];

        // loop over the pieces within a bitboard
        while (bitboard)
        {
            // init square occupied by the piece
            int square = lb_get_ls1b_index(bitboard);

            // hash piece
            final_key ^= lb_piece_keys[piece][square];

            // pop LS1B
            pop_bit(bitboard, square);
        }
    }

    // if enpassant square is on board
    if (lb_enpassant != no_sq)
        // hash enpassant
        final_key ^= lb_enpassant_keys[lb_enpassant];

    // hash castling rights
    final_key ^= lb_castle_keys[lb_castle];

    // hash the side only if black is to move
    if (lb_side == black)
        final_key ^= lb_side_key;

    // return generated hash key
    return final_key;
}

U64 rb_generate_hash_key()
{
    // final hash key
    U64 final_key = 0ULL;

    // temp piece bitboard copy
    U64 bitboard;

    // loop over piece bitboards
    for (int piece = P; piece <= k; piece++)
    {
        // init piece bitboard copy
        bitboard = rb_bitboards[piece];

        // loop over the pieces within a bitboard
        while (bitboard)
        {
            // init square occupied by the piece
            int square = rb_get_ls1b_index(bitboard);

            // hash piece
            final_key ^= rb_piece_keys[piece][square];

            // pop LS1B
            pop_bit(bitboard, square);
        }
    }

    // if enpassant square is on board
    if (rb_enpassant != no_sq)
        // hash enpassant
        final_key ^= rb_enpassant_keys[rb_enpassant];

    // hash castling rights
    final_key ^= rb_castle_keys[rb_castle];

    // hash the side only if black is to move
    if (rb_side == black)
        final_key ^= rb_side_key;

    // return generated hash key
    return final_key;
}

/**********************************\
 ==================================
 
           Input & Output
 
 ==================================
\**********************************/

// print bitboard
void lb_print_bitboard(U64 bitboard)
{
#ifndef NDEBUG // print only in debug mode
    // print offset
    printf("\nlb\n");

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 8; file++)
        {
            // convert file & rank into square index
            int square = rank * 8 + file;

            // print ranks
            if (!file)
                printf("  %d ", 8 - rank);

            // print bit state (either 1 or 0)
            printf(" %d", get_bit(bitboard, square) ? 1 : 0);
        }

        // print new line every rank
        printf("\n");
    }

    // print board files
    printf("\n     a b c d e f g h\n\n");

    // print bitboard as unsigned decimal number
    printf("     Bitboard: %llud\n\n", bitboard);
#endif
}

void rb_print_bitboard(U64 bitboard)
{
#ifndef NDEBUG // print only in debug mode
    // print offset
    printf("\nrb\n");

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 8; file++)
        {
            // convert file & rank into square index
            int square = rank * 8 + file;

            // print ranks
            if (!file)
                printf("  %d ", 8 - rank);

            // print bit state (either 1 or 0)
            printf(" %d", get_bit(bitboard, square) ? 1 : 0);
        }

        // print new line every rank
        printf("\n");
    }

    // print board files
    printf("\n     a b c d e f g h\n\n");

    // print bitboard as unsigned decimal number
    printf("     Bitboard: %llud\n\n", bitboard);
#endif
}

// print board
void lb_print_board()
{
#ifndef NDEBUG // print only in debug mode
    // print offset
    printf("\nlb\n");

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop ober board files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // print ranks
            if (!file)
                printf("  %d ", 8 - rank);

            // define piece variable
            int piece = -1;

            // loop over all piece bitboards
            for (int bb_piece = P; bb_piece <= k; bb_piece++)
            {
                // if there is a piece on current square
                if (get_bit(lb_bitboards[bb_piece], square))
                    // get piece code
                    piece = bb_piece;
            }

// print different piece set depending on OS
#ifdef _WIN64
            printf(" %c", (piece == -1) ? '.' : lb_ascii_pieces[piece]);
#else
            printf(" %s", (piece == -1) ? "." : lb_unicode_pieces[piece]);
#endif
        }

        // print new line every rank
        printf("\n");
    }

    // print board files
    printf("\n     a b c d e f g h\n\n");

    // print side to move
    printf("     Side:     %s\n", !lb_side ? "white" : "black");

    // print enpassant square
    printf("     Enpassant:   %s\n", (lb_enpassant != no_sq) ? lb_square_to_coordinates[lb_enpassant] : "no");

    // print castling rights
    printf("     Castling:  %c%c%c%c\n\n", (lb_castle & wk) ? 'K' : '-',
           (lb_castle & wq) ? 'Q' : '-',
           (lb_castle & bk) ? 'k' : '-',
           (lb_castle & bq) ? 'q' : '-');

    // fifty move rule counter
    printf("     50 moves rule: %d\n", (lb_fifty / 2));

    // print hash key
    printf("     Hash key:  %llx\n", lb_hash_key);
#endif
}

void rb_print_board()
{
#ifndef NDEBUG // print only in debug mode
    // print offset
    printf("\nrb\n");

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop ober board files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // print ranks
            if (!file)
                printf("  %d ", 8 - rank);

            // define piece variable
            int piece = -1;

            // loop over all piece bitboards
            for (int bb_piece = P; bb_piece <= k; bb_piece++)
            {
                // if there is a piece on current square
                if (get_bit(rb_bitboards[bb_piece], square))
                    // get piece code
                    piece = bb_piece;
            }

// print different piece set depending on OS
#ifdef _WIN64
            printf(" %c", (piece == -1) ? '.' : rb_ascii_pieces[piece]);
#else
            printf(" %s", (piece == -1) ? "." : rb_unicode_pieces[piece]);
#endif
        }

        // print new line every rank
        printf("\n");
    }

    // print board files
    printf("\n     a b c d e f g h\n\n");

    // print side to move
    printf("     Side:     %s\n", !rb_side ? "white" : "black");

    // print enpassant square
    printf("     Enpassant:   %s\n", (rb_enpassant != no_sq) ? rb_square_to_coordinates[rb_enpassant] : "no");

    // print castling rights
    printf("     Castling:  %c%c%c%c\n\n", (rb_castle & wk) ? 'K' : '-',
           (rb_castle & wq) ? 'Q' : '-',
           (rb_castle & bk) ? 'k' : '-',
           (rb_castle & bq) ? 'q' : '-');

    // fifty move rule counter
    printf("     50 moves rule: %d\n", (rb_fifty / 2));

    // print hash key
    printf("     Hash key:  %llx\n\n", rb_hash_key);
#endif
}

// reset board variables
void lb_reset_board()
{
    // reset board position (lb_bitboards)
    memset(lb_bitboards, 0ULL, sizeof(lb_bitboards));

    // reset occupancies (lb_bitboards)
    memset(lb_occupancies, 0ULL, sizeof(lb_occupancies));

    ///<ADD>
    // reset captured pieces
    memset(lb_cap_pieces, 0, sizeof(lb_cap_pieces));
    // reset counter
    lb_cap_pieces_count = 0;

    // reset passthrough pieces
    memset(lb_pt_pieces, 0, sizeof(lb_pt_pieces));
    // reset counter
    lb_pt_pieces_count = 0;

    // reset promoted bitboards
    memset(lb_promoted_bitboards, 0ULL, sizeof(lb_promoted_bitboards));
    ///</ADD>

    // reset game state variables
    lb_side = 0;
    lb_enpassant = no_sq;
    lb_castle = 0;
    // reset fifty move rule counter
    lb_fifty = 0;

    // reset repetition index
    lb_repetition_index = 0;

    // reset lb_repetition table
    memset(lb_repetition_table, 0ULL, sizeof(lb_repetition_table));
}

void rb_reset_board()
{
    // reset board position (bitboards)
    memset(rb_bitboards, 0ULL, sizeof(rb_bitboards));

    // reset occupancies (rb_bitboards)
    memset(rb_occupancies, 0ULL, sizeof(rb_occupancies));

    ///<ADD>
    // reset captured pieces
    memset(rb_cap_pieces, 0, sizeof(rb_cap_pieces));

    // reset counter
    rb_cap_pieces_count = 0;

    // reset passthrough pieces
    memset(rb_pt_pieces, 0, sizeof(rb_pt_pieces));

    // reset counter
    rb_pt_pieces_count = 0;

    // reset promote bitboards
    memset(rb_promoted_bitboards, 0ULL, sizeof(rb_promoted_bitboards));
    ///</ADD>

    // reset game state variables
    rb_side = 0;
    rb_enpassant = no_sq;
    rb_castle = 0;
    // reset fifty move rule counter
    rb_fifty = 0;

    // reset repetition index
    rb_repetition_index = 0;

    // reset rb_repetition table
    memset(rb_repetition_table, 0ULL, sizeof(rb_repetition_table));
}

// parse FEN string
void lb_parse_fen(char *fen)
{
    // prepare for new game
    lb_reset_board();

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 8; file++)
        {
            // init current square
            int square = rank * 8 + file;

            // match ascii pieces within FEN string
            if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z'))
            {
                // init piece type
                int piece = lb_char_pieces[*fen];

                // set piece on corresponding bitboard
                set_bit(lb_bitboards[piece], square);

                // increment pointer to FEN string
                fen++;
            }

            // match empty square numbers within FEN string
            if (*fen >= '0' && *fen <= '9')
            {
                // init offset (convert char 0 to int 0)
                int offset = *fen - '0';

                // define piece variable
                int piece = -1;

                // loop over all piece bitboards
                for (int bb_piece = P; bb_piece <= k; bb_piece++)
                {
                    // if there is a piece on current square
                    if (get_bit(lb_bitboards[bb_piece], square))
                        // get piece code
                        piece = bb_piece;
                }

                // on empty current square
                if (piece == -1)
                    // decrement file
                    file--;

                // adjust file counter
                file += offset;

                // increment pointer to FEN string
                fen++;
            }

            // match rank separator
            if (*fen == '/')
                // increment pointer to FEN string
                fen++;
        }
    }

    // got to parsing side to move (increment pointer to FEN string)
    fen++;

    // parse side to move
    (*fen == 'w') ? (lb_side = white) : (lb_side = black);

    // go to parsing castling rights
    fen += 2;

    // parse castling rights
    while (*fen != ' ')
    {
        switch (*fen)
        {
        case 'K':
            lb_castle |= wk;
            break;
        case 'Q':
            lb_castle |= wq;
            break;
        case 'k':
            lb_castle |= bk;
            break;
        case 'q':
            lb_castle |= bq;
            break;
        case '-':
            break;
        }

        // increment pointer to FEN string
        fen++;
    }

    // got to parsing enpassant square (increment pointer to FEN string)
    fen++;

    // parse enpassant square
    if (*fen != '-')
    {
        // parse enpassant file & rank
        int file = fen[0] - 'a';
        int rank = 8 - (fen[1] - '0');

        // init enpassant square
        lb_enpassant = rank * 8 + file;
    }

    // no enpassant square
    else
        lb_enpassant = no_sq;

    // go to parsing half move counter (increment pointer to FEN string)
    fen++;

    // parse half move counter to init fifty move counter
    lb_fifty = atoi(fen);

    // loop over white pieces bitboards
    for (int piece = P; piece <= K; piece++)
        // populate white occupancy bitboard
        lb_occupancies[white] |= lb_bitboards[piece];

    // loop over black pieces bitboards
    for (int piece = p; piece <= k; piece++)
        // populate white occupancy bitboard
        lb_occupancies[black] |= lb_bitboards[piece];

    // init all occupancies
    lb_occupancies[both] |= lb_occupancies[white];
    lb_occupancies[both] |= lb_occupancies[black];

    // init hash key
    lb_hash_key = lb_generate_hash_key();
}

// parse FEN string
void rb_parse_fen(char *fen)
{
    // prepare for new game
    rb_reset_board();

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 8; file++)
        {
            // init current square
            int square = rank * 8 + file;

            // match ascii pieces within FEN string
            if ((*fen >= 'a' && *fen <= 'z') || (*fen >= 'A' && *fen <= 'Z'))
            {
                // init piece type
                int piece = rb_char_pieces[*fen];

                // set piece on corresponding bitboard
                set_bit(rb_bitboards[piece], square);

                // increment pointer to FEN string
                fen++;
            }

            // match empty square numbers within FEN string
            if (*fen >= '0' && *fen <= '9')
            {
                // init offset (convert char 0 to int 0)
                int offset = *fen - '0';

                // define piece variable
                int piece = -1;

                // loop over all piece bitboards
                for (int bb_piece = P; bb_piece <= k; bb_piece++)
                {
                    // if there is a piece on current square
                    if (get_bit(rb_bitboards[bb_piece], square))
                        // get piece code
                        piece = bb_piece;
                }

                // on empty current square
                if (piece == -1)
                    // decrement file
                    file--;

                // adjust file counter
                file += offset;

                // increment pointer to FEN string
                fen++;
            }

            // match rank separator
            if (*fen == '/')
                // increment pointer to FEN string
                fen++;
        }
    }

    // got to parsing side to move (increment pointer to FEN string)
    fen++;

    // parse side to move
    (*fen == 'w') ? (rb_side = white) : (rb_side = black);

    // go to parsing castling rights
    fen += 2;

    // parse castling rights
    while (*fen != ' ')
    {
        switch (*fen)
        {
        case 'K':
            rb_castle |= wk;
            break;
        case 'Q':
            rb_castle |= wq;
            break;
        case 'k':
            rb_castle |= bk;
            break;
        case 'q':
            rb_castle |= bq;
            break;
        case '-':
            break;
        }

        // increment pointer to FEN string
        fen++;
    }

    // got to parsing enpassant square (increment pointer to FEN string)
    fen++;

    // parse enpassant square
    if (*fen != '-')
    {
        // parse enpassant file & rank
        int file = fen[0] - 'a';
        int rank = 8 - (fen[1] - '0');

        // init enpassant square
        rb_enpassant = rank * 8 + file;
    }

    // no enpassant square
    else
        rb_enpassant = no_sq;

    // go to parsing half move counter (increment pointer to FEN string)
    fen++;

    // parse half move counter to init fifty move counter
    rb_fifty = atoi(fen);

    // loop over white pieces bitboards
    for (int piece = P; piece <= K; piece++)
        // populate white occupancy bitboard
        rb_occupancies[white] |= rb_bitboards[piece];

    // loop over black pieces bitboards
    for (int piece = p; piece <= k; piece++)
        // populate white occupancy bitboard
        rb_occupancies[black] |= rb_bitboards[piece];

    // init all occupancies
    rb_occupancies[both] |= rb_occupancies[white];
    rb_occupancies[both] |= rb_occupancies[black];

    // init hash key
    rb_hash_key = rb_generate_hash_key();
}

/**********************************\
 ==================================
 
              Attacks
 
 ==================================
\**********************************/

/*
     not A file          not H file         not HG files      not AB files
      bitboard            bitboard            bitboard          bitboard

 8  0 1 1 1 1 1 1 1    1 1 1 1 1 1 1 0    1 1 1 1 1 1 0 0    0 0 1 1 1 1 1 1
 7  0 1 1 1 1 1 1 1    1 1 1 1 1 1 1 0    1 1 1 1 1 1 0 0    0 0 1 1 1 1 1 1
 6  0 1 1 1 1 1 1 1    1 1 1 1 1 1 1 0    1 1 1 1 1 1 0 0    0 0 1 1 1 1 1 1
 5  0 1 1 1 1 1 1 1    1 1 1 1 1 1 1 0    1 1 1 1 1 1 0 0    0 0 1 1 1 1 1 1
 4  0 1 1 1 1 1 1 1    1 1 1 1 1 1 1 0    1 1 1 1 1 1 0 0    0 0 1 1 1 1 1 1
 3  0 1 1 1 1 1 1 1    1 1 1 1 1 1 1 0    1 1 1 1 1 1 0 0    0 0 1 1 1 1 1 1
 2  0 1 1 1 1 1 1 1    1 1 1 1 1 1 1 0    1 1 1 1 1 1 0 0    0 0 1 1 1 1 1 1
 1  0 1 1 1 1 1 1 1    1 1 1 1 1 1 1 0    1 1 1 1 1 1 0 0    0 0 1 1 1 1 1 1

    a b c d e f g h    a b c d e f g h    a b c d e f g h    a b c d e f g h

*/

// not A file constant
const U64 lb_not_a_file = 18374403900871474942ULL;
const U64 rb_not_a_file = 18374403900871474942ULL;

// not H file constant
const U64 lb_not_h_file = 9187201950435737471ULL;
const U64 rb_not_h_file = 9187201950435737471ULL;

// not HG file constant
const U64 lb_not_hg_file = 4557430888798830399ULL;
const U64 rb_not_hg_file = 4557430888798830399ULL;

// not AB file constant
const U64 lb_not_ab_file = 18229723555195321596ULL;
const U64 rb_not_ab_file = 18229723555195321596ULL;

// bishop relevant occupancy bit count for every square on board
const int lb_bishop_relevant_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6};
const int rb_bishop_relevant_bits[64] = {
    6, 5, 5, 5, 5, 5, 5, 6,
    5, 5, 5, 5, 5, 5, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 9, 9, 7, 5, 5,
    5, 5, 7, 7, 7, 7, 5, 5,
    5, 5, 5, 5, 5, 5, 5, 5,
    6, 5, 5, 5, 5, 5, 5, 6};

// rook relevant occupancy bit count for every square on board
const int lb_rook_relevant_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12};
const int rb_rook_relevant_bits[64] = {
    12, 11, 11, 11, 11, 11, 11, 12,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    11, 10, 10, 10, 10, 10, 10, 11,
    12, 11, 11, 11, 11, 11, 11, 12};

// rook magic numbers
U64 lb_rook_magic_numbers[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL};
U64 rb_rook_magic_numbers[64] = {
    0x8a80104000800020ULL,
    0x140002000100040ULL,
    0x2801880a0017001ULL,
    0x100081001000420ULL,
    0x200020010080420ULL,
    0x3001c0002010008ULL,
    0x8480008002000100ULL,
    0x2080088004402900ULL,
    0x800098204000ULL,
    0x2024401000200040ULL,
    0x100802000801000ULL,
    0x120800800801000ULL,
    0x208808088000400ULL,
    0x2802200800400ULL,
    0x2200800100020080ULL,
    0x801000060821100ULL,
    0x80044006422000ULL,
    0x100808020004000ULL,
    0x12108a0010204200ULL,
    0x140848010000802ULL,
    0x481828014002800ULL,
    0x8094004002004100ULL,
    0x4010040010010802ULL,
    0x20008806104ULL,
    0x100400080208000ULL,
    0x2040002120081000ULL,
    0x21200680100081ULL,
    0x20100080080080ULL,
    0x2000a00200410ULL,
    0x20080800400ULL,
    0x80088400100102ULL,
    0x80004600042881ULL,
    0x4040008040800020ULL,
    0x440003000200801ULL,
    0x4200011004500ULL,
    0x188020010100100ULL,
    0x14800401802800ULL,
    0x2080040080800200ULL,
    0x124080204001001ULL,
    0x200046502000484ULL,
    0x480400080088020ULL,
    0x1000422010034000ULL,
    0x30200100110040ULL,
    0x100021010009ULL,
    0x2002080100110004ULL,
    0x202008004008002ULL,
    0x20020004010100ULL,
    0x2048440040820001ULL,
    0x101002200408200ULL,
    0x40802000401080ULL,
    0x4008142004410100ULL,
    0x2060820c0120200ULL,
    0x1001004080100ULL,
    0x20c020080040080ULL,
    0x2935610830022400ULL,
    0x44440041009200ULL,
    0x280001040802101ULL,
    0x2100190040002085ULL,
    0x80c0084100102001ULL,
    0x4024081001000421ULL,
    0x20030a0244872ULL,
    0x12001008414402ULL,
    0x2006104900a0804ULL,
    0x1004081002402ULL};

// bishop magic numbers
U64 lb_bishop_magic_numbers[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL};
U64 rb_bishop_magic_numbers[64] = {
    0x40040844404084ULL,
    0x2004208a004208ULL,
    0x10190041080202ULL,
    0x108060845042010ULL,
    0x581104180800210ULL,
    0x2112080446200010ULL,
    0x1080820820060210ULL,
    0x3c0808410220200ULL,
    0x4050404440404ULL,
    0x21001420088ULL,
    0x24d0080801082102ULL,
    0x1020a0a020400ULL,
    0x40308200402ULL,
    0x4011002100800ULL,
    0x401484104104005ULL,
    0x801010402020200ULL,
    0x400210c3880100ULL,
    0x404022024108200ULL,
    0x810018200204102ULL,
    0x4002801a02003ULL,
    0x85040820080400ULL,
    0x810102c808880400ULL,
    0xe900410884800ULL,
    0x8002020480840102ULL,
    0x220200865090201ULL,
    0x2010100a02021202ULL,
    0x152048408022401ULL,
    0x20080002081110ULL,
    0x4001001021004000ULL,
    0x800040400a011002ULL,
    0xe4004081011002ULL,
    0x1c004001012080ULL,
    0x8004200962a00220ULL,
    0x8422100208500202ULL,
    0x2000402200300c08ULL,
    0x8646020080080080ULL,
    0x80020a0200100808ULL,
    0x2010004880111000ULL,
    0x623000a080011400ULL,
    0x42008c0340209202ULL,
    0x209188240001000ULL,
    0x400408a884001800ULL,
    0x110400a6080400ULL,
    0x1840060a44020800ULL,
    0x90080104000041ULL,
    0x201011000808101ULL,
    0x1a2208080504f080ULL,
    0x8012020600211212ULL,
    0x500861011240000ULL,
    0x180806108200800ULL,
    0x4000020e01040044ULL,
    0x300000261044000aULL,
    0x802241102020002ULL,
    0x20906061210001ULL,
    0x5a84841004010310ULL,
    0x4010801011c04ULL,
    0xa010109502200ULL,
    0x4a02012000ULL,
    0x500201010098b028ULL,
    0x8040002811040900ULL,
    0x28000010020204ULL,
    0x6000020202d0240ULL,
    0x8918844842082200ULL,
    0x4010011029020020ULL};

// pawn attacks table [side][square]
U64 lb_pawn_attacks[2][64];
U64 rb_pawn_attacks[2][64];

// knight attacks table [square]
U64 lb_knight_attacks[64];
U64 rb_knight_attacks[64];

// king attacks table [square]
U64 lb_king_attacks[64];
U64 rb_king_attacks[64];

// bishop attack masks
U64 lb_bishop_masks[64];
U64 rb_bishop_masks[64];

// rook attack masks
U64 lb_rook_masks[64];
U64 rb_rook_masks[64];

// bishop attacks table [square][occupancies]
U64 lb_bishop_attacks[64][512];
U64 rb_bishop_attacks[64][512];

// rook attacks rable [square][occupancies]
U64 lb_rook_attacks[64][4096];
U64 rb_rook_attacks[64][4096];

// generate pawn attacks
U64 lb_mask_pawn_attacks(int lb_side, int square)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // piece bitboard
    U64 bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    // white pawns
    if (!lb_side)
    {
        // generate pawn attacks
        if ((bitboard >> 7) & lb_not_a_file)
            attacks |= (bitboard >> 7);
        if ((bitboard >> 9) & lb_not_h_file)
            attacks |= (bitboard >> 9);
    }
    // black pawns
    else
    {
        // generate pawn attacks
        if ((bitboard << 7) & lb_not_h_file)
            attacks |= (bitboard << 7);
        if ((bitboard << 9) & lb_not_a_file)
            attacks |= (bitboard << 9);
    }

    // return attack map
    return attacks;
}

U64 rb_mask_pawn_attacks(int rb_side, int square)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // piece bitboard
    U64 bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    // white pawns
    if (!rb_side)
    {
        // generate pawn attacks
        if ((bitboard >> 7) & rb_not_a_file)
            attacks |= (bitboard >> 7);
        if ((bitboard >> 9) & rb_not_h_file)
            attacks |= (bitboard >> 9);
    }

    // black pawns
    else
    {
        // generate pawn attacks
        if ((bitboard << 7) & rb_not_h_file)
            attacks |= (bitboard << 7);
        if ((bitboard << 9) & rb_not_a_file)
            attacks |= (bitboard << 9);
    }

    // return attack map
    return attacks;
}

// generate knight attacks
U64 lb_mask_knight_attacks(int square)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // piece bitboard
    U64 bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    // generate knight attacks
    if ((bitboard >> 17) & lb_not_h_file)
        attacks |= (bitboard >> 17);
    if ((bitboard >> 15) & lb_not_a_file)
        attacks |= (bitboard >> 15);
    if ((bitboard >> 10) & lb_not_hg_file)
        attacks |= (bitboard >> 10);
    if ((bitboard >> 6) & lb_not_ab_file)
        attacks |= (bitboard >> 6);
    if ((bitboard << 17) & lb_not_a_file)
        attacks |= (bitboard << 17);
    if ((bitboard << 15) & lb_not_h_file)
        attacks |= (bitboard << 15);
    if ((bitboard << 10) & lb_not_ab_file)
        attacks |= (bitboard << 10);
    if ((bitboard << 6) & lb_not_hg_file)
        attacks |= (bitboard << 6);

    // return attack map
    return attacks;
}

U64 rb_mask_knight_attacks(int square)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // piece bitboard
    U64 bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    // generate knight attacks
    if ((bitboard >> 17) & rb_not_h_file)
        attacks |= (bitboard >> 17);
    if ((bitboard >> 15) & rb_not_a_file)
        attacks |= (bitboard >> 15);
    if ((bitboard >> 10) & rb_not_hg_file)
        attacks |= (bitboard >> 10);
    if ((bitboard >> 6) & rb_not_ab_file)
        attacks |= (bitboard >> 6);
    if ((bitboard << 17) & rb_not_a_file)
        attacks |= (bitboard << 17);
    if ((bitboard << 15) & rb_not_h_file)
        attacks |= (bitboard << 15);
    if ((bitboard << 10) & rb_not_ab_file)
        attacks |= (bitboard << 10);
    if ((bitboard << 6) & rb_not_hg_file)
        attacks |= (bitboard << 6);

    // return attack map
    return attacks;
}

// generate king attacks
U64 lb_mask_king_attacks(int square)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // piece bitboard
    U64 bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    // generate king attacks
    if (bitboard >> 8)
        attacks |= (bitboard >> 8);
    if ((bitboard >> 9) & lb_not_h_file)
        attacks |= (bitboard >> 9);
    if ((bitboard >> 7) & lb_not_a_file)
        attacks |= (bitboard >> 7);
    if ((bitboard >> 1) & lb_not_h_file)
        attacks |= (bitboard >> 1);
    if (bitboard << 8)
        attacks |= (bitboard << 8);
    if ((bitboard << 9) & lb_not_a_file)
        attacks |= (bitboard << 9);
    if ((bitboard << 7) & lb_not_h_file)
        attacks |= (bitboard << 7);
    if ((bitboard << 1) & lb_not_a_file)
        attacks |= (bitboard << 1);

    // return attack map
    return attacks;
}

U64 rb_mask_king_attacks(int square)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // piece bitboard
    U64 bitboard = 0ULL;

    // set piece on board
    set_bit(bitboard, square);

    // generate king attacks
    if (bitboard >> 8)
        attacks |= (bitboard >> 8);
    if ((bitboard >> 9) & rb_not_h_file)
        attacks |= (bitboard >> 9);
    if ((bitboard >> 7) & rb_not_a_file)
        attacks |= (bitboard >> 7);
    if ((bitboard >> 1) & rb_not_h_file)
        attacks |= (bitboard >> 1);
    if (bitboard << 8)
        attacks |= (bitboard << 8);
    if ((bitboard << 9) & rb_not_a_file)
        attacks |= (bitboard << 9);
    if ((bitboard << 7) & rb_not_h_file)
        attacks |= (bitboard << 7);
    if ((bitboard << 1) & rb_not_a_file)
        attacks |= (bitboard << 1);

    // return attack map
    return attacks;
}

// mask bishop attacks
U64 lb_mask_bishop_attacks(int square)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target rank & files
    int tr = square / 8;
    int tf = square % 8;

    // mask relevant bishop occupancy bits
    for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++)
        attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++)
        attacks |= (1ULL << (r * 8 + f));
    for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--)
        attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--)
        attacks |= (1ULL << (r * 8 + f));

    // return attack map
    return attacks;
}

U64 rb_mask_bishop_attacks(int square)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target rank & files
    int tr = square / 8;
    int tf = square % 8;

    // mask relevant bishop occupancy bits
    for (r = tr + 1, f = tf + 1; r <= 6 && f <= 6; r++, f++)
        attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf + 1; r >= 1 && f <= 6; r--, f++)
        attacks |= (1ULL << (r * 8 + f));
    for (r = tr + 1, f = tf - 1; r <= 6 && f >= 1; r++, f--)
        attacks |= (1ULL << (r * 8 + f));
    for (r = tr - 1, f = tf - 1; r >= 1 && f >= 1; r--, f--)
        attacks |= (1ULL << (r * 8 + f));

    // return attack map
    return attacks;
}

// mask rook attacks
U64 lb_mask_rook_attacks(int square)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target rank & files
    int tr = square / 8;
    int tf = square % 8;

    // mask relevant rook occupancy bits
    for (r = tr + 1; r <= 6; r++)
        attacks |= (1ULL << (r * 8 + tf));
    for (r = tr - 1; r >= 1; r--)
        attacks |= (1ULL << (r * 8 + tf));
    for (f = tf + 1; f <= 6; f++)
        attacks |= (1ULL << (tr * 8 + f));
    for (f = tf - 1; f >= 1; f--)
        attacks |= (1ULL << (tr * 8 + f));

    // return attack map
    return attacks;
}

U64 rb_mask_rook_attacks(int square)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target rank & files
    int tr = square / 8;
    int tf = square % 8;

    // mask relevant rook occupancy bits
    for (r = tr + 1; r <= 6; r++)
        attacks |= (1ULL << (r * 8 + tf));
    for (r = tr - 1; r >= 1; r--)
        attacks |= (1ULL << (r * 8 + tf));
    for (f = tf + 1; f <= 6; f++)
        attacks |= (1ULL << (tr * 8 + f));
    for (f = tf - 1; f >= 1; f--)
        attacks |= (1ULL << (tr * 8 + f));

    // return attack map
    return attacks;
}

// generate bishop attacks on the fly
U64 lb_bishop_attacks_on_the_fly(int square, U64 block)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target rank & files
    int tr = square / 8;
    int tf = square % 8;

    // generate bishop atacks
    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    // return attack map
    return attacks;
}

U64 rb_bishop_attacks_on_the_fly(int square, U64 block)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target rank & files
    int tr = square / 8;
    int tf = square % 8;

    // generate bishop atacks
    for (r = tr + 1, f = tf + 1; r <= 7 && f <= 7; r++, f++)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    for (r = tr - 1, f = tf + 1; r >= 0 && f <= 7; r--, f++)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    for (r = tr + 1, f = tf - 1; r <= 7 && f >= 0; r++, f--)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    for (r = tr - 1, f = tf - 1; r >= 0 && f >= 0; r--, f--)
    {
        attacks |= (1ULL << (r * 8 + f));
        if ((1ULL << (r * 8 + f)) & block)
            break;
    }

    // return attack map
    return attacks;
}

// generate rook attacks on the fly
U64 lb_rook_attacks_on_the_fly(int square, U64 block)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target rank & files
    int tr = square / 8;
    int tf = square % 8;

    // generate rook attacks
    for (r = tr + 1; r <= 7; r++)
    {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block)
            break;
    }

    for (r = tr - 1; r >= 0; r--)
    {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block)
            break;
    }

    for (f = tf + 1; f <= 7; f++)
    {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block)
            break;
    }

    for (f = tf - 1; f >= 0; f--)
    {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block)
            break;
    }

    // return attack map
    return attacks;
}

U64 rb_rook_attacks_on_the_fly(int square, U64 block)
{
    // result attacks bitboard
    U64 attacks = 0ULL;

    // init ranks & files
    int r, f;

    // init target rank & files
    int tr = square / 8;
    int tf = square % 8;

    // generate rook attacks
    for (r = tr + 1; r <= 7; r++)
    {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block)
            break;
    }

    for (r = tr - 1; r >= 0; r--)
    {
        attacks |= (1ULL << (r * 8 + tf));
        if ((1ULL << (r * 8 + tf)) & block)
            break;
    }

    for (f = tf + 1; f <= 7; f++)
    {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block)
            break;
    }

    for (f = tf - 1; f >= 0; f--)
    {
        attacks |= (1ULL << (tr * 8 + f));
        if ((1ULL << (tr * 8 + f)) & block)
            break;
    }

    // return attack map
    return attacks;
}

// init leaper pieces attacks
void lb_init_leapers_attacks()
{
    // loop over 64 board squares
    for (int square = 0; square < 64; square++)
    {
        // init pawn attacks
        lb_pawn_attacks[white][square] = lb_mask_pawn_attacks(white, square);
        lb_pawn_attacks[black][square] = lb_mask_pawn_attacks(black, square);

        // init knight attacks
        lb_knight_attacks[square] = lb_mask_knight_attacks(square);

        // init king attacks
        lb_king_attacks[square] = lb_mask_king_attacks(square);
    }
}

void rb_init_leapers_attacks()
{
    // loop over 64 board squares
    for (int square = 0; square < 64; square++)
    {
        // init pawn attacks
        rb_pawn_attacks[white][square] = rb_mask_pawn_attacks(white, square);
        rb_pawn_attacks[black][square] = rb_mask_pawn_attacks(black, square);

        // init knight attacks
        rb_knight_attacks[square] = rb_mask_knight_attacks(square);

        // init king attacks
        rb_king_attacks[square] = rb_mask_king_attacks(square);
    }
}

// set occupancies
U64 lb_set_occupancy(int index, int bits_in_mask, U64 attack_mask)
{
    // occupancy map
    U64 occupancy = 0ULL;

    // loop over the range of bits within attack mask
    for (int count = 0; count < bits_in_mask; count++)
    {
        // get LS1B index of attacks mask
        int square = lb_get_ls1b_index(attack_mask);

        // pop LS1B in attack map
        pop_bit(attack_mask, square);

        // make sure occupancy is on board
        if (index & (1 << count))
            // populate occupancy map
            occupancy |= (1ULL << square);
    }

    // return occupancy map
    return occupancy;
}

U64 rb_set_occupancy(int index, int bits_in_mask, U64 attack_mask)
{
    // occupancy map
    U64 occupancy = 0ULL;

    // loop over the range of bits within attack mask
    for (int count = 0; count < bits_in_mask; count++)
    {
        // get LS1B index of attacks mask
        int square = rb_get_ls1b_index(attack_mask);

        // pop LS1B in attack map
        pop_bit(attack_mask, square);

        // make sure occupancy is on board
        if (index & (1 << count))
            // populate occupancy map
            occupancy |= (1ULL << square);
    }

    // return occupancy map
    return occupancy;
}

/**********************************\
 ==================================
 
               Magics
 
 ==================================
\**********************************/

// init slider piece's attack tables
void lb_init_sliders_attacks(int bishop)
{
    // loop over 64 board squares
    for (int square = 0; square < 64; square++)
    {
        // init bishop & rook masks
        lb_bishop_masks[square] = lb_mask_bishop_attacks(square);
        lb_rook_masks[square] = lb_mask_rook_attacks(square);

        // init current mask
        U64 attack_mask = bishop ? lb_bishop_masks[square] : lb_rook_masks[square];

        // init relevant occupancy bit count
        int relevant_bits_count = lb_count_bits(attack_mask);

        // init occupancy indicies
        int occupancy_indicies = (1 << relevant_bits_count);

        // loop over occupancy indicies
        for (int index = 0; index < occupancy_indicies; index++)
        {
            // bishop
            if (bishop)
            {
                // init current occupancy variation
                U64 occupancy = lb_set_occupancy(index, relevant_bits_count, attack_mask);

                // init magic index
                int magic_index = (occupancy * lb_bishop_magic_numbers[square]) >> (64 - lb_bishop_relevant_bits[square]);

                // init bishop attacks
                lb_bishop_attacks[square][magic_index] = lb_bishop_attacks_on_the_fly(square, occupancy);
            }

            // rook
            else
            {
                // init current occupancy variation
                U64 occupancy = lb_set_occupancy(index, relevant_bits_count, attack_mask);

                // init magic index
                int magic_index = (occupancy * lb_rook_magic_numbers[square]) >> (64 - lb_rook_relevant_bits[square]);

                // init rook attacks
                lb_rook_attacks[square][magic_index] = lb_rook_attacks_on_the_fly(square, occupancy);
            }
        }
    }
}

void rb_init_sliders_attacks(int bishop)
{
    // loop over 64 board squares
    for (int square = 0; square < 64; square++)
    {
        // init bishop & rook masks
        rb_bishop_masks[square] = rb_mask_bishop_attacks(square);
        rb_rook_masks[square] = rb_mask_rook_attacks(square);

        // init current mask
        U64 attack_mask = bishop ? rb_bishop_masks[square] : rb_rook_masks[square];

        // init relevant occupancy bit count
        int relevant_bits_count = rb_count_bits(attack_mask);

        // init occupancy indicies
        int occupancy_indicies = (1 << relevant_bits_count);

        // loop over occupancy indicies
        for (int index = 0; index < occupancy_indicies; index++)
        {
            // bishop
            if (bishop)
            {
                // init current occupancy variation
                U64 occupancy = rb_set_occupancy(index, relevant_bits_count, attack_mask);

                // init magic index
                int magic_index = (occupancy * rb_bishop_magic_numbers[square]) >> (64 - rb_bishop_relevant_bits[square]);

                // init bishop attacks
                rb_bishop_attacks[square][magic_index] = rb_bishop_attacks_on_the_fly(square, occupancy);
            }

            // rook
            else
            {
                // init current occupancy variation
                U64 occupancy = rb_set_occupancy(index, relevant_bits_count, attack_mask);

                // init magic index
                int magic_index = (occupancy * rb_rook_magic_numbers[square]) >> (64 - rb_rook_relevant_bits[square]);

                // init rook attacks
                rb_rook_attacks[square][magic_index] = rb_rook_attacks_on_the_fly(square, occupancy);
            }
        }
    }
}

// get bishop attacks
static inline U64 lb_get_bishop_attacks(int square, U64 occupancy)
{
    // get bishop attacks assuming current board occupancy
    occupancy &= lb_bishop_masks[square];
    occupancy *= lb_bishop_magic_numbers[square];
    occupancy >>= 64 - lb_bishop_relevant_bits[square];

    // return bishop attacks
    return lb_bishop_attacks[square][occupancy];
}

static inline U64 rb_get_bishop_attacks(int square, U64 occupancy)
{
    // get bishop attacks assuming current board occupancy
    occupancy &= rb_bishop_masks[square];
    occupancy *= rb_bishop_magic_numbers[square];
    occupancy >>= 64 - rb_bishop_relevant_bits[square];

    // return bishop attacks
    return rb_bishop_attacks[square][occupancy];
}

// get rook attacks
static inline U64 lb_get_rook_attacks(int square, U64 occupancy)
{
    // get rook attacks assuming current board occupancy
    occupancy &= lb_rook_masks[square];
    occupancy *= lb_rook_magic_numbers[square];
    occupancy >>= 64 - lb_rook_relevant_bits[square];

    // return rook attacks
    return lb_rook_attacks[square][occupancy];
}

static inline U64 rb_get_rook_attacks(int square, U64 occupancy)
{
    // get rook attacks assuming current board occupancy
    occupancy &= rb_rook_masks[square];
    occupancy *= rb_rook_magic_numbers[square];
    occupancy >>= 64 - rb_rook_relevant_bits[square];

    // return rook attacks
    return rb_rook_attacks[square][occupancy];
}

// get queen attacks
static inline U64 lb_get_queen_attacks(int square, U64 occupancy)
{
    // init result attacks bitboard
    U64 queen_attacks = 0ULL;

    // init bishop occupancies
    U64 bishop_occupancy = occupancy;

    // init rook occupancies
    U64 rook_occupancy = occupancy;

    // get bishop attacks assuming current board occupancy
    bishop_occupancy &= lb_bishop_masks[square];
    bishop_occupancy *= lb_bishop_magic_numbers[square];
    bishop_occupancy >>= 64 - lb_bishop_relevant_bits[square];

    // get bishop attacks
    queen_attacks = lb_bishop_attacks[square][bishop_occupancy];

    // get rook attacks assuming current board occupancy
    rook_occupancy &= lb_rook_masks[square];
    rook_occupancy *= lb_rook_magic_numbers[square];
    rook_occupancy >>= 64 - lb_rook_relevant_bits[square];

    // get rook attacks
    queen_attacks |= lb_rook_attacks[square][rook_occupancy];

    // return queen attacks
    return queen_attacks;
}

static inline U64 rb_get_queen_attacks(int square, U64 occupancy)
{
    // init result attacks bitboard
    U64 queen_attacks = 0ULL;

    // init bishop occupancies
    U64 bishop_occupancy = occupancy;

    // init rook occupancies
    U64 rook_occupancy = occupancy;

    // get bishop attacks assuming current board occupancy
    bishop_occupancy &= rb_bishop_masks[square];
    bishop_occupancy *= rb_bishop_magic_numbers[square];
    bishop_occupancy >>= 64 - rb_bishop_relevant_bits[square];

    // get bishop attacks
    queen_attacks = rb_bishop_attacks[square][bishop_occupancy];

    // get rook attacks assuming current board occupancy
    rook_occupancy &= rb_rook_masks[square];
    rook_occupancy *= rb_rook_magic_numbers[square];
    rook_occupancy >>= 64 - rb_rook_relevant_bits[square];

    // get rook attacks
    queen_attacks |= rb_rook_attacks[square][rook_occupancy];

    // return queen attacks
    return queen_attacks;
}

/**********************************\
 ==================================
 
           Move generator
 
 ==================================
\**********************************/

// is square current given attacked by the current given side
static inline int lb_is_square_attacked(int square, int lb_side)
{
    // attacked by white pawns
    if ((lb_side == white) && (lb_pawn_attacks[black][square] & lb_bitboards[P]))
        return 1;

    // attacked by black pawns
    if ((lb_side == black) && (lb_pawn_attacks[white][square] & lb_bitboards[p]))
        return 1;

    // attacked by knights
    if (lb_knight_attacks[square] & ((lb_side == white) ? lb_bitboards[N] : lb_bitboards[n]))
        return 1;

    // attacked by bishops
    if (lb_get_bishop_attacks(square, lb_occupancies[both]) & ((lb_side == white) ? lb_bitboards[B] : lb_bitboards[b]))
        return 1;

    // attacked by rooks
    if (lb_get_rook_attacks(square, lb_occupancies[both]) & ((lb_side == white) ? lb_bitboards[R] : lb_bitboards[r]))
        return 1;

    // attacked by bishops
    if (lb_get_queen_attacks(square, lb_occupancies[both]) & ((lb_side == white) ? lb_bitboards[Q] : lb_bitboards[q]))
        return 1;

    // attacked by kings
    if (lb_king_attacks[square] & ((lb_side == white) ? lb_bitboards[K] : lb_bitboards[k]))
        return 1;

    // by default return 0
    return 0;
}

// is square current given attacked by the current given side
static inline int rb_is_square_attacked(int square, int rb_side)
{
    // attacked by white pawns
    if ((rb_side == white) && (rb_pawn_attacks[black][square] & rb_bitboards[P]))
        return 1;

    // attacked by black pawns
    if ((rb_side == black) && (rb_pawn_attacks[white][square] & rb_bitboards[p]))
        return 1;

    // attacked by knights
    if (rb_knight_attacks[square] & ((rb_side == white) ? rb_bitboards[N] : rb_bitboards[n]))
        return 1;

    // attacked by bishops
    if (rb_get_bishop_attacks(square, rb_occupancies[both]) & ((rb_side == white) ? rb_bitboards[B] : rb_bitboards[b]))
        return 1;

    // attacked by rooks
    if (rb_get_rook_attacks(square, rb_occupancies[both]) & ((rb_side == white) ? rb_bitboards[R] : rb_bitboards[r]))
        return 1;

    // attacked by bishops
    if (rb_get_queen_attacks(square, rb_occupancies[both]) & ((rb_side == white) ? rb_bitboards[Q] : rb_bitboards[q]))
        return 1;

    // attacked by kings
    if (rb_king_attacks[square] & ((rb_side == white) ? rb_bitboards[K] : rb_bitboards[k]))
        return 1;

    // by default return 0
    return 0;
}

// print attacked squares
void lb_print_attacked_squares(int lb_side)
{
#ifndef NDEBUG // print only in debug mode
    printf("\nlb\n");

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // print ranks
            if (!file)
                printf("  %d ", 8 - rank);

            // check whether current square is attacked or not
            printf(" %d", lb_is_square_attacked(square, lb_side) ? 1 : 0);
        }

        // print new line every rank
        printf("\n");
    }

    // print files
    printf("\n     a b c d e f g h\n\n");
#endif
}

void rb_print_attacked_squares(int rb_side)
{
#ifndef NDEBUG // print only in debug mode
    printf("\nrb\n");

    // loop over board ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over board files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // print ranks
            if (!file)
                printf("  %d ", 8 - rank);

            // check whether current square is attacked or not
            printf(" %d", rb_is_square_attacked(square, rb_side) ? 1 : 0);
        }

        // print new line every rank
        printf("\n");
    }

    // print files
    printf("\n     a b c d e f g h\n\n");
#endif
}

// add move to the move list
static inline void lb_add_move(moves_t *move_list, int move)
{
    ///<ADD>
    if (move_list->count < 255)
    {
    ///</ADD>
        // strore move
        move_list->moves[move_list->count] = move;

        // increment move count
        move_list->count++;
    ///<ADD>
    }
    else
        PrintAssert(move_list->count < 255);
    ///</ADD>
}

static inline void rb_add_move(moves_t *move_list, int move)
{
    ///<ADD>
    if (move_list->count < 255)
    {
    ///</ADD>
        // strore move
        move_list->moves[move_list->count] = move;

        // increment move count
        move_list->count++;
    ///<ADD>
    }
    else
        PrintAssert(move_list->count < 255);
    ///</ADD>
}

// print move (for UCI purposes)
void lb_print_move(int move)
{
#ifndef NDEBUG // print only in debug mode
    if (get_move_promoted(move))
        printf("%s%s%c", lb_square_to_coordinates[get_move_source(move)],
               lb_square_to_coordinates[get_move_target(move)],
               lb_promoted_pieces[get_move_promoted(move)]);
    else
        printf("%s%s", lb_square_to_coordinates[get_move_source(move)],
               lb_square_to_coordinates[get_move_target(move)]);
#endif
}

void rb_print_move(int move)
{
#ifndef NDEBUG // print only in debug mode
    if (get_move_promoted(move))
        printf("%s%s%c", rb_square_to_coordinates[get_move_source(move)],
               rb_square_to_coordinates[get_move_target(move)],
               rb_promoted_pieces[get_move_promoted(move)]);
    else
        printf("%s%s", rb_square_to_coordinates[get_move_source(move)],
               rb_square_to_coordinates[get_move_target(move)]);
#endif
}

// print move list
void lb_print_move_list(moves_t *move_list)
{
#ifndef NDEBUG // print only in debug mode
    // do nothing on empty move list
    if (!move_list->count)
    {
        printf("\nlb\n     No move in the move list!\n");
        return;
    }

    printf("\nlb\n     move    piece     capture   double    enpass    castling\n\n");

    // loop over moves within a move list
    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        // init move
        int move = move_list->moves[move_count];

#ifdef _WIN64
        // print move
        printf("     %s%s%c   %c         %d         %d         %d         %d\n", lb_square_to_coordinates[get_move_source(move)],
               lb_square_to_coordinates[get_move_target(move)],
               get_move_promoted(move) ? lb_promoted_pieces[get_move_promoted(move)] : ' ',
               lb_ascii_pieces[get_move_piece(move)],
               get_move_capture(move) ? 1 : 0,
               get_move_double(move) ? 1 : 0,
               get_move_enpassant(move) ? 1 : 0,
               get_move_castling(move) ? 1 : 0);
#else
        // print move
        printf("    %s%s%c   %s         %d         %d         %d         %d\n", lb_square_to_coordinates[get_move_source(move)],
               lb_square_to_coordinates[get_move_target(move)],
               get_move_promoted(move) ? lb_promoted_pieces[get_move_promoted(move)] : ' ',
               lb_unicode_pieces[get_move_piece(move)],
               get_move_capture(move) ? 1 : 0,
               get_move_double(move) ? 1 : 0,
               get_move_enpassant(move) ? 1 : 0,
               get_move_castling(move) ? 1 : 0);
#endif
    }

    // print total number of moves
    printf("\n\n     Total number of moves: %d\n\n", move_list->count);
#endif
}

void rb_print_move_list(moves_t *move_list)
{
#ifndef NDEBUG // print only in debug mode
    // do nothing on empty move list
    if (!move_list->count)
    {
        printf("\n     No move in the move list!\n");
        return;
    }
    printf("\nrb\n");
    printf("\n     move    piece     capture   double    enpass    castling\n\n");

    // loop over moves within a move list
    for (int move_count = 0; move_count < move_list->count; move_count++)
    {
        // init move
        int move = move_list->moves[move_count];

#ifdef _WIN64
        // print move
        printf("     %s%s%c   %c         %d         %d         %d         %d\n", rb_square_to_coordinates[get_move_source(move)],
               rb_square_to_coordinates[get_move_target(move)],
               get_move_promoted(move) ? rb_promoted_pieces[get_move_promoted(move)] : ' ',
               rb_ascii_pieces[get_move_piece(move)],
               get_move_capture(move) ? 1 : 0,
               get_move_double(move) ? 1 : 0,
               get_move_enpassant(move) ? 1 : 0,
               get_move_castling(move) ? 1 : 0);
#else
        // print move
        printf("     %s%s%c   %s         %d         %d         %d         %d\n", rb_square_to_coordinates[get_move_source(move)],
               rb_square_to_coordinates[get_move_target(move)],
               get_move_promoted(move) ? rb_promoted_pieces[get_move_promoted(move)] : ' ',
               rb_unicode_pieces[get_move_piece(move)],
               get_move_capture(move) ? 1 : 0,
               get_move_double(move) ? 1 : 0,
               get_move_enpassant(move) ? 1 : 0,
               get_move_castling(move) ? 1 : 0);
#endif
    }

    // print total number of moves
    printf("\n\n     Total number of moves: %d\n\n", move_list->count);
#endif
}

// preserve board state
///<ADD> cap_pieces + count, passthrough pieces + count, promoted_bitboards, fifty
#define lb_copy_board()                                                                                                   \
    U64 lb_bitboards_copy[12], lb_occupancies_copy[3], lb_promoted_bitboards_copy[2], lb_hash_key_copy;                   \
    int lb_side_copy, lb_enpassant_copy, lb_castle_copy, lb_fifty_copy, lb_cap_pieces_copy[64], lb_cap_pieces_count_copy, \
        lb_pt_pieces_copy[64], lb_pt_pieces_count_copy;                                                                   \
    memcpy(lb_bitboards_copy, lb_bitboards, 96);                                                                          \
    memcpy(lb_occupancies_copy, lb_occupancies, 24);                                                                      \
    memcpy(lb_promoted_bitboards_copy, lb_promoted_bitboards, 16);                                                        \
    memcpy(lb_cap_pieces_copy, lb_cap_pieces, 256);                                                                       \
    memcpy(lb_pt_pieces_copy, lb_pt_pieces, 256);                                                                         \
    lb_cap_pieces_count_copy = lb_cap_pieces_count, lb_pt_pieces_count_copy = lb_pt_pieces_count;                         \
    lb_side_copy = lb_side, lb_enpassant_copy = lb_enpassant, lb_castle_copy = lb_castle,                                 \
    lb_fifty_copy = lb_fifty, lb_hash_key_copy = lb_hash_key;

#define rb_copy_board()                                                                                                   \
    U64 rb_bitboards_copy[12], rb_occupancies_copy[3], rb_promoted_bitboards_copy[2], rb_hash_key_copy;                   \
    int rb_side_copy, rb_enpassant_copy, rb_castle_copy, rb_fifty_copy, rb_cap_pieces_copy[64], rb_cap_pieces_count_copy, \
        rb_pt_pieces_copy[64], rb_pt_pieces_count_copy;                                                                   \
    memcpy(rb_bitboards_copy, rb_bitboards, 96);                                                                          \
    memcpy(rb_occupancies_copy, rb_occupancies, 24);                                                                      \
    memcpy(rb_promoted_bitboards_copy, rb_promoted_bitboards, 16);                                                        \
    memcpy(rb_cap_pieces_copy, rb_cap_pieces, 256);                                                                       \
    memcpy(rb_pt_pieces_copy, rb_pt_pieces, 256);                                                                         \
    rb_cap_pieces_count_copy = rb_cap_pieces_count, rb_pt_pieces_count_copy = rb_pt_pieces_count;                         \
    rb_side_copy = rb_side, rb_enpassant_copy = rb_enpassant, rb_castle_copy = rb_castle,                                 \
    rb_fifty_copy = rb_fifty, rb_hash_key_copy = rb_hash_key;
///</ADD>

// restore board state
///<ADD> cap_pieces + count, passthrough pieces + count, promoted_bitboards, fifty
#define lb_take_back()                                                                            \
    memcpy(lb_bitboards, lb_bitboards_copy, 96);                                                  \
    memcpy(lb_occupancies, lb_occupancies_copy, 24);                                              \
    memcpy(lb_promoted_bitboards, lb_promoted_bitboards_copy, 16);                                \
    memcpy(lb_cap_pieces, lb_cap_pieces_copy, 256);                                               \
    memcpy(lb_pt_pieces, lb_pt_pieces_copy, 256);                                                 \
    lb_cap_pieces_count = lb_cap_pieces_count_copy, lb_pt_pieces_count = lb_pt_pieces_count_copy; \
    lb_side = lb_side_copy, lb_enpassant = lb_enpassant_copy, lb_castle = lb_castle_copy,         \
    lb_fifty = lb_fifty_copy, lb_hash_key = lb_hash_key_copy;

#define rb_take_back()                                                                            \
    memcpy(rb_bitboards, rb_bitboards_copy, 96);                                                  \
    memcpy(rb_occupancies, rb_occupancies_copy, 24);                                              \
    memcpy(rb_promoted_bitboards, rb_promoted_bitboards_copy, 16);                                \
    memcpy(rb_cap_pieces, rb_cap_pieces_copy, 256);                                               \
    memcpy(rb_pt_pieces, rb_pt_pieces_copy, 256);                                                 \
    rb_cap_pieces_count = rb_cap_pieces_count_copy, rb_pt_pieces_count = rb_pt_pieces_count_copy; \
    rb_side = rb_side_copy, rb_enpassant = rb_enpassant_copy, rb_castle = rb_castle_copy,         \
    rb_fifty = rb_fifty_copy, rb_hash_key = rb_hash_key_copy;
///</ADD>

/*
                           castling   move     in      in
                              right update     binary  decimal

 king & rooks didn't move:     1111 & 1111  =  1111    15

        white king  moved:     1111 & 1100  =  1100    12
  white king's rook moved:     1111 & 1110  =  1110    14
 white queen's rook moved:     1111 & 1101  =  1101    13

         black king moved:     1111 & 0011  =  1011    3
  black king's rook moved:     1111 & 1011  =  1011    11
 black queen's rook moved:     1111 & 0111  =  0111    7

*/

// castling rights update constants
const int lb_castling_rights[64] = {
    7, 15, 15, 15, 3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14};
const int rb_castling_rights[64] = {
    7, 15, 15, 15, 3, 15, 15, 11,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    15, 15, 15, 15, 15, 15, 15, 15,
    13, 15, 15, 15, 12, 15, 15, 14};

///<ADD>
static inline int lb_put_move(int move)
{
    // preserve board state
    lb_copy_board();

    // parse put move
    int target_square = get_move_target(move);
    int piece = get_move_piece(move);

    set_bit(lb_bitboards[piece], target_square);
    if (piece < Q)
        set_bit(lb_occupancies[white], target_square);
    else
        set_bit(lb_occupancies[black], target_square);
    set_bit(lb_occupancies[both], target_square);
    lb_hash_key ^= lb_piece_keys[piece][target_square]; // set piece to the target square in hash key
    // increment fifty move rule counter
    lb_fifty++;
    // change side
    lb_side ^= 1;

    // hash side
    lb_hash_key ^= lb_side_key;

    // make sure that both kings have not been exposed into a check because then the move is invalid
    if (lb_is_square_attacked(lb_get_ls1b_index(lb_bitboards[k]), white) ||
        lb_is_square_attacked(lb_get_ls1b_index(lb_bitboards[K]), black))
    {
        // take move back
        lb_take_back();

        // return illegal put move
        return 0;
    }

    // remove piece from the cap pieces list
    int idx = -1;
    for (int i = 0; i < lb_cap_pieces_count; ++i)
    {
        if (lb_cap_pieces[i] == piece + 100)
        {
            idx = i;
            break;
        }
    }
    if (idx >= 0)
    {
        for (int i = idx; i < lb_cap_pieces_count - 1; ++i)
            lb_cap_pieces[i] = lb_cap_pieces[i + 1];
        --lb_cap_pieces_count;
    }

    // return legal put move
    return 1;
}

static inline int rb_put_move(int move)
{
    // preserve board state
    rb_copy_board();

    // parse put move
    int target_square = get_move_target(move);
    int piece = get_move_piece(move);

    set_bit(rb_bitboards[piece], target_square);
    if (piece < Q)
        set_bit(rb_occupancies[white], target_square);
    else
        set_bit(rb_occupancies[black], target_square);
    set_bit(rb_occupancies[both], target_square);
    // set piece to the target square in hash key
    rb_hash_key ^= rb_piece_keys[piece][target_square];
    // increment fifty move rule counter
    rb_fifty++;
    // change side
    rb_side ^= 1;

    // hash side
    rb_hash_key ^= rb_side_key;

    // make sure that both kings have not been exposed into a check
    if (rb_is_square_attacked(rb_get_ls1b_index(rb_bitboards[k]), white) ||
        rb_is_square_attacked(rb_get_ls1b_index(rb_bitboards[K]), black))
    {
        // take move back
        rb_take_back();

        // return illegal put move
        return 0;
    }

    // remove piece from the cap pieces list
    int idx = -1;
    for (int i = 0; i < rb_cap_pieces_count; ++i)
    {
        if (rb_cap_pieces[i] == piece + 100)
        {
            idx = i;
            break;
        }
    }
    if (idx >= 0)
    {
        for (int i = idx; i < rb_cap_pieces_count - 1; ++i)
            rb_cap_pieces[i] = rb_cap_pieces[i + 1];
        --rb_cap_pieces_count;
    }

    // return legal put move
    return 1;
}
///</ADD>

// make move on chess board
static inline int lb_make_move(int move, int move_flag)
{
    // quiet moves
    if (move_flag == all_moves)
    {
        // parse move
        int source_square = get_move_source(move);
        int target_square = get_move_target(move);
        int piece = get_move_piece(move);
        int promoted_piece = get_move_promoted(move);
        int capture = get_move_capture(move);
        int double_push = get_move_double(move);
        int enpass = get_move_enpassant(move);
        int castling = get_move_castling(move);

        ///<ADD>
        if (source_square == target_square)
            return lb_put_move(move);
        ///</ADD>

        // preserve board state
        lb_copy_board();

        // move piece
        pop_bit(lb_bitboards[piece], source_square);
        set_bit(lb_bitboards[piece], target_square);

        ///<ADD>
        // Is piece a promoted piece then move on the promoted bitboard for the side to move
        if (get_bit(lb_promoted_bitboards[lb_side], source_square))
        {
            pop_bit(lb_promoted_bitboards[lb_side], source_square);
            set_bit(lb_promoted_bitboards[lb_side], target_square);
        }
        ///</ADD>

        // hash piece
        lb_hash_key ^= lb_piece_keys[piece][source_square]; // remove piece from source square in hash key
        lb_hash_key ^= lb_piece_keys[piece][target_square]; // set piece to the target square in hash key

        // increment fifty move rule counter
        lb_fifty++;

        // if pawn moved
        if (piece == P || piece == p)
            // reset fifty move rule counter
            lb_fifty = 0;

        // handling capture moves
        if (capture)
        {
            // reset fifty move rule counter
            lb_fifty = 0;

            // pick up bitboard piece index ranges depending on side
            int start_piece, end_piece;

            // white to move
            if (lb_side == white)
            {
                start_piece = p;
                end_piece = k;
            }
            // black to move
            else
            {
                start_piece = P;
                end_piece = K;
            }

            // loop over bitboards opposite to the current side to move
            for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++)
            {
                // if there's a piece on the target square
                if (get_bit(lb_bitboards[bb_piece], target_square))
                {
                    // for updating the captured bitboards
                    int cap_piece = bb_piece;

                    ///<ADD>
                    // is the captured piece a promoted piece?
                    if (get_bit(lb_promoted_bitboards[lb_side ^ 1], target_square))
                    {
                        // remove from the promoted bitboards
                        pop_bit(lb_promoted_bitboards[lb_side ^ 1], target_square);
                        // passthrough a pawn
                        cap_piece = (!lb_side) ? p : P;
                    }
                    lb_pt_pieces[lb_pt_pieces_count++] = cap_piece + 100;
                    ///</ADD>

                    // remove it from corresponding bitboard
                    pop_bit(lb_bitboards[bb_piece], target_square);

                    // remove the piece from hash key
                    lb_hash_key ^= lb_piece_keys[bb_piece][target_square];
                    break;
                }
            }
        }

        // handle pawn promotions
        if (promoted_piece)
        {
            // white to move
            if (lb_side == white)
            {
                ///<ADD>
                // add the square to the promoted bitboard to keep track of the promoted piece
                set_bit(lb_promoted_bitboards[white], target_square);
                ///</ADD>

                // erase the pawn from the target square
                pop_bit(lb_bitboards[P], target_square);

                // remove pawn from hash key
                lb_hash_key ^= lb_piece_keys[P][target_square];
            }
            // black to move
            else
            {
                ///<ADD>
                // add the square to the promoted bitboard to keep track of the promoted piece
                set_bit(lb_promoted_bitboards[black], target_square);
                ///</ADD>

                // erase the pawn from the target square
                pop_bit(lb_bitboards[p], target_square);

                // remove pawn from hash key
                lb_hash_key ^= lb_piece_keys[p][target_square];
            }

            // set up promoted piece on chess board
            set_bit(lb_bitboards[promoted_piece], target_square);

            // add promoted piece into the hash key
            lb_hash_key ^= lb_piece_keys[promoted_piece][target_square];
        }

        // handle enpassant captures
        if (enpass)
        {
            // erase the pawn depending on side to move
            (lb_side == white) ? pop_bit(lb_bitboards[p], target_square + 8) : pop_bit(lb_bitboards[P], target_square - 8);

            // white to move
            if (lb_side == white)
            {
                // remove captured pawn
                pop_bit(lb_bitboards[p], target_square + 8);

                // remove pawn from hash key
                lb_hash_key ^= lb_piece_keys[p][target_square + 8];
            }

            // black to move
            else
            {
                // remove captured pawn
                pop_bit(lb_bitboards[P], target_square - 8);

                // remove pawn from hash key
                lb_hash_key ^= lb_piece_keys[P][target_square - 8];
            }
        }

        // hash enpassant if available (remove enpassant square from hash key )
        if (lb_enpassant != no_sq)
            lb_hash_key ^= lb_enpassant_keys[lb_enpassant];

        // reset enpassant square
        lb_enpassant = no_sq;

        // handle double pawn push
        if (double_push)
        {
            // white to move
            if (lb_side == white)
            {
                // set enpassant square
                lb_enpassant = target_square + 8;

                // hash enpassant
                lb_hash_key ^= lb_enpassant_keys[target_square + 8];
            }

            // black to move
            else
            {
                // set enpassant square
                lb_enpassant = target_square - 8;

                // hash enpassant
                lb_hash_key ^= lb_enpassant_keys[target_square - 8];
            }
        }

        // handle castling moves
        if (castling)
        {
            // switch target square
            switch (target_square)
            {
            // white castles king side
            case (g1):
                // move H rook
                pop_bit(lb_bitboards[R], h1);
                set_bit(lb_bitboards[R], f1);

                // hash rook
                lb_hash_key ^= lb_piece_keys[R][h1]; // remove rook from h1 from hash key
                lb_hash_key ^= lb_piece_keys[R][f1]; // put rook on f1 into a hash key
                break;

            // white castles queen side
            case (c1):
                // move A rook
                pop_bit(lb_bitboards[R], a1);
                set_bit(lb_bitboards[R], d1);

                // hash rook
                lb_hash_key ^= lb_piece_keys[R][a1]; // remove rook from a1 from hash key
                lb_hash_key ^= lb_piece_keys[R][d1]; // put rook on d1 into a hash key
                break;

            // black castles king side
            case (g8):
                // move H rook
                pop_bit(lb_bitboards[r], h8);
                set_bit(lb_bitboards[r], f8);

                // hash rook
                lb_hash_key ^= lb_piece_keys[r][h8]; // remove rook from h8 from hash key
                lb_hash_key ^= lb_piece_keys[r][f8]; // put rook on f8 into a hash key
                break;

            // black castles queen side
            case (c8):
                // move A rook
                pop_bit(lb_bitboards[r], a8);
                set_bit(lb_bitboards[r], d8);

                // hash rook
                lb_hash_key ^= lb_piece_keys[r][a8]; // remove rook from a8 from hash key
                lb_hash_key ^= lb_piece_keys[r][d8]; // put rook on d8 into a hash key
                break;
            }
        }

        // hash castling
        lb_hash_key ^= lb_castle_keys[lb_castle];

        // update castling rights
        lb_castle &= lb_castling_rights[source_square];
        lb_castle &= lb_castling_rights[target_square];

        // hash castling
        lb_hash_key ^= lb_castle_keys[lb_castle];

        // reset occupancies
        memset(lb_occupancies, 0ULL, 24);

        // loop over white pieces bitboards
        for (int bb_piece = P; bb_piece <= K; bb_piece++)
            // update white occupancies
            lb_occupancies[white] |= lb_bitboards[bb_piece];

        // loop over black pieces bitboards
        for (int bb_piece = p; bb_piece <= k; bb_piece++)
            // update black occupancies
            lb_occupancies[black] |= lb_bitboards[bb_piece];

        // update both sides occupancies
        lb_occupancies[both] |= lb_occupancies[white];
        lb_occupancies[both] |= lb_occupancies[black];

        // change side
        lb_side ^= 1;

        // hash side
        lb_hash_key ^= lb_side_key;

        // make sure that king has not been exposed into a check
        if (lb_is_square_attacked((lb_side == white) ? lb_get_ls1b_index(lb_bitboards[k]) : lb_get_ls1b_index(lb_bitboards[K]), lb_side))
        {
            // take move back
            lb_take_back();

            // return illegal move
            return 0;
        }

        // otherwise
        else
            // return legal move
            return 1;
    }
    // capture moves
    else
    {
        // make sure move is the capture
        if (get_move_capture(move))
            lb_make_move(move, all_moves);

        // otherwise the move is not a capture
        else
            // don't make it
            return 0;
    }
    return 0;
}

static inline int rb_make_move(int move, int move_flag)
{
    // quiet moves
    if (move_flag == all_moves)
    {
        // parse move
        int source_square = get_move_source(move);
        int target_square = get_move_target(move);
        int piece = get_move_piece(move);
        int promoted_piece = get_move_promoted(move);
        int capture = get_move_capture(move);
        int double_push = get_move_double(move);
        int enpass = get_move_enpassant(move);
        int castling = get_move_castling(move);

        ///<ADD>
        if (source_square == target_square)
            return rb_put_move(move);
        ///</ADD>

        // preserve board state
        rb_copy_board();

        // move piece
        pop_bit(rb_bitboards[piece], source_square);
        set_bit(rb_bitboards[piece], target_square);

        ///<ADD>
        /* Is piece a promoted piece then move on the promoted bitboard for the side 2 move */
        if (get_bit(rb_promoted_bitboards[rb_side], source_square))
        {
            pop_bit(rb_promoted_bitboards[rb_side], source_square);
            set_bit(rb_promoted_bitboards[rb_side], target_square);
        }
        ///</ADD>

        // hash piece
        rb_hash_key ^= rb_piece_keys[piece][source_square]; // remove piece from source square in hash key
        rb_hash_key ^= rb_piece_keys[piece][target_square]; // set piece to the target square in hash key

        // increment fifty move rule counter
        rb_fifty++;

        // if pawn moved
        if (piece == P || piece == p)
            // reset fifty move rule counter
            rb_fifty = 0;

        // handling capture moves
        if (capture)
        {
            // reset fifty move rule counter
            rb_fifty = 0;

            // pick up bitboard piece index ranges depending on side
            int start_piece, end_piece;

            // white to move
            if (rb_side == white)
            {
                start_piece = p;
                end_piece = k;
            }
            // black to move
            else
            {
                start_piece = P;
                end_piece = K;
            }

            // loop over bitboards opposite to the current side to move
            for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++)
            {
                // if there's a piece on the target square
                if (get_bit(rb_bitboards[bb_piece], target_square))
                {
                    // for updating the captured bitboards in the gui
                    int cap_piece = bb_piece;

                    ///<ADD>
                    // is the captured piece a promoted piece ?
                    if (get_bit(rb_promoted_bitboards[rb_side ^ 1], target_square))
                    {
                        // remove from the promoted bitboards
                        pop_bit(rb_promoted_bitboards[rb_side ^ 1], target_square);
                        // passthrough as a pawn
                        cap_piece = (!rb_side) ? p : P;
                    }
                    rb_pt_pieces[rb_pt_pieces_count++] = cap_piece + 100;
                    ///</ADD>

                    // remove it from corresponding bitboard
                    pop_bit(rb_bitboards[bb_piece], target_square);

                    // remove the piece from hash key
                    rb_hash_key ^= rb_piece_keys[bb_piece][target_square];
                    break;
                }
            }
        }

        // handle pawn promotions
        if (promoted_piece)
        {
            // white to move
            if (rb_side == white)
            {
                ///<ADD>
                // add the square to the promoted bitboard to keep track of the promoted piece
                set_bit(rb_promoted_bitboards[white], target_square);
                ///</ADD>

                // erase the pawn from the target square
                pop_bit(rb_bitboards[P], target_square);

                // remove pawn from hash key
                rb_hash_key ^= rb_piece_keys[P][target_square];
            }
            else // black to move
            {
                ///<ADD>
                // add the square to the promoted bitboard to keep track of the promoted piece
                set_bit(rb_promoted_bitboards[black], target_square);
                ///</ADD>

                // erase the pawn from the target square
                pop_bit(rb_bitboards[p], target_square);

                // remove pawn from hash key
                rb_hash_key ^= rb_piece_keys[p][target_square];
            }

            // set up promoted piece on chess board
            set_bit(rb_bitboards[promoted_piece], target_square);

            // add promoted piece into the hash key
            rb_hash_key ^= rb_piece_keys[promoted_piece][target_square];
        }

        // handle enpassant captures
        if (enpass)
        {
            // erase the pawn depending on side to move
            (rb_side == white) ? pop_bit(rb_bitboards[p], target_square + 8) : pop_bit(rb_bitboards[P], target_square - 8);

            // white to move
            if (rb_side == white)
            {
                // remove captured pawn
                pop_bit(rb_bitboards[p], target_square + 8);

                // remove pawn from hash key
                rb_hash_key ^= rb_piece_keys[p][target_square + 8];
            }

            // black to move
            else
            {
                // remove captured pawn
                pop_bit(rb_bitboards[P], target_square - 8);

                // remove pawn from hash key
                rb_hash_key ^= rb_piece_keys[P][target_square - 8];
            }
        }

        // hash enpassant if available (remove enpassant square from hash key )
        if (rb_enpassant != no_sq)
            rb_hash_key ^= rb_enpassant_keys[rb_enpassant];

        // reset enpassant square
        rb_enpassant = no_sq;

        // handle double pawn push
        if (double_push)
        {
            // white to move
            if (rb_side == white)
            {
                // set enpassant square
                rb_enpassant = target_square + 8;

                // hash enpassant
                rb_hash_key ^= rb_enpassant_keys[target_square + 8];
            }

            // black to move
            else
            {
                // set enpassant square
                rb_enpassant = target_square - 8;

                // hash enpassant
                rb_hash_key ^= rb_enpassant_keys[target_square - 8];
            }
        }

        // handle castling moves
        if (castling)
        {
            // switch target square
            switch (target_square)
            {
            // white castles king side
            case (g1):
                // move H rook
                pop_bit(rb_bitboards[R], h1);
                set_bit(rb_bitboards[R], f1);

                // hash rook
                rb_hash_key ^= rb_piece_keys[R][h1]; // remove rook from h1 from hash key
                rb_hash_key ^= rb_piece_keys[R][f1]; // put rook on f1 into a hash key
                break;

            // white castles queen side
            case (c1):
                // move A rook
                pop_bit(rb_bitboards[R], a1);
                set_bit(rb_bitboards[R], d1);

                // hash rook
                rb_hash_key ^= rb_piece_keys[R][a1]; // remove rook from a1 from hash key
                rb_hash_key ^= rb_piece_keys[R][d1]; // put rook on d1 into a hash key
                break;

            // black castles king side
            case (g8):
                // move H rook
                pop_bit(rb_bitboards[r], h8);
                set_bit(rb_bitboards[r], f8);

                // hash rook
                rb_hash_key ^= rb_piece_keys[r][h8]; // remove rook from h8 from hash key
                rb_hash_key ^= rb_piece_keys[r][f8]; // put rook on f8 into a hash key
                break;

            // black castles queen side
            case (c8):
                // move A rook
                pop_bit(rb_bitboards[r], a8);
                set_bit(rb_bitboards[r], d8);

                // hash rook
                rb_hash_key ^= rb_piece_keys[r][a8]; // remove rook from a8 from hash key
                rb_hash_key ^= rb_piece_keys[r][d8]; // put rook on d8 into a hash key
                break;
            }
        }

        // hash castling
        rb_hash_key ^= rb_castle_keys[rb_castle];

        // update castling rights
        rb_castle &= rb_castling_rights[source_square];
        rb_castle &= rb_castling_rights[target_square];

        // hash castling
        rb_hash_key ^= rb_castle_keys[rb_castle];

        // reset occupancies
        memset(rb_occupancies, 0ULL, 24);

        // loop over white pieces bitboards
        for (int bb_piece = P; bb_piece <= K; bb_piece++)
            // update white occupancies
            rb_occupancies[white] |= rb_bitboards[bb_piece];

        // loop over black pieces bitboards
        for (int bb_piece = p; bb_piece <= k; bb_piece++)
            // update black occupancies
            rb_occupancies[black] |= rb_bitboards[bb_piece];

        // update both sides occupancies
        rb_occupancies[both] |= rb_occupancies[white];
        rb_occupancies[both] |= rb_occupancies[black];

        // change side
        rb_side ^= 1;

        // hash side
        rb_hash_key ^= rb_side_key;

        // make sure that king has not been exposed into a check
        if (rb_is_square_attacked((rb_side == white) ? rb_get_ls1b_index(rb_bitboards[k]) : rb_get_ls1b_index(rb_bitboards[K]), rb_side))
        {
            // take move back
            rb_take_back();

            // return illegal move
            return 0;
        }
        // otherwise
        else
            // return legal move
            return 1;
    }
    // capture moves
    else
    {
        // make sure move is the capture
        if (get_move_capture(move))
            rb_make_move(move, all_moves);

        // otherwise the move is not a capture
        else
            // don't make it
            return 0;
    }
    return 0;
}

// generate all moves
static inline void lb_generate_moves(moves_t *move_list)
{
    // init move count
    move_list->count = 0;

    // define source & target squares
    int source_square, target_square;

    // define current piece's bitboard copy & it's attacks
    U64 bitboard, attacks;

    // loop over all the bitboards
    for (int piece = P; piece <= k; piece++)
    {
        // init piece bitboard copy
        bitboard = lb_bitboards[piece];

        // generate white pawns & white king castling moves
        if (lb_side == white)
        {
            // pick up white pawn bitboards index
            if (piece == P)
            {
                // loop over white pawns within white pawn bitboard
                while (bitboard)
                {
                    // init source square
                    source_square = lb_get_ls1b_index(bitboard);

                    // init target square
                    target_square = source_square - 8;

                    // generate quiet pawn moves
                    if (!(target_square < a8) && !get_bit(lb_occupancies[both], target_square))
                    {
                        // pawn promotion
                        if (source_square >= a7 && source_square <= h7)
                        {
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, Q, 0, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, R, 0, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, B, 0, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, N, 0, 0, 0, 0));
                        }

                        else
                        {
                            // one square ahead pawn move
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            // two squares ahead pawn move
                            if ((source_square >= a2 && source_square <= h2) && !get_bit(lb_occupancies[both], target_square - 8))
                                lb_add_move(move_list, encode_move(source_square, target_square - 8, piece, 0, 0, 1, 0, 0));
                        }
                    }

                    // init pawn attacks bitboard
                    attacks = lb_pawn_attacks[lb_side][source_square] & lb_occupancies[black];

                    // generate pawn captures
                    while (attacks)
                    {
                        // init target square
                        target_square = lb_get_ls1b_index(attacks);

                        // pawn promotion
                        if (source_square >= a7 && source_square <= h7)
                        {
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, Q, 1, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, R, 1, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, B, 1, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, N, 1, 0, 0, 0));
                        }

                        else
                            // one square ahead pawn move
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                        // pop ls1b of the pawn attacks
                        pop_bit(attacks, target_square);
                    }

                    // generate enpassant captures
                    if (lb_enpassant != no_sq)
                    {
                        // lookup pawn attacks and bitwise AND with enpassant square (bit)
                        U64 lb_enpassant_attacks = lb_pawn_attacks[lb_side][source_square] & (1ULL << lb_enpassant);

                        // make sure enpassant capture available
                        if (lb_enpassant_attacks)
                        {
                            // init enpassant capture target square
                            int target_enpassant = lb_get_ls1b_index(lb_enpassant_attacks);
                            lb_add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    // pop ls1b from piece bitboard copy
                    pop_bit(bitboard, source_square);
                }
            }

            // castling moves
            if (piece == K)
            {
                // king side castling is available
                if (lb_castle & wk)
                {
                    // make sure square between king and king's rook are empty
                    if (!get_bit(lb_occupancies[both], f1) && !get_bit(lb_occupancies[both], g1))
                    {
                        // make sure king and the f1 squares are not under attacks
                        if (!lb_is_square_attacked(e1, black) && !lb_is_square_attacked(f1, black))
                            lb_add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
                    }
                }

                // queen side castling is available
                if (lb_castle & wq)
                {
                    // make sure square between king and queen's rook are empty
                    if (!get_bit(lb_occupancies[both], d1) && !get_bit(lb_occupancies[both], c1) && !get_bit(lb_occupancies[both], b1))
                    {
                        // make sure king and the d1 squares are not under attacks
                        if (!lb_is_square_attacked(e1, black) && !lb_is_square_attacked(d1, black))
                            lb_add_move(move_list, encode_move(e1, c1, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }

        // generate black pawns & black king castling moves
        else
        {
            // pick up black pawn bitboards index
            if (piece == p)
            {
                // loop over white pawns within white pawn bitboard
                while (bitboard)
                {
                    // init source square
                    source_square = lb_get_ls1b_index(bitboard);

                    // init target square
                    target_square = source_square + 8;

                    // generate quiet pawn moves
                    if (!(target_square > h1) && !get_bit(lb_occupancies[both], target_square))
                    {
                        // pawn promotion
                        if (source_square >= a2 && source_square <= h2)
                        {
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, q, 0, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, r, 0, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, b, 0, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, n, 0, 0, 0, 0));
                        }

                        else
                        {
                            // one square ahead pawn move
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            // two squares ahead pawn move
                            if ((source_square >= a7 && source_square <= h7) && !get_bit(lb_occupancies[both], target_square + 8))
                                lb_add_move(move_list, encode_move(source_square, target_square + 8, piece, 0, 0, 1, 0, 0));
                        }
                    }

                    // init pawn attacks bitboard
                    attacks = lb_pawn_attacks[lb_side][source_square] & lb_occupancies[white];

                    // generate pawn captures
                    while (attacks)
                    {
                        // init target square
                        target_square = lb_get_ls1b_index(attacks);

                        // pawn promotion
                        if (source_square >= a2 && source_square <= h2)
                        {
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, q, 1, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, r, 1, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, b, 1, 0, 0, 0));
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, n, 1, 0, 0, 0));
                        }

                        else
                            // one square ahead pawn move
                            lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                        // pop ls1b of the pawn attacks
                        pop_bit(attacks, target_square);
                    }

                    // generate enpassant captures
                    if (lb_enpassant != no_sq)
                    {
                        // lookup pawn attacks and bitwise AND with enpassant square (bit)
                        U64 lb_enpassant_attacks = lb_pawn_attacks[lb_side][source_square] & (1ULL << lb_enpassant);

                        // make sure enpassant capture available
                        if (lb_enpassant_attacks)
                        {
                            // init enpassant capture target square
                            int target_enpassant = lb_get_ls1b_index(lb_enpassant_attacks);
                            lb_add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    // pop ls1b from piece bitboard copy
                    pop_bit(bitboard, source_square);
                }
            }

            // castling moves
            if (piece == k)
            {
                // king side castling is available
                if (lb_castle & bk)
                {
                    // make sure square between king and king's rook are empty
                    if (!get_bit(lb_occupancies[both], f8) && !get_bit(lb_occupancies[both], g8))
                    {
                        // make sure king and the f8 squares are not under attacks
                        if (!lb_is_square_attacked(e8, white) && !lb_is_square_attacked(f8, white))
                            lb_add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
                    }
                }

                // queen side castling is available
                if (lb_castle & bq)
                {
                    // make sure square between king and queen's rook are empty
                    if (!get_bit(lb_occupancies[both], d8) && !get_bit(lb_occupancies[both], c8) && !get_bit(lb_occupancies[both], b8))
                    {
                        // make sure king and the d8 squares are not under attacks
                        if (!lb_is_square_attacked(e8, white) && !lb_is_square_attacked(d8, white))
                            lb_add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }

        // genarate knight moves
        if ((lb_side == white) ? piece == N : piece == n)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = lb_get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = lb_knight_attacks[source_square] & ((lb_side == white) ? ~lb_occupancies[white] : ~lb_occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = lb_get_ls1b_index(attacks);

                    // quiet move
                    if (!get_bit(((lb_side == white) ? lb_occupancies[black] : lb_occupancies[white]), target_square))
                        lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate bishop moves
        if ((lb_side == white) ? piece == B : piece == b)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = lb_get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = lb_get_bishop_attacks(source_square, lb_occupancies[both]) & ((lb_side == white) ? ~lb_occupancies[white] : ~lb_occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = lb_get_ls1b_index(attacks);

                    // quiet move
                    if (!get_bit(((lb_side == white) ? lb_occupancies[black] : lb_occupancies[white]), target_square))
                        lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate rook moves
        if ((lb_side == white) ? piece == R : piece == r)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = lb_get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = lb_get_rook_attacks(source_square, lb_occupancies[both]) & ((lb_side == white) ? ~lb_occupancies[white] : ~lb_occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = lb_get_ls1b_index(attacks);

                    // quiet move
                    if (!get_bit(((lb_side == white) ? lb_occupancies[black] : lb_occupancies[white]), target_square))
                        lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate queen moves
        if ((lb_side == white) ? piece == Q : piece == q)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = lb_get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = lb_get_queen_attacks(source_square, lb_occupancies[both]) & ((lb_side == white) ? ~lb_occupancies[white] : ~lb_occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = lb_get_ls1b_index(attacks);

                    // quiet move
                    if (!get_bit(((lb_side == white) ? lb_occupancies[black] : lb_occupancies[white]), target_square))
                        lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate king moves
        if ((lb_side == white) ? piece == K : piece == k)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = lb_get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = lb_king_attacks[source_square] & ((lb_side == white) ? ~lb_occupancies[white] : ~lb_occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = lb_get_ls1b_index(attacks);

                    // quiet move
                    if (!get_bit(((lb_side == white) ? lb_occupancies[black] : lb_occupancies[white]), target_square))
                        lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        lb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }
    }

    ///<ADD>
    // make captured pieces move
    if (lb_cap_pieces_count == 0)
        return;

    int lb_cappieces[11];
    int lb_rank[2][5] = {
        {Q, P, R, N, B},
        {q, p, r, n, b},
    };

    for (int i = 0; i < 11; ++i)
        lb_cappieces[i] = 0;
    if (lb_cap_pieces_count > 0)
    {
        for (int i = 0; i < lb_cap_pieces_count; ++i)
        {
            int piece = lb_cap_pieces[i] - 100;
            ++lb_cappieces[piece];
        }
    }

    // add put moves for captures pieces
    for (int i = 0; i < 5; ++i)
    {
        int piece = lb_rank[lb_side][i];
        if (lb_cappieces[piece] > 0)
        {
            // make put moves only for the side to move
            U64 bitboard = piece == P || piece == p ? ~lb_occupancies[both] & pawn_put_options : ~lb_occupancies[both] & piece_put_options;
            while (bitboard)
            {
                int square = lb_get_ls1b_index(bitboard);
                lb_add_move(move_list, encode_move(square, square, piece, 0, 0, 0, 0, 0));
                pop_bit(bitboard, square);
            }
        }
    }
    ///</ADD>
}

static inline void rb_generate_moves(moves_t *move_list)
{
    // init move count
    move_list->count = 0;

    // define source & target squares
    int source_square, target_square;

    // define current piece's bitboard copy & it's attacks
    U64 bitboard, attacks;

    // loop over all the bitboards
    for (int piece = P; piece <= k; piece++)
    {
        // init piece bitboard copy
        bitboard = rb_bitboards[piece];

        // generate white pawns & white king castling moves
        if (rb_side == white)
        {
            // pick up white pawn bitboards index
            if (piece == P)
            {
                // loop over white pawns within white pawn bitboard
                while (bitboard)
                {
                    // init source square
                    source_square = rb_get_ls1b_index(bitboard);

                    // init target square
                    target_square = source_square - 8;

                    // generate quiet pawn moves
                    if (!(target_square < a8) && !get_bit(rb_occupancies[both], target_square))
                    {
                        // pawn promotion
                        if (source_square >= a7 && source_square <= h7)
                        {
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, Q, 0, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, R, 0, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, B, 0, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, N, 0, 0, 0, 0));
                        }

                        else
                        {
                            // one square ahead pawn move
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            // two squares ahead pawn move
                            if ((source_square >= a2 && source_square <= h2) && !get_bit(rb_occupancies[both], target_square - 8))
                                rb_add_move(move_list, encode_move(source_square, target_square - 8, piece, 0, 0, 1, 0, 0));
                        }
                    }

                    // init pawn attacks bitboard
                    attacks = rb_pawn_attacks[rb_side][source_square] & rb_occupancies[black];

                    // generate pawn captures
                    while (attacks)
                    {
                        // init target square
                        target_square = rb_get_ls1b_index(attacks);

                        // pawn promotion
                        if (source_square >= a7 && source_square <= h7)
                        {
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, Q, 1, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, R, 1, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, B, 1, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, N, 1, 0, 0, 0));
                        }

                        else
                            // one square ahead pawn move
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                        // pop ls1b of the pawn attacks
                        pop_bit(attacks, target_square);
                    }

                    // generate enpassant captures
                    if (rb_enpassant != no_sq)
                    {
                        // lookup pawn attacks and bitwise AND with enpassant square (bit)
                        U64 rb_enpassant_attacks = rb_pawn_attacks[rb_side][source_square] & (1ULL << rb_enpassant);

                        // make sure enpassant capture available
                        if (rb_enpassant_attacks)
                        {
                            // init enpassant capture target square
                            int target_enpassant = rb_get_ls1b_index(rb_enpassant_attacks);
                            rb_add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    // pop ls1b from piece bitboard copy
                    pop_bit(bitboard, source_square);
                }
            }

            // castling moves
            if (piece == K)
            {
                // king side castling is available
                if (rb_castle & wk)
                {
                    // make sure square between king and king's rook are empty
                    if (!get_bit(rb_occupancies[both], f1) && !get_bit(rb_occupancies[both], g1))
                    {
                        // make sure king and the f1 squares are not under attacks
                        if (!rb_is_square_attacked(e1, black) && !rb_is_square_attacked(f1, black))
                            rb_add_move(move_list, encode_move(e1, g1, piece, 0, 0, 0, 0, 1));
                    }
                }

                // queen side castling is available
                if (rb_castle & wq)
                {
                    // make sure square between king and queen's rook are empty
                    if (!get_bit(rb_occupancies[both], d1) && !get_bit(rb_occupancies[both], c1) && !get_bit(rb_occupancies[both], b1))
                    {
                        // make sure king and the d1 squares are not under attacks
                        if (!rb_is_square_attacked(e1, black) && !rb_is_square_attacked(d1, black))
                            rb_add_move(move_list, encode_move(e1, c1, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }

        // generate black pawns & black king castling moves
        else
        {
            // pick up black pawn bitboards index
            if (piece == p)
            {
                // loop over white pawns within white pawn bitboard
                while (bitboard)
                {
                    // init source square
                    source_square = rb_get_ls1b_index(bitboard);

                    // init target square
                    target_square = source_square + 8;

                    // generate quiet pawn moves
                    if (!(target_square > h1) && !get_bit(rb_occupancies[both], target_square))
                    {
                        // pawn promotion
                        if (source_square >= a2 && source_square <= h2)
                        {
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, q, 0, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, r, 0, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, b, 0, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, n, 0, 0, 0, 0));
                        }

                        else
                        {
                            // one square ahead pawn move
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                            // two squares ahead pawn move
                            if ((source_square >= a7 && source_square <= h7) && !get_bit(rb_occupancies[both], target_square + 8))
                                rb_add_move(move_list, encode_move(source_square, target_square + 8, piece, 0, 0, 1, 0, 0));
                        }
                    }

                    // init pawn attacks bitboard
                    attacks = rb_pawn_attacks[rb_side][source_square] & rb_occupancies[white];

                    // generate pawn captures
                    while (attacks)
                    {
                        // init target square
                        target_square = rb_get_ls1b_index(attacks);

                        // pawn promotion
                        if (source_square >= a2 && source_square <= h2)
                        {
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, q, 1, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, r, 1, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, b, 1, 0, 0, 0));
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, n, 1, 0, 0, 0));
                        }

                        else
                            // one square ahead pawn move
                            rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                        // pop ls1b of the pawn attacks
                        pop_bit(attacks, target_square);
                    }

                    // generate enpassant captures
                    if (rb_enpassant != no_sq)
                    {
                        // lookup pawn attacks and bitwise AND with enpassant square (bit)
                        U64 rb_enpassant_attacks = rb_pawn_attacks[rb_side][source_square] & (1ULL << rb_enpassant);

                        // make sure enpassant capture available
                        if (rb_enpassant_attacks)
                        {
                            // init enpassant capture target square
                            int target_enpassant = rb_get_ls1b_index(rb_enpassant_attacks);
                            rb_add_move(move_list, encode_move(source_square, target_enpassant, piece, 0, 1, 0, 1, 0));
                        }
                    }

                    // pop ls1b from piece bitboard copy
                    pop_bit(bitboard, source_square);
                }
            }

            // castling moves
            if (piece == k)
            {
                // king side castling is available
                if (rb_castle & bk)
                {
                    // make sure square between king and king's rook are empty
                    if (!get_bit(rb_occupancies[both], f8) && !get_bit(rb_occupancies[both], g8))
                    {
                        // make sure king and the f8 squares are not under attacks
                        if (!rb_is_square_attacked(e8, white) && !rb_is_square_attacked(f8, white))
                            rb_add_move(move_list, encode_move(e8, g8, piece, 0, 0, 0, 0, 1));
                    }
                }

                // queen side castling is available
                if (rb_castle & bq)
                {
                    // make sure square between king and queen's rook are empty
                    if (!get_bit(rb_occupancies[both], d8) && !get_bit(rb_occupancies[both], c8) && !get_bit(rb_occupancies[both], b8))
                    {
                        // make sure king and the d8 squares are not under attacks
                        if (!rb_is_square_attacked(e8, white) && !rb_is_square_attacked(d8, white))
                            rb_add_move(move_list, encode_move(e8, c8, piece, 0, 0, 0, 0, 1));
                    }
                }
            }
        }

        // genarate knight moves
        if ((rb_side == white) ? piece == N : piece == n)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = rb_get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = rb_knight_attacks[source_square] & ((rb_side == white) ? ~rb_occupancies[white] : ~rb_occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = rb_get_ls1b_index(attacks);

                    // quiet move
                    if (!get_bit(((rb_side == white) ? rb_occupancies[black] : rb_occupancies[white]), target_square))
                        rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate bishop moves
        if ((rb_side == white) ? piece == B : piece == b)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = rb_get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = rb_get_bishop_attacks(source_square, rb_occupancies[both]) & ((rb_side == white) ? ~rb_occupancies[white] : ~rb_occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = rb_get_ls1b_index(attacks);

                    // quiet move
                    if (!get_bit(((rb_side == white) ? rb_occupancies[black] : rb_occupancies[white]), target_square))
                        rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate rook moves
        if ((rb_side == white) ? piece == R : piece == r)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = rb_get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = rb_get_rook_attacks(source_square, rb_occupancies[both]) & ((rb_side == white) ? ~rb_occupancies[white] : ~rb_occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = rb_get_ls1b_index(attacks);

                    // quiet move
                    if (!get_bit(((rb_side == white) ? rb_occupancies[black] : rb_occupancies[white]), target_square))
                        rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate queen moves
        if ((rb_side == white) ? piece == Q : piece == q)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = rb_get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = rb_get_queen_attacks(source_square, rb_occupancies[both]) & ((rb_side == white) ? ~rb_occupancies[white] : ~rb_occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = rb_get_ls1b_index(attacks);

                    // quiet move
                    if (!get_bit(((rb_side == white) ? rb_occupancies[black] : rb_occupancies[white]), target_square))
                        rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }

        // generate king moves
        if ((rb_side == white) ? piece == K : piece == k)
        {
            // loop over source squares of piece bitboard copy
            while (bitboard)
            {
                // init source square
                source_square = rb_get_ls1b_index(bitboard);

                // init piece attacks in order to get set of target squares
                attacks = rb_king_attacks[source_square] & ((rb_side == white) ? ~rb_occupancies[white] : ~rb_occupancies[black]);

                // loop over target squares available from generated attacks
                while (attacks)
                {
                    // init target square
                    target_square = rb_get_ls1b_index(attacks);

                    // quiet move
                    if (!get_bit(((rb_side == white) ? rb_occupancies[black] : rb_occupancies[white]), target_square))
                        rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 0, 0, 0, 0));

                    else
                        // capture move
                        rb_add_move(move_list, encode_move(source_square, target_square, piece, 0, 1, 0, 0, 0));

                    // pop ls1b in current attacks set
                    pop_bit(attacks, target_square);
                }

                // pop ls1b of the current piece bitboard copy
                pop_bit(bitboard, source_square);
            }
        }
    }

    int rb_cappieces[11];
    int rb_rank[2][5] = {
        {Q, P, R, N, B},
        {q, p, r, n, b},
    };

    for (int i = 0; i < 11; ++i)
        rb_cappieces[i] = 0;
    if (rb_cap_pieces_count > 0)
    {
        for (int i = 0; i < rb_cap_pieces_count; ++i)
        {
            int piece = rb_cap_pieces[i] - 100;
            ++rb_cappieces[piece];
        }
    }

    // add put moves for captures pieces
    for (int i = 0; i < 5; ++i)
    {
        int piece = rb_rank[lb_side][i];
        if (rb_cappieces[piece] > 0)
        {
            // make put moves only for the side to move
            U64 bitboard = piece == P || piece == p ? ~lb_occupancies[both] & pawn_put_options : ~lb_occupancies[both] & piece_put_options;
            while (bitboard)
            {
                int square = rb_get_ls1b_index(bitboard);
                rb_add_move(move_list, encode_move(square, square, piece, 0, 0, 0, 0, 0));
                pop_bit(bitboard, square);
            }
        }
    }
    ///</ADD>
}

/**********************************\
 ==================================
 
               Perft
 
 ==================================
\**********************************/

// leaf nodes (number of positions reached during the test of the move generator at a given depth)
U64 lb_nodes;
U64 rb_nodes;

/**********************************\
 ==================================
 
             Evaluation
 
 ==================================
\**********************************/

// material scrore

/*
    ♙ =   100   = ♙
    ♘ =   300   = ♙ * 3
    ♗ =   350   = ♙ * 3 + ♙ * 0.5
    ♖ =   500   = ♙ * 5
    ♕ =   1000  = ♙ * 10
    ♔ =   10000 = ♙ * 100

*/

// material score [game phase][piece]
const int lb_material_score[2][12] =
    {
        // opening material score
        82, 337, 365, 477, 1025, 12000, -82, -337, -365, -477, -1025, -12000,

        // endgame material score
        94, 281, 297, 512, 936, 12000, -94, -281, -297, -512, -936, -12000};
const int rb_material_score[2][12] =
    {
        // opening material score
        82, 337, 365, 477, 1025, 12000, -82, -337, -365, -477, -1025, -12000,

        // endgame material score
        94, 281, 297, 512, 936, 12000, -94, -281, -297, -512, -936, -12000};

// game phase scores
const int lb_opening_phase_score = 6192;
const int rb_opening_phase_score = 6192;
const int lb_endgame_phase_score = 518;
const int rb_endgame_phase_score = 518;

// positional piece scores [game phase][piece][square]
const int lb_positional_score[2][6][64] =

    // opening positional piece scores //
    {
        // pawn
        0, 0, 0, 0, 0, 0, 0, 0,
        98, 134, 61, 95, 68, 126, 34, -11,
        -6, 7, 26, 31, 65, 56, 25, -20,
        -14, 13, 6, 21, 23, 12, 17, -23,
        -27, -2, -5, 12, 17, 6, 10, -25,
        -26, -4, -4, -10, 3, 3, 33, -12,
        -35, -1, -20, -23, -15, 24, 38, -22,
        0, 0, 0, 0, 0, 0, 0, 0,

        // knight
        -167, -89, -34, -49, 61, -97, -15, -107,
        -73, -41, 72, 36, 23, 62, 7, -17,
        -47, 60, 37, 65, 84, 129, 73, 44,
        -9, 17, 19, 53, 37, 69, 18, 22,
        -13, 4, 16, 13, 28, 19, 21, -8,
        -23, -9, 12, 10, 19, 17, 25, -16,
        -29, -53, -12, -3, -1, 18, -14, -19,
        -105, -21, -58, -33, -17, -28, -19, -23,

        // bishop
        -29, 4, -82, -37, -25, -42, 7, -8,
        -26, 16, -18, -13, 30, 59, 18, -47,
        -16, 37, 43, 40, 35, 50, 37, -2,
        -4, 5, 19, 50, 37, 37, 7, -2,
        -6, 13, 13, 26, 34, 12, 10, 4,
        0, 15, 15, 15, 14, 27, 18, 10,
        4, 15, 16, 0, 7, 21, 33, 1,
        -33, -3, -14, -21, -13, -12, -39, -21,

        // rook
        32, 42, 32, 51, 63, 9, 31, 43,
        27, 32, 58, 62, 80, 67, 26, 44,
        -5, 19, 26, 36, 17, 45, 61, 16,
        -24, -11, 7, 26, 24, 35, -8, -20,
        -36, -26, -12, -1, 9, -7, 6, -23,
        -45, -25, -16, -17, 3, 0, -5, -33,
        -44, -16, -20, -9, -1, 11, -6, -71,
        -19, -13, 1, 17, 16, 7, -37, -26,

        // queen
        -28, 0, 29, 12, 59, 44, 43, 45,
        -24, -39, -5, 1, -16, 57, 28, 54,
        -13, -17, 7, 8, 29, 56, 47, 57,
        -27, -27, -16, -16, -1, 17, -2, 1,
        -9, -26, -9, -10, -2, -4, 3, -3,
        -14, 2, -11, -2, -5, 2, 14, 5,
        -35, -8, 11, 2, 8, 15, -3, 1,
        -1, -18, -9, 10, -15, -25, -31, -50,

        // king
        -65, 23, 16, -15, -56, -34, 2, 13,
        29, -1, -20, -7, -8, -4, -38, -29,
        -9, 24, 2, -16, -20, 6, 22, -22,
        -17, -20, -12, -27, -30, -25, -14, -36,
        -49, -1, -27, -39, -46, -44, -33, -51,
        -14, -14, -22, -46, -44, -30, -15, -27,
        1, 7, -8, -64, -43, -16, 9, 8,
        -15, 36, 12, -54, 8, -28, 24, 14,

        // Endgame positional piece scores //

        // pawn
        0, 0, 0, 0, 0, 0, 0, 0,
        178, 173, 158, 134, 147, 132, 165, 187,
        94, 100, 85, 67, 56, 53, 82, 84,
        32, 24, 13, 5, -2, 4, 17, 17,
        13, 9, -3, -7, -7, -8, 3, -1,
        4, 7, -6, 1, 0, -5, -1, -8,
        13, 8, 8, 10, 13, 0, 2, -7,
        0, 0, 0, 0, 0, 0, 0, 0,

        // knight
        -58, -38, -13, -28, -31, -27, -63, -99,
        -25, -8, -25, -2, -9, -25, -24, -52,
        -24, -20, 10, 9, -1, -9, -19, -41,
        -17, 3, 22, 22, 22, 11, 8, -18,
        -18, -6, 16, 25, 16, 17, 4, -18,
        -23, -3, -1, 15, 10, -3, -20, -22,
        -42, -20, -10, -5, -2, -20, -23, -44,
        -29, -51, -23, -15, -22, -18, -50, -64,

        // bishop
        -14, -21, -11, -8, -7, -9, -17, -24,
        -8, -4, 7, -12, -3, -13, -4, -14,
        2, -8, 0, -1, -2, 6, 0, 4,
        -3, 9, 12, 9, 14, 10, 3, 2,
        -6, 3, 13, 19, 7, 10, -3, -9,
        -12, -3, 8, 10, 13, 3, -7, -15,
        -14, -18, -7, -1, 4, -9, -15, -27,
        -23, -9, -23, -5, -9, -16, -5, -17,

        // rook
        13, 10, 18, 15, 12, 12, 8, 5,
        11, 13, 13, 11, -3, 3, 8, 3,
        7, 7, 7, 5, 4, -3, -5, -3,
        4, 3, 13, 1, 2, 1, -1, 2,
        3, 5, 8, 4, -5, -6, -8, -11,
        -4, 0, -5, -1, -7, -12, -8, -16,
        -6, -6, 0, 2, -9, -9, -11, -3,
        -9, 2, 3, -1, -5, -13, 4, -20,

        // queen
        -9, 22, 22, 27, 27, 19, 10, 20,
        -17, 20, 32, 41, 58, 25, 30, 0,
        -20, 6, 9, 49, 47, 35, 19, 9,
        3, 22, 24, 45, 57, 40, 57, 36,
        -18, 28, 19, 47, 31, 34, 39, 23,
        -16, -27, 15, 6, 9, 17, 10, 5,
        -22, -23, -30, -16, -16, -23, -36, -32,
        -33, -28, -22, -43, -5, -32, -20, -41,

        // king
        -74, -35, -18, -18, -11, 15, 4, -17,
        -12, 17, 14, 17, 17, 38, 23, 11,
        10, 17, 23, 15, 20, 45, 44, 13,
        -8, 22, 24, 27, 26, 33, 26, 3,
        -18, -4, 21, 24, 27, 23, 9, -11,
        -19, -3, 11, 21, 23, 16, 7, -9,
        -27, -11, 4, 13, 14, 4, -5, -17,
        -53, -34, -21, -11, -28, -14, -24, -43};
const int rb_positional_score[2][6][64] =

    // opening positional piece scores //
    {
        // pawn
        0, 0, 0, 0, 0, 0, 0, 0,
        98, 134, 61, 95, 68, 126, 34, -11,
        -6, 7, 26, 31, 65, 56, 25, -20,
        -14, 13, 6, 21, 23, 12, 17, -23,
        -27, -2, -5, 12, 17, 6, 10, -25,
        -26, -4, -4, -10, 3, 3, 33, -12,
        -35, -1, -20, -23, -15, 24, 38, -22,
        0, 0, 0, 0, 0, 0, 0, 0,

        // knight
        -167, -89, -34, -49, 61, -97, -15, -107,
        -73, -41, 72, 36, 23, 62, 7, -17,
        -47, 60, 37, 65, 84, 129, 73, 44,
        -9, 17, 19, 53, 37, 69, 18, 22,
        -13, 4, 16, 13, 28, 19, 21, -8,
        -23, -9, 12, 10, 19, 17, 25, -16,
        -29, -53, -12, -3, -1, 18, -14, -19,
        -105, -21, -58, -33, -17, -28, -19, -23,

        // bishop
        -29, 4, -82, -37, -25, -42, 7, -8,
        -26, 16, -18, -13, 30, 59, 18, -47,
        -16, 37, 43, 40, 35, 50, 37, -2,
        -4, 5, 19, 50, 37, 37, 7, -2,
        -6, 13, 13, 26, 34, 12, 10, 4,
        0, 15, 15, 15, 14, 27, 18, 10,
        4, 15, 16, 0, 7, 21, 33, 1,
        -33, -3, -14, -21, -13, -12, -39, -21,

        // rook
        32, 42, 32, 51, 63, 9, 31, 43,
        27, 32, 58, 62, 80, 67, 26, 44,
        -5, 19, 26, 36, 17, 45, 61, 16,
        -24, -11, 7, 26, 24, 35, -8, -20,
        -36, -26, -12, -1, 9, -7, 6, -23,
        -45, -25, -16, -17, 3, 0, -5, -33,
        -44, -16, -20, -9, -1, 11, -6, -71,
        -19, -13, 1, 17, 16, 7, -37, -26,

        // queen
        -28, 0, 29, 12, 59, 44, 43, 45,
        -24, -39, -5, 1, -16, 57, 28, 54,
        -13, -17, 7, 8, 29, 56, 47, 57,
        -27, -27, -16, -16, -1, 17, -2, 1,
        -9, -26, -9, -10, -2, -4, 3, -3,
        -14, 2, -11, -2, -5, 2, 14, 5,
        -35, -8, 11, 2, 8, 15, -3, 1,
        -1, -18, -9, 10, -15, -25, -31, -50,

        // king
        -65, 23, 16, -15, -56, -34, 2, 13,
        29, -1, -20, -7, -8, -4, -38, -29,
        -9, 24, 2, -16, -20, 6, 22, -22,
        -17, -20, -12, -27, -30, -25, -14, -36,
        -49, -1, -27, -39, -46, -44, -33, -51,
        -14, -14, -22, -46, -44, -30, -15, -27,
        1, 7, -8, -64, -43, -16, 9, 8,
        -15, 36, 12, -54, 8, -28, 24, 14,

        // Endgame positional piece scores //

        // pawn
        0, 0, 0, 0, 0, 0, 0, 0,
        178, 173, 158, 134, 147, 132, 165, 187,
        94, 100, 85, 67, 56, 53, 82, 84,
        32, 24, 13, 5, -2, 4, 17, 17,
        13, 9, -3, -7, -7, -8, 3, -1,
        4, 7, -6, 1, 0, -5, -1, -8,
        13, 8, 8, 10, 13, 0, 2, -7,
        0, 0, 0, 0, 0, 0, 0, 0,

        // knight
        -58, -38, -13, -28, -31, -27, -63, -99,
        -25, -8, -25, -2, -9, -25, -24, -52,
        -24, -20, 10, 9, -1, -9, -19, -41,
        -17, 3, 22, 22, 22, 11, 8, -18,
        -18, -6, 16, 25, 16, 17, 4, -18,
        -23, -3, -1, 15, 10, -3, -20, -22,
        -42, -20, -10, -5, -2, -20, -23, -44,
        -29, -51, -23, -15, -22, -18, -50, -64,

        // bishop
        -14, -21, -11, -8, -7, -9, -17, -24,
        -8, -4, 7, -12, -3, -13, -4, -14,
        2, -8, 0, -1, -2, 6, 0, 4,
        -3, 9, 12, 9, 14, 10, 3, 2,
        -6, 3, 13, 19, 7, 10, -3, -9,
        -12, -3, 8, 10, 13, 3, -7, -15,
        -14, -18, -7, -1, 4, -9, -15, -27,
        -23, -9, -23, -5, -9, -16, -5, -17,

        // rook
        13, 10, 18, 15, 12, 12, 8, 5,
        11, 13, 13, 11, -3, 3, 8, 3,
        7, 7, 7, 5, 4, -3, -5, -3,
        4, 3, 13, 1, 2, 1, -1, 2,
        3, 5, 8, 4, -5, -6, -8, -11,
        -4, 0, -5, -1, -7, -12, -8, -16,
        -6, -6, 0, 2, -9, -9, -11, -3,
        -9, 2, 3, -1, -5, -13, 4, -20,

        // queen
        -9, 22, 22, 27, 27, 19, 10, 20,
        -17, 20, 32, 41, 58, 25, 30, 0,
        -20, 6, 9, 49, 47, 35, 19, 9,
        3, 22, 24, 45, 57, 40, 57, 36,
        -18, 28, 19, 47, 31, 34, 39, 23,
        -16, -27, 15, 6, 9, 17, 10, 5,
        -22, -23, -30, -16, -16, -23, -36, -32,
        -33, -28, -22, -43, -5, -32, -20, -41,

        // king
        -74, -35, -18, -18, -11, 15, 4, -17,
        -12, 17, 14, 17, 17, 38, 23, 11,
        10, 17, 23, 15, 20, 45, 44, 13,
        -8, 22, 24, 27, 26, 33, 26, 3,
        -18, -4, 21, 24, 27, 23, 9, -11,
        -19, -3, 11, 21, 23, 16, 7, -9,
        -27, -11, 4, 13, 14, 4, -5, -17,
        -53, -34, -21, -11, -28, -14, -24, -43};

// mirror positional score tables for opposite side
const int lb_mirror_score[128] =
    {
        a1, b1, c1, d1, e1, f1, g1, h1,
        a2, b2, c2, d2, e2, f2, g2, h2,
        a3, b3, c3, d3, e3, f3, g3, h3,
        a4, b4, c4, d4, e4, f4, g4, h4,
        a5, b5, c5, d5, e5, f5, g5, h5,
        a6, b6, c6, d6, e6, f6, g6, h6,
        a7, b7, c7, d7, e7, f7, g7, h7,
        a8, b8, c8, d8, e8, f8, g8, h8};
const int rb_mirror_score[128] =
    {
        a1, b1, c1, d1, e1, f1, g1, h1,
        a2, b2, c2, d2, e2, f2, g2, h2,
        a3, b3, c3, d3, e3, f3, g3, h3,
        a4, b4, c4, d4, e4, f4, g4, h4,
        a5, b5, c5, d5, e5, f5, g5, h5,
        a6, b6, c6, d6, e6, f6, g6, h6,
        a7, b7, c7, d7, e7, f7, g7, h7,
        a8, b8, c8, d8, e8, f8, g8, h8};

/*
          Rank mask            File mask           Isolated mask        Passed pawn mask
        for square a6        for square f2         for square g2          for square c4

    8  0 0 0 0 0 0 0 0    8  0 0 0 0 0 1 0 0    8  0 0 0 0 0 1 0 1     8  0 1 1 1 0 0 0 0
    7  0 0 0 0 0 0 0 0    7  0 0 0 0 0 1 0 0    7  0 0 0 0 0 1 0 1     7  0 1 1 1 0 0 0 0
    6  1 1 1 1 1 1 1 1    6  0 0 0 0 0 1 0 0    6  0 0 0 0 0 1 0 1     6  0 1 1 1 0 0 0 0
    5  0 0 0 0 0 0 0 0    5  0 0 0 0 0 1 0 0    5  0 0 0 0 0 1 0 1     5  0 1 1 1 0 0 0 0
    4  0 0 0 0 0 0 0 0    4  0 0 0 0 0 1 0 0    4  0 0 0 0 0 1 0 1     4  0 0 0 0 0 0 0 0
    3  0 0 0 0 0 0 0 0    3  0 0 0 0 0 1 0 0    3  0 0 0 0 0 1 0 1     3  0 0 0 0 0 0 0 0
    2  0 0 0 0 0 0 0 0    2  0 0 0 0 0 1 0 0    2  0 0 0 0 0 1 0 1     2  0 0 0 0 0 0 0 0
    1  0 0 0 0 0 0 0 0    1  0 0 0 0 0 1 0 0    1  0 0 0 0 0 1 0 1     1  0 0 0 0 0 0 0 0

       a b c d e f g h       a b c d e f g h       a b c d e f g h        a b c d e f g h
*/

// file masks [square]
U64 lb_file_masks[64];
U64 rb_file_masks[64];

// rank masks [square]
U64 lb_rank_masks[64];
U64 rb_rank_masks[64];

// isolated pawn masks [square]
U64 lb_isolated_masks[64];
U64 rb_isolated_masks[64];

// white passed pawn masks [square]
U64 lb_white_passed_masks[64];
U64 rb_white_passed_masks[64];

// black passed pawn masks [square]
U64 lb_black_passed_masks[64];
U64 rb_black_passed_masks[64];

// extract rank from a square [square]
const int lb_get_rank[64] =
    {
        7, 7, 7, 7, 7, 7, 7, 7,
        6, 6, 6, 6, 6, 6, 6, 6,
        5, 5, 5, 5, 5, 5, 5, 5,
        4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0};
const int rb_get_rank[64] =
    {
        7, 7, 7, 7, 7, 7, 7, 7,
        6, 6, 6, 6, 6, 6, 6, 6,
        5, 5, 5, 5, 5, 5, 5, 5,
        4, 4, 4, 4, 4, 4, 4, 4,
        3, 3, 3, 3, 3, 3, 3, 3,
        2, 2, 2, 2, 2, 2, 2, 2,
        1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0};

// double pawns penalty
const int lb_double_pawn_penalty_opening = -5;
const int rb_double_pawn_penalty_opening = -5;
const int lb_double_pawn_penalty_endgame = -10;
const int rb_double_pawn_penalty_endgame = -10;

// isolated pawn penalty
const int lb_isolated_pawn_penalty_opening = -5;
const int rb_isolated_pawn_penalty_opening = -5;
const int lb_isolated_pawn_penalty_endgame = -10;
const int rb_isolated_pawn_penalty_endgame = -10;

// passed pawn bonus
const int lb_passed_pawn_bonus[8] = {0, 10, 30, 50, 75, 100, 150, 200};
const int rb_passed_pawn_bonus[8] = {0, 10, 30, 50, 75, 100, 150, 200};

// semi open file score
const int lb_semi_open_file_score = 10;
const int rb_semi_open_file_score = 10;

// open file score
const int lb_open_file_score = 15;
const int rb_open_file_score = 15;

// mobility units (values from engine Fruit reloaded)
static const int lb_bishop_unit = 4;
static const int rb_bishop_unit = 4;
static const int lb_queen_unit = 9;
static const int rb_queen_unit = 9;

// mobility bonuses (values from engine Fruit reloaded)
static const int lb_bishop_mobility_opening = 5;
static const int rb_bishop_mobility_opening = 5;
static const int lb_bishop_mobility_endgame = 5;
static const int rb_bishop_mobility_endgame = 5;
static const int lb_queen_mobility_opening = 1;
static const int rb_queen_mobility_opening = 1;
static const int lb_queen_mobility_endgame = 2;
static const int rb_queen_mobility_endgame = 2;

// king's shield bonus
const int lb_king_shield_bonus = 5;
const int rb_king_shield_bonus = 5;

// set file or rank mask
U64 lb_set_file_rank_mask(int file_number, int rank_number)
{
    // file or rank mask
    U64 mask = 0ULL;

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            if (file_number != -1)
            {
                // on file match
                if (file == file_number)
                    // set bit on mask
                    set_bit(mask, square);
            }

            else if (rank_number != -1)
            {
                // on rank match
                if (rank == rank_number)
                    // set bit on mask
                    set_bit(mask, square);
            }
        }
    }

    // return mask
    return mask;
}

U64 rb_set_file_rank_mask(int file_number, int rank_number)
{
    // file or rank mask
    U64 mask = 0ULL;

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            if (file_number != -1)
            {
                // on file match
                if (file == file_number)
                    // set bit on mask
                    set_bit(mask, square);
            }

            else if (rank_number != -1)
            {
                // on rank match
                if (rank == rank_number)
                    // set bit on mask
                    set_bit(mask, square);
            }
        }
    }

    // return mask
    return mask;
}

// init evaluation masks
void lb_init_evaluation_masks()
{
    /******** Init file masks ********/

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // init file mask for a current square
            lb_file_masks[square] |= lb_set_file_rank_mask(file, -1);
        }
    }

    /******** Init rank masks ********/

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // init rank mask for a current square
            lb_rank_masks[square] |= lb_set_file_rank_mask(-1, rank);
        }
    }

    /******** Init isolated masks ********/

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // init isolated pawns masks for a current square
            lb_isolated_masks[square] |= lb_set_file_rank_mask(file - 1, -1);
            lb_isolated_masks[square] |= lb_set_file_rank_mask(file + 1, -1);
        }
    }

    /******** White passed masks ********/

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // init white passed pawns mask for a current square
            lb_white_passed_masks[square] |= lb_set_file_rank_mask(file - 1, -1);
            lb_white_passed_masks[square] |= lb_set_file_rank_mask(file, -1);
            lb_white_passed_masks[square] |= lb_set_file_rank_mask(file + 1, -1);

            // loop over redudant ranks
            for (int i = 0; i < (8 - rank); i++)
                // reset redudant bits
                lb_white_passed_masks[square] &= ~lb_rank_masks[(7 - i) * 8 + file];
        }
    }

    /******** Black passed masks ********/

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // init black passed pawns mask for a current square
            lb_black_passed_masks[square] |= lb_set_file_rank_mask(file - 1, -1);
            lb_black_passed_masks[square] |= lb_set_file_rank_mask(file, -1);
            lb_black_passed_masks[square] |= lb_set_file_rank_mask(file + 1, -1);

            // loop over redudant ranks
            for (int i = 0; i < rank + 1; i++)
                // reset redudant bits
                lb_black_passed_masks[square] &= ~lb_rank_masks[i * 8 + file];
        }
    }
}

void rb_init_evaluation_masks()
{
    /******** Init file masks ********/

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // init file mask for a current square
            rb_file_masks[square] |= rb_set_file_rank_mask(file, -1);
        }
    }

    /******** Init rank masks ********/

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // init rank mask for a current square
            rb_rank_masks[square] |= rb_set_file_rank_mask(-1, rank);
        }
    }

    /******** Init isolated masks ********/

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // init isolated pawns masks for a current square
            rb_isolated_masks[square] |= rb_set_file_rank_mask(file - 1, -1);
            rb_isolated_masks[square] |= rb_set_file_rank_mask(file + 1, -1);
        }
    }

    /******** White passed masks ********/

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // init white passed pawns mask for a current square
            rb_white_passed_masks[square] |= rb_set_file_rank_mask(file - 1, -1);
            rb_white_passed_masks[square] |= rb_set_file_rank_mask(file, -1);
            rb_white_passed_masks[square] |= rb_set_file_rank_mask(file + 1, -1);

            // loop over redudant ranks
            for (int i = 0; i < (8 - rank); i++)
                // reset redudant bits
                rb_white_passed_masks[square] &= ~rb_rank_masks[(7 - i) * 8 + file];
        }
    }

    /******** Black passed masks ********/

    // loop over ranks
    for (int rank = 0; rank < 8; rank++)
    {
        // loop over files
        for (int file = 0; file < 8; file++)
        {
            // init square
            int square = rank * 8 + file;

            // init black passed pawns mask for a current square
            rb_black_passed_masks[square] |= rb_set_file_rank_mask(file - 1, -1);
            rb_black_passed_masks[square] |= rb_set_file_rank_mask(file, -1);
            rb_black_passed_masks[square] |= rb_set_file_rank_mask(file + 1, -1);

            // loop over redudant ranks
            for (int i = 0; i < rank + 1; i++)
                // reset redudant bits
                rb_black_passed_masks[square] &= ~rb_rank_masks[i * 8 + file];
        }
    }
}

// get game phase score
static inline int lb_get_game_phase_score()
{
    /*
        The game phase score of the game is derived from the pieces
        (not counting pawns and kings) that are still on the board.
        The full material starting position game phase score is:

        4 * knight material score in the opening +
        4 * bishop material score in the opening +
        4 * rook material score in the opening +
        2 * queen material score in the opening
    */

    // white & black game phase scores
    int white_piece_scores = 0, black_piece_scores = 0;

    // loop over white pieces
    for (int piece = N; piece <= Q; piece++)
        white_piece_scores += lb_count_bits(lb_bitboards[piece]) * lb_material_score[opening][piece];

    // loop over white pieces
    for (int piece = n; piece <= q; piece++)
        black_piece_scores += lb_count_bits(lb_bitboards[piece]) * -lb_material_score[opening][piece];

    // return game phase score
    return white_piece_scores + black_piece_scores;
}

static inline int rb_get_game_phase_score()
{
    /*
        The game phase score of the game is derived from the pieces
        (not counting pawns and kings) that are still on the board.
        The full material starting position game phase score is:

        4 * knight material score in the opening +
        4 * bishop material score in the opening +
        4 * rook material score in the opening +
        2 * queen material score in the opening
    */

    // white & black game phase scores
    int white_piece_scores = 0, black_piece_scores = 0;

    // loop over white pieces
    for (int piece = N; piece <= Q; piece++)
        white_piece_scores += rb_count_bits(rb_bitboards[piece]) * rb_material_score[opening][piece];

    // loop over white pieces
    for (int piece = n; piece <= q; piece++)
        black_piece_scores += rb_count_bits(rb_bitboards[piece]) * -rb_material_score[opening][piece];

    // return game phase score
    return white_piece_scores + black_piece_scores;
}

// position evaluation
static inline int lb_evaluate()
{
    // get game phase score
    int game_phase_score = lb_get_game_phase_score();

    // game phase (opening, middle game, endgame)
    int game_phase = -1;

    // pick up game phase based on game phase score
    if (game_phase_score > lb_opening_phase_score)
        game_phase = opening;
    else if (game_phase_score < lb_endgame_phase_score)
        game_phase = endgame;
    else
        game_phase = middlegame;

    // static evaluation score
    int score = 0, score_opening = 0, score_endgame = 0;

    // current pieces bitboard copy
    U64 bitboard;

    // init piece & square
    int piece, square;

    // penalties
    int double_pawns = 0;

    // loop over piece bitboards
    for (int bb_piece = P; bb_piece <= k; bb_piece++)
    {
        // init piece bitboard copy
        bitboard = lb_bitboards[bb_piece];

        // loop over pieces within a bitboard
        while (bitboard)
        {
            // init piece
            piece = bb_piece;

            // init square
            square = lb_get_ls1b_index(bitboard);

            // get opening/endgame material score
            score_opening += lb_material_score[opening][piece];
            score_endgame += lb_material_score[endgame][piece];

            // score positional piece scores
            switch (piece)
            {
            // evaluate white pawns
            case P:
                // get opening/endgame positional score
                score_opening += lb_positional_score[opening][PAWN][square];
                score_endgame += lb_positional_score[endgame][PAWN][square];

                // double pawn penalty
                double_pawns = lb_count_bits(lb_bitboards[P] & lb_file_masks[square]);

                // on double pawns (tripple, etc)
                if (double_pawns > 1)
                {
                    score_opening += (double_pawns - 1) * lb_double_pawn_penalty_opening;
                    score_endgame += (double_pawns - 1) * lb_double_pawn_penalty_endgame;
                }

                // on isolated pawn
                if ((lb_bitboards[P] & lb_isolated_masks[square]) == 0)
                {
                    // give an isolated pawn penalty
                    score_opening += lb_isolated_pawn_penalty_opening;
                    score_endgame += lb_isolated_pawn_penalty_endgame;
                }
                // on passed pawn
                if ((lb_white_passed_masks[square] & lb_bitboards[p]) == 0)
                {
                    // give passed pawn bonus
                    score_opening += lb_passed_pawn_bonus[lb_get_rank[square]];
                    score_endgame += lb_passed_pawn_bonus[lb_get_rank[square]];
                }

                break;

            // evaluate white knights
            case N:
                // get opening/endgame positional score
                score_opening += lb_positional_score[opening][KNIGHT][square];
                score_endgame += lb_positional_score[endgame][KNIGHT][square];

                break;

            // evaluate white bishops
            case B:
                // get opening/endgame positional score
                score_opening += lb_positional_score[opening][BISHOP][square];
                score_endgame += lb_positional_score[endgame][BISHOP][square];

                // mobility
                score_opening += (lb_count_bits(lb_get_bishop_attacks(square, lb_occupancies[both])) - lb_bishop_unit) * lb_bishop_mobility_opening;
                score_endgame += (lb_count_bits(lb_get_bishop_attacks(square, lb_occupancies[both])) - lb_bishop_unit) * lb_bishop_mobility_endgame;
                break;

            // evaluate white rooks
            case R:
                // get opening/endgame positional score
                score_opening += lb_positional_score[opening][ROOK][square];
                score_endgame += lb_positional_score[endgame][ROOK][square];

                // semi open file
                if ((lb_bitboards[P] & lb_file_masks[square]) == 0)
                {
                    // add semi open file bonus
                    score_opening += lb_semi_open_file_score;
                    score_endgame += lb_semi_open_file_score;
                }

                // semi open file
                if (((lb_bitboards[P] | lb_bitboards[p]) & lb_file_masks[square]) == 0)
                {
                    // add semi open file bonus
                    score_opening += lb_open_file_score;
                    score_endgame += lb_open_file_score;
                }

                break;

            // evaluate white queens
            case Q:
                // get opening/endgame positional score
                score_opening += lb_positional_score[opening][QUEEN][square];
                score_endgame += lb_positional_score[endgame][QUEEN][square];

                // mobility
                score_opening += (lb_count_bits(lb_get_queen_attacks(square, lb_occupancies[both])) - lb_queen_unit) * lb_queen_mobility_opening;
                score_endgame += (lb_count_bits(lb_get_queen_attacks(square, lb_occupancies[both])) - lb_queen_unit) * lb_queen_mobility_endgame;
                break;

            // evaluate white king
            case K:
                // get opening/endgame positional score
                score_opening += lb_positional_score[opening][KING][square];
                score_endgame += lb_positional_score[endgame][KING][square];

                // semi open file
                if ((lb_bitboards[P] & lb_file_masks[square]) == 0)
                {
                    // add semi open file penalty
                    score_opening -= lb_semi_open_file_score;
                    score_endgame -= lb_semi_open_file_score;
                }

                // semi open file
                if (((lb_bitboards[P] | lb_bitboards[p]) & lb_file_masks[square]) == 0)
                {
                    // add semi open file penalty
                    score_opening -= lb_open_file_score;
                    score_endgame -= lb_open_file_score;
                }

                // king safety bonus
                score_opening += lb_count_bits(lb_king_attacks[square] & lb_occupancies[white]) * lb_king_shield_bonus;
                score_endgame += lb_count_bits(lb_king_attacks[square] & lb_occupancies[white]) * lb_king_shield_bonus;

                break;

            // evaluate black pawns
            case p:
                // get opening/endgame positional score
                score_opening -= lb_positional_score[opening][PAWN][lb_mirror_score[square]];
                score_endgame -= lb_positional_score[endgame][PAWN][lb_mirror_score[square]];

                // double pawn penalty
                double_pawns = lb_count_bits(lb_bitboards[p] & lb_file_masks[square]);

                // on double pawns (tripple, etc)
                if (double_pawns > 1)
                {
                    score_opening -= (double_pawns - 1) * lb_double_pawn_penalty_opening;
                    score_endgame -= (double_pawns - 1) * lb_double_pawn_penalty_endgame;
                }

                // on isolated pawn
                if ((lb_bitboards[p] & lb_isolated_masks[square]) == 0)
                {
                    // give an isolated pawn penalty
                    score_opening -= lb_isolated_pawn_penalty_opening;
                    score_endgame -= lb_isolated_pawn_penalty_endgame;
                }
                // on passed pawn
                if ((lb_black_passed_masks[square] & lb_bitboards[P]) == 0)
                {
                    // give passed pawn bonus
                    score_opening -= lb_passed_pawn_bonus[lb_get_rank[square]];
                    score_endgame -= lb_passed_pawn_bonus[lb_get_rank[square]];
                }

                break;

            // evaluate black knights
            case n:
                // get opening/endgame positional score
                score_opening -= lb_positional_score[opening][KNIGHT][lb_mirror_score[square]];
                score_endgame -= lb_positional_score[endgame][KNIGHT][lb_mirror_score[square]];

                break;

            // evaluate black bishops
            case b:
                // get opening/endgame positional score
                score_opening -= lb_positional_score[opening][BISHOP][lb_mirror_score[square]];
                score_endgame -= lb_positional_score[endgame][BISHOP][lb_mirror_score[square]];

                // mobility
                score_opening -= (lb_count_bits(lb_get_bishop_attacks(square, lb_occupancies[both])) - lb_bishop_unit) * lb_bishop_mobility_opening;
                score_endgame -= (lb_count_bits(lb_get_bishop_attacks(square, lb_occupancies[both])) - lb_bishop_unit) * lb_bishop_mobility_endgame;
                break;

            // evaluate black rooks
            case r:
                // get opening/endgame positional score
                score_opening -= lb_positional_score[opening][ROOK][lb_mirror_score[square]];
                score_endgame -= lb_positional_score[endgame][ROOK][lb_mirror_score[square]];

                // semi open file
                if ((lb_bitboards[p] & lb_file_masks[square]) == 0)
                {
                    // add semi open file bonus
                    score_opening -= lb_semi_open_file_score;
                    score_endgame -= lb_semi_open_file_score;
                }

                // semi open file
                if (((lb_bitboards[P] | lb_bitboards[p]) & lb_file_masks[square]) == 0)
                {
                    // add semi open file bonus
                    score_opening -= lb_open_file_score;
                    score_endgame -= lb_open_file_score;
                }

                break;

            // evaluate black queens
            case q:
                // get opening/endgame positional score
                score_opening -= lb_positional_score[opening][QUEEN][lb_mirror_score[square]];
                score_endgame -= lb_positional_score[endgame][QUEEN][lb_mirror_score[square]];

                // mobility
                score_opening -= (lb_count_bits(lb_get_queen_attacks(square, lb_occupancies[both])) - lb_queen_unit) * lb_queen_mobility_opening;
                score_endgame -= (lb_count_bits(lb_get_queen_attacks(square, lb_occupancies[both])) - lb_queen_unit) * lb_queen_mobility_endgame;
                break;

            // evaluate black king
            case k:
                // get opening/endgame positional score
                score_opening -= lb_positional_score[opening][KING][lb_mirror_score[square]];
                score_endgame -= lb_positional_score[endgame][KING][lb_mirror_score[square]];

                // semi open file
                if ((lb_bitboards[p] & lb_file_masks[square]) == 0)
                {
                    // add semi open file penalty
                    score_opening += lb_semi_open_file_score;
                    score_endgame += lb_semi_open_file_score;
                }

                // semi open file
                if (((lb_bitboards[P] | lb_bitboards[p]) & lb_file_masks[square]) == 0)
                {
                    // add semi open file penalty
                    score_opening += lb_open_file_score;
                    score_endgame += lb_open_file_score;
                }

                // king safety bonus
                score_opening -= lb_count_bits(lb_king_attacks[square] & lb_occupancies[black]) * lb_king_shield_bonus;
                score_endgame -= lb_count_bits(lb_king_attacks[square] & lb_occupancies[black]) * lb_king_shield_bonus;
                break;
            }

            // pop ls1b
            pop_bit(bitboard, square);
        }
    }

    /*
        Now in order to calculate interpolated score
        for a given game phase we use this formula
        (same for material and positional scores):

        (
          score_opening * game_phase_score +
          score_endgame * (opening_phase_score - game_phase_score)
        ) / opening_phase_score

        E.g. the score for pawn on d4 at phase say 5000 would be
        interpolated_score = (12 * 5000 + (-7) * (6192 - 5000)) / 6192 = 8,342377261
    */

    // interpolate score in the middlegame
    if (game_phase == middlegame)
        score = (score_opening * game_phase_score +
                 score_endgame * (lb_opening_phase_score - game_phase_score)) /
                lb_opening_phase_score;

    // return pure opening score in opening
    else if (game_phase == opening)
        score = score_opening;

    // return pure endgame score in endgame
    else if (game_phase == endgame)
        score = score_endgame;

    // return final evaluation based on side
    return (lb_side == white) ? score : -score;
}

static inline int rb_evaluate()
{
    // get game phase score
    int game_phase_score = rb_get_game_phase_score();

    // game phase (opening, middle game, endgame)
    int game_phase = -1;

    // pick up game phase based on game phase score
    if (game_phase_score > rb_opening_phase_score)
        game_phase = opening;
    else if (game_phase_score < rb_endgame_phase_score)
        game_phase = endgame;
    else
        game_phase = middlegame;

    // static evaluation score
    int score = 0, score_opening = 0, score_endgame = 0;

    // current pieces bitboard copy
    U64 bitboard;

    // init piece & square
    int piece, square;

    // penalties
    int double_pawns = 0;

    // loop over piece bitboards
    for (int bb_piece = P; bb_piece <= k; bb_piece++)
    {
        // init piece bitboard copy
        bitboard = rb_bitboards[bb_piece];

        // loop over pieces within a bitboard
        while (bitboard)
        {
            // init piece
            piece = bb_piece;

            // init square
            square = rb_get_ls1b_index(bitboard);

            // get opening/endgame material score
            score_opening += rb_material_score[opening][piece];
            score_endgame += rb_material_score[endgame][piece];

            // score positional piece scores
            switch (piece)
            {
            // evaluate white pawns
            case P:
                // get opening/endgame positional score
                score_opening += rb_positional_score[opening][PAWN][square];
                score_endgame += rb_positional_score[endgame][PAWN][square];

                // double pawn penalty
                double_pawns = rb_count_bits(rb_bitboards[P] & rb_file_masks[square]);

                // on double pawns (tripple, etc)
                if (double_pawns > 1)
                {
                    score_opening += (double_pawns - 1) * rb_double_pawn_penalty_opening;
                    score_endgame += (double_pawns - 1) * rb_double_pawn_penalty_endgame;
                }

                // on isolated pawn
                if ((rb_bitboards[P] & rb_isolated_masks[square]) == 0)
                {
                    // give an isolated pawn penalty
                    score_opening += rb_isolated_pawn_penalty_opening;
                    score_endgame += rb_isolated_pawn_penalty_endgame;
                }
                // on passed pawn
                if ((rb_white_passed_masks[square] & rb_bitboards[p]) == 0)
                {
                    // give passed pawn bonus
                    score_opening += rb_passed_pawn_bonus[rb_get_rank[square]];
                    score_endgame += rb_passed_pawn_bonus[rb_get_rank[square]];
                }

                break;

            // evaluate white knights
            case N:
                // get opening/endgame positional score
                score_opening += rb_positional_score[opening][KNIGHT][square];
                score_endgame += rb_positional_score[endgame][KNIGHT][square];

                break;

            // evaluate white bishops
            case B:
                // get opening/endgame positional score
                score_opening += rb_positional_score[opening][BISHOP][square];
                score_endgame += rb_positional_score[endgame][BISHOP][square];

                // mobility
                score_opening += (rb_count_bits(rb_get_bishop_attacks(square, rb_occupancies[both])) - rb_bishop_unit) * rb_bishop_mobility_opening;
                score_endgame += (rb_count_bits(rb_get_bishop_attacks(square, rb_occupancies[both])) - rb_bishop_unit) * rb_bishop_mobility_endgame;
                break;

            // evaluate white rooks
            case R:
                // get opening/endgame positional score
                score_opening += rb_positional_score[opening][ROOK][square];
                score_endgame += rb_positional_score[endgame][ROOK][square];

                // semi open file
                if ((rb_bitboards[P] & rb_file_masks[square]) == 0)
                {
                    // add semi open file bonus
                    score_opening += rb_semi_open_file_score;
                    score_endgame += rb_semi_open_file_score;
                }

                // semi open file
                if (((rb_bitboards[P] | rb_bitboards[p]) & rb_file_masks[square]) == 0)
                {
                    // add semi open file bonus
                    score_opening += rb_open_file_score;
                    score_endgame += rb_open_file_score;
                }

                break;

            // evaluate white queens
            case Q:
                // get opening/endgame positional score
                score_opening += rb_positional_score[opening][QUEEN][square];
                score_endgame += rb_positional_score[endgame][QUEEN][square];

                // mobility
                score_opening += (rb_count_bits(rb_get_queen_attacks(square, rb_occupancies[both])) - rb_queen_unit) * rb_queen_mobility_opening;
                score_endgame += (rb_count_bits(rb_get_queen_attacks(square, rb_occupancies[both])) - rb_queen_unit) * rb_queen_mobility_endgame;
                break;

            // evaluate white king
            case K:
                // get opening/endgame positional score
                score_opening += rb_positional_score[opening][KING][square];
                score_endgame += rb_positional_score[endgame][KING][square];

                // semi open file
                if ((rb_bitboards[P] & rb_file_masks[square]) == 0)
                {
                    // add semi open file penalty
                    score_opening -= rb_semi_open_file_score;
                    score_endgame -= rb_semi_open_file_score;
                }

                // semi open file
                if (((rb_bitboards[P] | rb_bitboards[p]) & rb_file_masks[square]) == 0)
                {
                    // add semi open file penalty
                    score_opening -= rb_open_file_score;
                    score_endgame -= rb_open_file_score;
                }

                // king safety bonus
                score_opening += rb_count_bits(rb_king_attacks[square] & rb_occupancies[white]) * rb_king_shield_bonus;
                score_endgame += rb_count_bits(rb_king_attacks[square] & rb_occupancies[white]) * rb_king_shield_bonus;

                break;

            // evaluate black pawns
            case p:
                // get opening/endgame positional score
                score_opening -= rb_positional_score[opening][PAWN][rb_mirror_score[square]];
                score_endgame -= rb_positional_score[endgame][PAWN][rb_mirror_score[square]];

                // double pawn penalty
                double_pawns = rb_count_bits(rb_bitboards[p] & rb_file_masks[square]);

                // on double pawns (tripple, etc)
                if (double_pawns > 1)
                {
                    score_opening -= (double_pawns - 1) * rb_double_pawn_penalty_opening;
                    score_endgame -= (double_pawns - 1) * rb_double_pawn_penalty_endgame;
                }

                // on isolated pawn
                if ((rb_bitboards[p] & rb_isolated_masks[square]) == 0)
                {
                    // give an isolated pawn penalty
                    score_opening -= rb_isolated_pawn_penalty_opening;
                    score_endgame -= rb_isolated_pawn_penalty_endgame;
                }
                // on passed pawn
                if ((rb_black_passed_masks[square] & rb_bitboards[P]) == 0)
                {
                    // give passed pawn bonus
                    score_opening -= rb_passed_pawn_bonus[rb_get_rank[square]];
                    score_endgame -= rb_passed_pawn_bonus[rb_get_rank[square]];
                }

                break;

            // evaluate black knights
            case n:
                // get opening/endgame positional score
                score_opening -= rb_positional_score[opening][KNIGHT][rb_mirror_score[square]];
                score_endgame -= rb_positional_score[endgame][KNIGHT][rb_mirror_score[square]];

                break;

            // evaluate black bishops
            case b:
                // get opening/endgame positional score
                score_opening -= rb_positional_score[opening][BISHOP][rb_mirror_score[square]];
                score_endgame -= rb_positional_score[endgame][BISHOP][rb_mirror_score[square]];

                // mobility
                score_opening -= (rb_count_bits(rb_get_bishop_attacks(square, rb_occupancies[both])) - rb_bishop_unit) * rb_bishop_mobility_opening;
                score_endgame -= (rb_count_bits(rb_get_bishop_attacks(square, rb_occupancies[both])) - rb_bishop_unit) * rb_bishop_mobility_endgame;
                break;

            // evaluate black rooks
            case r:
                // get opening/endgame positional score
                score_opening -= rb_positional_score[opening][ROOK][rb_mirror_score[square]];
                score_endgame -= rb_positional_score[endgame][ROOK][rb_mirror_score[square]];

                // semi open file
                if ((rb_bitboards[p] & rb_file_masks[square]) == 0)
                {
                    // add semi open file bonus
                    score_opening -= rb_semi_open_file_score;
                    score_endgame -= rb_semi_open_file_score;
                }

                // semi open file
                if (((rb_bitboards[P] | rb_bitboards[p]) & rb_file_masks[square]) == 0)
                {
                    // add semi open file bonus
                    score_opening -= rb_open_file_score;
                    score_endgame -= rb_open_file_score;
                }

                break;

            // evaluate black queens
            case q:
                // get opening/endgame positional score
                score_opening -= rb_positional_score[opening][QUEEN][rb_mirror_score[square]];
                score_endgame -= rb_positional_score[endgame][QUEEN][rb_mirror_score[square]];

                // mobility
                score_opening -= (rb_count_bits(rb_get_queen_attacks(square, rb_occupancies[both])) - rb_queen_unit) * rb_queen_mobility_opening;
                score_endgame -= (rb_count_bits(rb_get_queen_attacks(square, rb_occupancies[both])) - rb_queen_unit) * rb_queen_mobility_endgame;
                break;

            // evaluate black king
            case k:
                // get opening/endgame positional score
                score_opening -= rb_positional_score[opening][KING][rb_mirror_score[square]];
                score_endgame -= rb_positional_score[endgame][KING][rb_mirror_score[square]];

                // semi open file
                if ((rb_bitboards[p] & rb_file_masks[square]) == 0)
                {
                    // add semi open file penalty
                    score_opening += rb_semi_open_file_score;
                    score_endgame += rb_semi_open_file_score;
                }

                // semi open file
                if (((rb_bitboards[P] | rb_bitboards[p]) & rb_file_masks[square]) == 0)
                {
                    // add semi open file penalty
                    score_opening += rb_open_file_score;
                    score_endgame += rb_open_file_score;
                }

                // king safety bonus
                score_opening -= rb_count_bits(rb_king_attacks[square] & rb_occupancies[black]) * rb_king_shield_bonus;
                score_endgame -= rb_count_bits(rb_king_attacks[square] & rb_occupancies[black]) * rb_king_shield_bonus;
                break;
            }

            // pop ls1b
            pop_bit(bitboard, square);
        }
    }

    /*
        Now in order to calculate interpolated score
        for a given game phase we use this formula
        (same for material and positional scores):

        (
          score_opening * game_phase_score +
          score_endgame * (opening_phase_score - game_phase_score)
        ) / opening_phase_score

        E.g. the score for pawn on d4 at phase say 5000 would be
        interpolated_score = (12 * 5000 + (-7) * (6192 - 5000)) / 6192 = 8,342377261
    */

    // interpolate score in the middlegame
    if (game_phase == middlegame)
        score = (score_opening * game_phase_score +
                 score_endgame * (rb_opening_phase_score - game_phase_score)) /
                rb_opening_phase_score;

    // return pure opening score in opening
    else if (game_phase == opening)
        score = score_opening;

    // return pure endgame score in endgame
    else if (game_phase == endgame)
        score = score_endgame;

    // return final evaluation based on side
    return (rb_side == white) ? score : -score;
}

/**********************************\
 ==================================
 
               Search
 
 ==================================
\**********************************/

// most valuable victim & less valuable attacker

/*

    (Victims) Pawn Knight Bishop   Rook  Queen   King
  (Attackers)
        Pawn   105    205    305    405    505    605
      Knight   104    204    304    404    504    604
      Bishop   103    203    303    403    503    603
        Rook   102    202    302    402    502    602
       Queen   101    201    301    401    501    601
        King   100    200    300    400    500    600

*/

// MVV LVA [attacker][victim]
static int lb_mvv_lva[12][12] = {
    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600,

    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600};

static int rb_mvv_lva[12][12] = {
    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600,

    105, 205, 305, 405, 505, 605, 105, 205, 305, 405, 505, 605,
    104, 204, 304, 404, 504, 604, 104, 204, 304, 404, 504, 604,
    103, 203, 303, 403, 503, 603, 103, 203, 303, 403, 503, 603,
    102, 202, 302, 402, 502, 602, 102, 202, 302, 402, 502, 602,
    101, 201, 301, 401, 501, 601, 101, 201, 301, 401, 501, 601,
    100, 200, 300, 400, 500, 600, 100, 200, 300, 400, 500, 600};

///<ADD>    
// values for pieces put on the board
static int lb_put_value[11] = {
    100, 300, 325, 500, 900, 0, 100, 300, 325, 500, 900};

static int rb_put_value[11] = {
    100, 300, 300, 500, 900, 0, 100, 300, 300, 500, 900};

///</ADD>    

// killer moves [id][ply]
int lb_killer_moves[2][max_ply];
int rb_killer_moves[2][max_ply];

// history moves [piece][square]
int lb_history_moves[12][64];
int rb_history_moves[12][64];

/*
      ================================
            Triangular PV table
      --------------------------------
        PV line: e2e4 e7e5 g1f3 b8c6
      ================================

           0    1    2    3    4    5

      0    m1   m2   m3   m4   m5   m6

      1    0    m2   m3   m4   m5   m6

      2    0    0    m3   m4   m5   m6

      3    0    0    0    m4   m5   m6

      4    0    0    0    0    m5   m6

      5    0    0    0    0    0    m6
*/

// PV length [ply]
int lb_pv_length[max_ply];
int rb_pv_length[max_ply];

// PV table [ply][ply]
int lb_pv_table[max_ply][max_ply];
int rb_pv_table[max_ply][max_ply];

// follow PV & score PV move
int lb_follow_pv, lb_score_pv;
int rb_follow_pv, rb_score_pv;

/**********************************\
 ==================================
 
        Transposition table
 
 ==================================
\**********************************/

// number hash table entries
int lb_hash_entries = 0;
int rb_hash_entries = 0;

// define TT instance
tt *lb_hash_table = NULL;
tt *rb_hash_table = NULL;

// clear TT (hash table)
void lb_clear_hash_table()
{
    // init hash table entry pointer
    tt *hash_entry;

    // loop over TT elements
    for (hash_entry = lb_hash_table; hash_entry < lb_hash_table + lb_hash_entries; hash_entry++)
    {
        // reset TT inner fields
        hash_entry->hash_key = 0;
        hash_entry->depth = 0;
        hash_entry->flag = 0;
        hash_entry->score = 0;
    }
}

void rb_clear_hash_table()
{
    // init hash table entry pointer
    tt *hash_entry;

    // loop over TT elements
    for (hash_entry = rb_hash_table; hash_entry < rb_hash_table + rb_hash_entries; hash_entry++)
    {
        // reset TT inner fields
        hash_entry->hash_key = 0;
        hash_entry->depth = 0;
        hash_entry->flag = 0;
        hash_entry->score = 0;
    }
}

// dynamically allocate memory for hash table
void lb_init_hash_table(int mb)
{
    // init hash size
    int hash_size = 0x100000 * mb;

    // init number of hash entries
    lb_hash_entries = hash_size / sizeof(tt);

    // free hash table if not empty
    if (lb_hash_table != NULL)
    {
#ifndef NDEBUG // print only in debug mode
        printf("\nlb\n    Clearing hash memory...\n");
#endif
        // free hash table dynamic memory
        free(lb_hash_table);
    }

    // allocate memory
    lb_hash_table = (tt *)malloc(lb_hash_entries * sizeof(tt));

    // if allocation has failed
    if (lb_hash_table == NULL)
    {
#ifndef NDEBUG // print only in debug mode
        printf("   Couldn't allocate memory for hash table, tryinr %dMB...", mb / 2);
#endif

        // try to allocate with half size
        lb_init_hash_table(mb / 2);
    }

    // if allocation succeeded
    else
    {
        // clear hash table
        lb_clear_hash_table();
#ifndef NDEBUG // print only in debug mode
        printf("   Hash table is initialied with %d entries\n", lb_hash_entries);
#endif
    }
}

void rb_init_hash_table(int mb)
{
    // init hash size
    int hash_size = 0x100000 * mb;

    // init number of hash entries
    rb_hash_entries = hash_size / sizeof(tt);

    // free hash table if not empty
    if (rb_hash_table != NULL)
    {
#ifndef NDEBUG // print only in debug mode
        printf("\nrb\n    Clearing hash memory...\n");
#endif

        // free hash table dynamic memory
        free(rb_hash_table);
    }

    // allocate memory
    rb_hash_table = (tt *)malloc(rb_hash_entries * sizeof(tt));

    // if allocation has failed
    if (rb_hash_table == NULL)
    {
#ifndef NDEBUG // print only in debug mode
        printf("    Couldn't allocate memory for hash table, tryinr %dMB...", mb / 2);
#endif

        // try to allocate with half size
        rb_init_hash_table(mb / 2);
    }

    // if allocation succeeded
    else
    {
        // clear hash table
        rb_clear_hash_table();
#ifndef NDEBUG // print only in debug mode
        printf("    Hash table is initialied with %d entries\n", rb_hash_entries);
#endif
    }
}

// read hash entry data
static inline int lb_read_hash_entry(int alpha, int beta, int depth)
{
    // create a TT instance pointer to particular hash entry storing
    // the scoring data for the current board position if available
    tt *hash_entry = &lb_hash_table[lb_hash_key % lb_hash_entries];

    // make sure we're dealing with the exact position we need
    if (hash_entry->hash_key == lb_hash_key)
    {
        // make sure that we match the exact depth our search is now at
        if (hash_entry->depth >= depth)
        {
            // extract stored score from TT entry
            int score = hash_entry->score;

            // retrieve score independent from the actual path
            // from root node (position) to current node (position)
            if (score < -mate_score)
                score += lb_ply;
            if (score > mate_score)
                score -= lb_ply;

            // match the exact (PV node) score
            if (hash_entry->flag == hash_flag_exact)
                // return exact (PV node) score
                return score;

            // match alpha (fail-low node) score
            if ((hash_entry->flag == hash_flag_alpha) &&
                (score <= alpha))
                // return alpha (fail-low node) score
                return alpha;

            // match beta (fail-high node) score
            if ((hash_entry->flag == hash_flag_beta) &&
                (score >= beta))
                // return beta (fail-high node) score
                return beta;
        }
    }

    // if hash entry doesn't exist
    return no_hash_entry;
}

static inline int rb_read_hash_entry(int alpha, int beta, int depth)
{
    // create a TT instance pointer to particular hash entry storing
    // the scoring data for the current board position if available
    tt *hash_entry = &rb_hash_table[rb_hash_key % rb_hash_entries];

    // make sure we're dealing with the exact position we need
    if (hash_entry->hash_key == rb_hash_key)
    {
        // make sure that we match the exact depth our search is now at
        if (hash_entry->depth >= depth)
        {
            // extract stored score from TT entry
            int score = hash_entry->score;

            // retrieve score independent from the actual path
            // from root node (position) to current node (position)
            if (score < -mate_score)
                score += rb_ply;
            if (score > mate_score)
                score -= rb_ply;

            // match the exact (PV node) score
            if (hash_entry->flag == hash_flag_exact)
                // return exact (PV node) score
                return score;

            // match alpha (fail-low node) score
            if ((hash_entry->flag == hash_flag_alpha) &&
                (score <= alpha))
                // return alpha (fail-low node) score
                return alpha;

            // match beta (fail-high node) score
            if ((hash_entry->flag == hash_flag_beta) &&
                (score >= beta))
                // return beta (fail-high node) score
                return beta;
        }
    }

    // if hash entry doesn't exist
    return no_hash_entry;
}

// write hash entry data
static inline void lb_write_hash_entry(int score, int depth, int hash_flag)
{
    // create a TT instance pointer to particular hash entry storing
    // the scoring data for the current board position if available
    tt *hash_entry = &lb_hash_table[lb_hash_key % lb_hash_entries];

    // store score independent from the actual path
    // from root node (position) to current node (position)
    if (score < -mate_score)
        score -= lb_ply;
    if (score > mate_score)
        score += lb_ply;

    // write hash entry data
    hash_entry->hash_key = lb_hash_key;
    hash_entry->score = score;
    hash_entry->flag = hash_flag;
    hash_entry->depth = depth;
}

static inline void rb_write_hash_entry(int score, int depth, int hash_flag)
{
    // create a TT instance pointer to particular hash entry storing
    // the scoring data for the current board position if available
    tt *hash_entry = &rb_hash_table[rb_hash_key % rb_hash_entries];

    // store score independent from the actual path
    // from root node (position) to current node (position)
    if (score < -mate_score)
        score -= rb_ply;
    if (score > mate_score)
        score += rb_ply;

    // write hash entry data
    hash_entry->hash_key = rb_hash_key;
    hash_entry->score = score;
    hash_entry->flag = hash_flag;
    hash_entry->depth = depth;
}

// enable PV move scoring
static inline void lb_enable_pv_scoring(moves_t *move_list)
{
    // disable following PV
    lb_follow_pv = 0;

    // loop over the moves within a move list
    for (int count = 0; count < move_list->count; count++)
    {
        // make sure we hit PV move
        if (lb_pv_table[0][lb_ply] == move_list->moves[count])
        {
            // enable move scoring
            lb_score_pv = 1;

            // enable following PV
            lb_follow_pv = 1;
        }
    }
}

static inline void rb_enable_pv_scoring(moves_t *move_list)
{
    // disable following PV
    rb_follow_pv = 0;

    // loop over the moves within a move list
    for (int count = 0; count < move_list->count; count++)
    {
        // make sure we hit PV move
        if (rb_pv_table[0][rb_ply] == move_list->moves[count])
        {
            // enable move scoring
            rb_score_pv = 1;

            // enable following PV
            rb_follow_pv = 1;
        }
    }
}

/*  =======================
         Move ordering
    =======================

    1. PV move
    2. Captures in MVV/LVA
    3. 1st killer move
    4. 2nd killer move
    ///<ADD>
    5. Put move
    ///</ADD>
    6. History moves
    7. Unsorted moves
*/

// score moves
static inline int lb_score_move(int move)
{
    // if PV move scoring is allowed
    if (lb_score_pv)
    {
        // make sure we are dealing with PV move
        if (lb_pv_table[0][lb_ply] == move)
        {
            // disable score PV flag
            lb_score_pv = 0;

            // give PV move the highest score to search it first
            return 20000;
        }
    }

    // score capture move
    if (get_move_capture(move))
    {
        // init target piece
        int target_piece = P;

        // pick up bitboard piece index ranges depending on side
        int start_piece, end_piece;

        // pick up side to move
        if (lb_side == white)
        {
            start_piece = p;
            end_piece = k;
        }
        else
        {
            start_piece = P;
            end_piece = K;
        }

        // loop over lb_bitboards opposite to the current side to move
        for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++)
        {
            // if there's a piece on the target square
            if (get_bit(lb_bitboards[bb_piece], get_move_target(move)))
            {
                // remove it from corresponding bitboard
                target_piece = bb_piece;
                break;
            }
        }

        // score move by MVV LVA lookup [source piece][target piece]
        return lb_mvv_lva[get_move_piece(move)][target_piece] + 10000;
    }

    // score quiet move
    else
    {
        // score 1st killer move
        if (lb_killer_moves[0][lb_ply] == move)
            return 9000;

        // score 2nd killer move
        else if (lb_killer_moves[1][lb_ply] == move)
            return 8000;

        ///<ADD>    
        // score put move
        else if (get_move_source(move) == get_move_target(move))
            return lb_put_value[get_move_piece(move)] + 1000;
        ///</ADD>

        // score history move
        else
            return lb_history_moves[get_move_piece(move)][get_move_target(move)];
    }

    return 0;
}

static inline int rb_score_move(int move)
{
    // if PV move scoring is allowed
    if (rb_score_pv)
    {
        // make sure we are dealing with PV move
        if (rb_pv_table[0][rb_ply] == move)
        {
            // disable score PV flag
            rb_score_pv = 0;

            // give PV move the highest score to search it first
            return 20000;
        }
    }

    // score capture move
    if (get_move_capture(move))
    {
        // init target piece
        int target_piece = P;

        // pick up bitboard piece index ranges depending on side
        int start_piece, end_piece;

        // pick up side to move
        if (rb_side == white)
        {
            start_piece = p;
            end_piece = k;
        }
        else
        {
            start_piece = P;
            end_piece = K;
        }

        // loop over rb_bitboards opposite to the current side to move
        for (int bb_piece = start_piece; bb_piece <= end_piece; bb_piece++)
        {
            // if there's a piece on the target square
            if (get_bit(rb_bitboards[bb_piece], get_move_target(move)))
            {
                // remove it from corresponding bitboard
                target_piece = bb_piece;
                break;
            }
        }

        // score move by MVV LVA lookup [source piece][target piece]
        return rb_mvv_lva[get_move_piece(move)][target_piece] + 10000;
    }

    // score quiet move
    else
    {
        // score 1st killer move
        if (rb_killer_moves[0][rb_ply] == move)
            return 9000;

        // score 2nd killer move
        else if (rb_killer_moves[1][rb_ply] == move)
            return 8000;

        // score put move
        else if (get_move_source(move) == get_move_target(move))
            return rb_put_value[get_move_piece(move)] + 1000;

        // score history move
        else
            return rb_history_moves[get_move_piece(move)][get_move_target(move)];
    }

    return 0;
}

// sort moves in descending order
static inline int lb_sort_moves(moves_t *move_list)
{
    // move scores
    int move_scores[move_list->count];

    // score all the moves within a move list
    for (int count = 0; count < move_list->count; count++)
        // score move
        move_scores[count] = lb_score_move(move_list->moves[count]);

    // loop over current move within a move list
    for (int current_move = 0; current_move < move_list->count; current_move++)
    {
        // loop over next move within a move list
        for (int next_move = current_move + 1; next_move < move_list->count; next_move++)
        {
            // compare current and next move scores
            if (move_scores[current_move] < move_scores[next_move])
            {
                // swap scores
                int temp_score = move_scores[current_move];
                move_scores[current_move] = move_scores[next_move];
                move_scores[next_move] = temp_score;

                // swap moves
                int temp_move = move_list->moves[current_move];
                move_list->moves[current_move] = move_list->moves[next_move];
                move_list->moves[next_move] = temp_move;
            }
        }
    }
    return 0;
}

static inline int rb_sort_moves(moves_t *move_list)
{
    // move scores
    int move_scores[move_list->count];

    // score all the moves within a move list
    for (int count = 0; count < move_list->count; count++)
        // score move
        move_scores[count] = rb_score_move(move_list->moves[count]);

    // loop over current move within a move list
    for (int current_move = 0; current_move < move_list->count; current_move++)
    {
        // loop over next move within a move list
        for (int next_move = current_move + 1; next_move < move_list->count; next_move++)
        {
            // compare current and next move scores
            if (move_scores[current_move] < move_scores[next_move])
            {
                // swap scores
                int temp_score = move_scores[current_move];
                move_scores[current_move] = move_scores[next_move];
                move_scores[next_move] = temp_score;

                // swap moves
                int temp_move = move_list->moves[current_move];
                move_list->moves[current_move] = move_list->moves[next_move];
                move_list->moves[next_move] = temp_move;
            }
        }
    }
    return 0;
}

// print move scores
void lb_print_move_scores(moves_t *move_list)
{
#ifndef NDEBUG // print only in debug mode
    printf("\nlb\n     Move scores:\n\n");

    // loop over moves within a move list
    for (int count = 0; count < move_list->count; count++)
    {
        printf("    move: ");
        lb_print_move(move_list->moves[count]);
        printf("score: %d\n", lb_score_move(move_list->moves[count]));
    }
#endif
}

void rb_print_move_scores(moves_t *move_list)
{
#ifndef NDEBUG // print only in debug mode
    printf("\nrb\n     Move scores:\n\n");

    // loop over moves within a move list
    for (int count = 0; count < move_list->count; count++)
    {
        printf("     move: ");
        rb_print_move(move_list->moves[count]);
        printf("score: %d\n", rb_score_move(move_list->moves[count]));
    }
#endif
}

// position repetition detection
static inline int lb_is_repetition()
{
    // loop over repetition indicies range
    for (int index = 0; index < lb_repetition_index; index++)
        // if we found the hash key same with a current
        if (lb_repetition_table[index] == lb_hash_key)
            // we found a lb_repetition
            return 1;

    // if no repetition found
    return 0;
}

static inline int rb_is_repetition()
{
    // loop over repetition indicies range
    for (int index = 0; index < rb_repetition_index; index++)
        // if we found the hash key same with a current
        if (rb_repetition_table[index] == rb_hash_key)
            // we found a rb_repetition
            return 1;

    // if no repetition found
    return 0;
}

// quiescence search
static inline int lb_quiescence(int alpha, int beta)
{
    // every 2047 lb_nodes
    if ((lb_nodes & 2047) == 0)
        // "listen" to the GUI/user input
        lb_communicate();

    // increment nodes count
    lb_nodes++;

    // we are too deep, hence there's an overflow of arrays relying on max ply constant
    if (lb_ply > max_ply - 1)
        // evaluate position
        return lb_evaluate();

    // evaluate position
    int evaluation = lb_evaluate();

    // fail-hard beta cutoff
    if (evaluation >= beta)
    {
        // node (position) fails high
        return beta;
    }

    // found a better move
    if (evaluation > alpha)
    {
        // PV node (position)
        alpha = evaluation;
    }

    // create move list instance
    moves_t move_list[1];

    // generate moves
    lb_generate_moves(move_list);

    // sort moves
    lb_sort_moves(move_list);

    // loop over moves within a movelist
    for (int count = 0; count < move_list->count; count++)
    {
        // preserve board state
        lb_copy_board();

        // increment ply
        lb_ply++;

        // increment repetition index & store hash key
        ///<ADD>
        if (lb_repetition_index < 999)
        {
        ///</ADD>    
            lb_repetition_index++;
            lb_repetition_table[lb_repetition_index] = lb_hash_key;
        ///<ADD>
        }
        else
            PrintAssert(lb_repetition_index < 999);
        ///</ADD>

        // make sure to make only legal moves
        if (lb_make_move(move_list->moves[count], only_captures) == 0)
        {
            // decrement ply
            lb_ply--;

            // decrement repetition index
            lb_repetition_index--;

            // skip to next move
            continue;
        }

        // score current move
        int score = -lb_quiescence(-beta, -alpha);

        // decrement ply
        lb_ply--;

        // decrement repetition index
        lb_repetition_index--;

        // take move back
        lb_take_back();

        // reutrn 0 if time is up
        if (lb_stopped == 1)
            // free used memory
            return 0;
        
        // found a better move
        if (score > alpha)
        {
            // PV node (position)
            alpha = score;

            // fail-hard beta cutoff
            if (score >= beta)
                // node (position) fails high
                return beta;
        }
    }

    // node (position) fails low
    return alpha;
}

static inline int rb_quiescence(int alpha, int beta)
{
    // every 2047 rb_nodes
    if ((rb_nodes & 2047) == 0)
        // "listen" to the GUI/user input
        rb_communicate();

    // increment nodes count
    rb_nodes++;

    // we are too deep, hence there's an overflow of arrays relying on max ply constant
    if (rb_ply > max_ply - 1)
        // evaluate position
        return rb_evaluate();

    // evaluate position
    int evaluation = rb_evaluate();

    // fail-hard beta cutoff
    if (evaluation >= beta)
    {
        // node (position) fails high
        return beta;
    }

    // found a better move
    if (evaluation > alpha)
    {
        // PV node (position)
        alpha = evaluation;
    }

    // create move list instance
    moves_t move_list[1];

    // generate moves
    rb_generate_moves(move_list);

    // sort moves
    rb_sort_moves(move_list);

    // loop over moves within a movelist
    for (int count = 0; count < move_list->count; count++)
    {
        // preserve board state
        rb_copy_board();

        // increment ply
        rb_ply++;

        // increment repetition index & store hash key
        ///<ADD>
        if (rb_repetition_index < 999)
        {
        ///</ADD>    
            rb_repetition_index++;
            rb_repetition_table[rb_repetition_index] = rb_hash_key;
        ///<ADD>
        }
        else
            PrintAssert(rb_repetition_index < 999);
        ///</ADD>
        
        // make sure to make only legal moves
        if (rb_make_move(move_list->moves[count], only_captures) == 0)
        {
            // decrement ply
            rb_ply--;

            // decrement repetition index
            rb_repetition_index--;

            // skip to next move
            continue;
        }

        // score current move
        int score = -rb_quiescence(-beta, -alpha);

        // decrement ply
        rb_ply--;

        // decrement repetition index
        rb_repetition_index--;

        // take move back
        rb_take_back();

        // reutrn 0 if time is up
        if (rb_stopped == 1)
            return 0;
        
        // found a better move
        if (score > alpha)
        {
            // PV node (position)
            alpha = score;

            // fail-hard beta cutoff
            if (score >= beta)
                // node (position) fails high
                return beta;
        }
    }

    // node (position) fails low
    return alpha;
}

// full depth moves counter
const int lb_full_depth_moves = 4;
const int rb_full_depth_moves = 4;

// depth limit to consider reduction
const int lb_reduction_limit = 3;
const int rb_reduction_limit = 3;

// negamax alpha beta search
static inline int lb_negamax(int alpha, int beta, int depth)
{
    // init PV length
    lb_pv_length[lb_ply] = lb_ply;

    // variable to store current move's score (from the static evaluation perspective)
    int score;

    // define hash flag
    int hash_flag = hash_flag_alpha;

    // if position repetition occurs
    if ((lb_ply && lb_is_repetition()) || lb_fifty >= 100)
        // return draw score
        return 0;

    // a hack by Pedro Castro to figure out whether the current node is PV node or not
    int pv_node = beta - alpha > 1;

    // read hash entry if we're not in a root ply and hash entry is available
    // and current node is not a PV node
    if (lb_ply && (score = lb_read_hash_entry(alpha, beta, depth)) != no_hash_entry && pv_node == 0)
        // if the move has already been searched (hence has a value)
        // we just return the score for this move without searching it
        return score;

    // every 2047 nodes
    if ((lb_nodes & 2047) == 0)
        // "listen" to the GUI/user input
        lb_communicate();

    // recursion escapre condition
    if (depth == 0)
        // run quiescence search
        return lb_quiescence(alpha, beta);

    // we are too deep, hence there's an overflow of arrays relying on max ply constant
    if (lb_ply > max_ply - 1)
        // evaluate position
        return lb_evaluate();

    // increment nodes count
    lb_nodes++;

    // is king in check
    int in_check = lb_is_square_attacked((lb_side == white) ? lb_get_ls1b_index(lb_bitboards[K]) : lb_get_ls1b_index(lb_bitboards[k]),
                                         lb_side ^ 1);

    // increase search depth if the king has been exposed into a check
    if (in_check)
        depth++;

    // legal moves counter
    int legal_moves = 0;

    // null move pruning
    if (depth >= 3 && in_check == 0 && lb_ply)
    {
        // preserve board state
        lb_copy_board();

        // increment ply
        lb_ply++;

        // increment repetition index & store hash key
        ///<ADD>
        if (lb_repetition_index < 999)
        {
        ///</ADD>    
            lb_repetition_index++;
            lb_repetition_table[lb_repetition_index] = lb_hash_key;
        ///<ADD>
        }
        else
            PrintAssert(lb_repetition_index < 999);
        ///</ADD>
        
        // hash enpassant if available
        if (lb_enpassant != no_sq)
            lb_hash_key ^= lb_enpassant_keys[lb_enpassant];

        // reset enpassant capture square
        lb_enpassant = no_sq;

        // switch the side, literally giving opponent an extra move to make
        lb_side ^= 1;

        // hash the side
        lb_hash_key ^= lb_side_key;

        /* search moves with reduced depth to find beta cutoffs
           depth - 1 - R where R is a reduction limit */
        score = -lb_negamax(-beta, -beta + 1, depth - 1 - 2);

        // decrement ply
        lb_ply--;

        // decrement repetition index
        lb_repetition_index--;

        // restore board state
        lb_take_back();

        // reutrn 0 if time is up
        if (lb_stopped == 1)
            return 0;

        // fail-hard beta cutoff
        if (score >= beta)
            // node (position) fails high
            return beta;
    }

    // create move list instance
    moves_t move_list[1];
    
    // generate moves
    lb_generate_moves(move_list);

    // if we are now following PV line
    if (lb_follow_pv)
        // enable PV move scoring
        lb_enable_pv_scoring(move_list);

    // sort moves
    lb_sort_moves(move_list);

    // number of moves searched in a move list
    int moves_searched = 0;

    // loop over moves within a movelist
    for (int count = 0; count < move_list->count; count++)
    {
        // preserve board state
        lb_copy_board();

        // increment ply
        lb_ply++;

        // increment repetition index & store hash key
        ///<ADD>
        if (lb_repetition_index < 999)
        {
        ///</ADD>    
            lb_repetition_index++;
            lb_repetition_table[lb_repetition_index] = lb_hash_key;
        ///<ADD>
        }
        else
            PrintAssert(lb_repetition_index < 999);
        ///</ADD>

        // make sure to make only legal moves
        if (lb_make_move(move_list->moves[count], all_moves) == 0)
        {
            // decrement ply
            lb_ply--;

            // decrement repetition index
            lb_repetition_index--;

            // skip to next move
            continue;
        }

        // increment legal moves
        legal_moves++;

        // full depth search
        if (moves_searched == 0)
            // do normal alpha beta search
            score = -lb_negamax(-beta, -alpha, depth - 1);

        // late move reduction (LMR)
        else
        {
            // condition to consider LMR
            if (
                moves_searched >= lb_full_depth_moves &&
                depth >= lb_reduction_limit &&
                in_check == 0 &&
                get_move_capture(move_list->moves[count]) == 0 &&
                get_move_promoted(move_list->moves[count]) == 0)
                // search current move with reduced depth:
                score = -lb_negamax(-alpha - 1, -alpha, depth - 2);

            // hack to ensure that full-depth search is done
            else
                score = alpha + 1;

            // principle variation search PVS
            if (score > alpha)
            {
                /* Once you've found a move with a score that is between alpha and beta,
                   the rest of the moves are searched with the goal of proving that they are all bad.
                   It's possible to do this a bit faster than a search that worries that one
                   of the remaining moves might be good. */
                score = -lb_negamax(-alpha - 1, -alpha, depth - 1);

                /* If the algorithm finds out that it was wrong, and that one of the
                   subsequent moves was better than the first PV move, it has to search again,
                   in the normal alpha-beta manner.  This happens sometimes, and it's a waste of time,
                   but generally not often enough to counteract the savings gained from doing the
                   "bad move proof" search referred to earlier. */
                if ((score > alpha) && (score < beta))
                    /* re-search the move that has failed to be proved to be bad
                       with normal alpha beta score bounds*/
                    score = -lb_negamax(-beta, -alpha, depth - 1);
            }
        }

        // decrement ply
        lb_ply--;

        // decrement repetition index
        lb_repetition_index--;

        // take move back
        lb_take_back();

        // reutrn 0 if time is up
        if (lb_stopped == 1)
            return 0;

        // increment the counter of moves searched so far
        moves_searched++;

        // found a better move
        if (score > alpha)
        {
            // switch hash flag from storing score for fail-low node
            // to the one storing score for PV node
            hash_flag = hash_flag_exact;

            // on quiet moves
            if (get_move_capture(move_list->moves[count]) == 0)
                // store history moves
                lb_history_moves[get_move_piece(move_list->moves[count])][get_move_target(move_list->moves[count])] += depth;

            // PV node (position)
            alpha = score;

            // write PV move
            lb_pv_table[lb_ply][lb_ply] = move_list->moves[count];

            // loop over the next ply
            for (int next_ply = lb_ply + 1; next_ply < lb_pv_length[lb_ply + 1]; next_ply++)
                // copy move from deeper ply into a current ply's line
                lb_pv_table[lb_ply][next_ply] = lb_pv_table[lb_ply + 1][next_ply];

            // adjust PV length
            lb_pv_length[lb_ply] = lb_pv_length[lb_ply + 1];

            // fail-hard beta cutoff
            if (score >= beta)
            {
                // store hash entry with the score equal to beta
                lb_write_hash_entry(beta, depth, hash_flag_beta);

                // on quiet moves
                if (get_move_capture(move_list->moves[count]) == 0)
                {
                    // store killer moves
                    lb_killer_moves[1][lb_ply] = lb_killer_moves[0][lb_ply];
                    lb_killer_moves[0][lb_ply] = move_list->moves[count];
                }

                // node (position) fails high
                return beta;
            }
        }
    }

    // we don't have any legal moves to make in the current postion
    if (legal_moves == 0)
    {
        // king is in check
        if (in_check)
            // return mating score (assuming closest distance to mating position)
            return -mate_value + lb_ply;
        // king is not in check
        // return stalemate score
        return 0;
    }

    // store hash entry with the score equal to alpha
    lb_write_hash_entry(alpha, depth, hash_flag);

    // node (position) fails low
    return alpha;
}

static inline int rb_negamax(int alpha, int beta, int depth)
{
    // init PV length
    rb_pv_length[rb_ply] = rb_ply;

    // variable to store current move's score (from the static evaluation perspective)
    int score;

    // define hash flag
    int hash_flag = hash_flag_alpha;

    // if position repetition occurs
    if ((rb_ply && rb_is_repetition()) || rb_fifty >= 100)
        // return draw score
        return 0;

    // a hack by Pedro Castro to figure out whether the current node is PV node or not
    int pv_node = beta - alpha > 1;

    // read hash entry if we're not in a root ply and hash entry is available
    // and current node is not a PV node
    if (rb_ply && (score = rb_read_hash_entry(alpha, beta, depth)) != no_hash_entry && pv_node == 0)
        // if the move has already been searched (hence has a value)
        // we just return the score for this move without searching it
        return score;

    // every 2047 nodes
    if ((rb_nodes & 2047) == 0)
        // "listen" to the GUI/user input
        rb_communicate();

    // recursion escapre condition
    if (depth == 0)
        // run quiescence search
        return rb_quiescence(alpha, beta);

    // we are too deep, hence there's an overflow of arrays relying on max ply constant
    if (rb_ply > max_ply - 1)
        // evaluate position
        return rb_evaluate();

    // increment nodes count
    rb_nodes++;

    // is king in check
    int in_check = rb_is_square_attacked((rb_side == white) ? rb_get_ls1b_index(rb_bitboards[K]) : rb_get_ls1b_index(rb_bitboards[k]),
                                         rb_side ^ 1);

    // increase search depth if the king has been exposed into a check
    if (in_check)
        depth++;

    // legal moves counter
    int legal_moves = 0;

    // null move pruning
    if (depth >= 3 && in_check == 0 && rb_ply)
    {
        // preserve board state
        rb_copy_board();

        // increment ply
        rb_ply++;

        // increment repetition index & store hash key
        ///<ADD>
        if (rb_repetition_index < 999)
        {
        ///</ADD>    
            rb_repetition_index++;
            rb_repetition_table[rb_repetition_index] = rb_hash_key;
        ///<ADD>
        }
        else
            PrintAssert(rb_repetition_index < 999);
        ///</ADD>

        // hash enpassant if available
        if (rb_enpassant != no_sq)
            rb_hash_key ^= rb_enpassant_keys[rb_enpassant];

        // reset enpassant capture square
        rb_enpassant = no_sq;

        // switch the side, literally giving opponent an extra move to make
        rb_side ^= 1;

        // hash the side
        rb_hash_key ^= rb_side_key;

        /* search moves with reduced depth to find beta cutoffs
           depth - 1 - R where R is a reduction limit */
        score = -rb_negamax(-beta, -beta + 1, depth - 1 - 2);

        // decrement ply
        rb_ply--;

        // decrement repetition index
        rb_repetition_index--;

        // restore board state
        rb_take_back();

        // reutrn 0 if time is up
        if (rb_stopped == 1)
            return 0;

        // fail-hard beta cutoff
        if (score >= beta)
            // node (position) fails high
            return beta;
    }

    // create move list instance
    moves_t move_list[1];
    
    // generate moves
    rb_generate_moves(move_list);

    // if we are now following PV line
    if (rb_follow_pv)
        // enable PV move scoring
        rb_enable_pv_scoring(move_list);

    // sort moves
    rb_sort_moves(move_list);

    // number of moves searched in a move list
    int moves_searched = 0;

    // loop over moves within a movelist
    for (int count = 0; count < move_list->count; count++)
    {
        // preserve board state
        rb_copy_board();

        // increment ply
        rb_ply++;

        // increment repetition index & store hash key
        ///<ADD>
        if (rb_repetition_index < 999)
        {
        ///</ADD>    
            rb_repetition_index++;
            rb_repetition_table[rb_repetition_index] = rb_hash_key;
        ///<ADD>
        }
        else
            PrintAssert(rb_repetition_index < 999);
        ///</ADD>

        // make sure to make only legal moves
        if (rb_make_move(move_list->moves[count], all_moves) == 0)
        {
            // decrement ply
            rb_ply--;

            // decrement repetition index
            rb_repetition_index--;

            // skip to next move
            continue;
        }

        // increment legal moves
        legal_moves++;

        // full depth search
        if (moves_searched == 0)
            // do normal alpha beta search
            score = -rb_negamax(-beta, -alpha, depth - 1);

        // late move reduction (LMR)
        else
        {
            // condition to consider LMR
            if (
                moves_searched >= rb_full_depth_moves &&
                depth >= rb_reduction_limit &&
                in_check == 0 &&
                get_move_capture(move_list->moves[count]) == 0 &&
                get_move_promoted(move_list->moves[count]) == 0)
                // search current move with reduced depth:
                score = -rb_negamax(-alpha - 1, -alpha, depth - 2);

            // hack to ensure that full-depth search is done
            else
                score = alpha + 1;

            // principle variation search PVS
            if (score > alpha)
            {
                /* Once you've found a move with a score that is between alpha and beta,
                   the rest of the moves are searched with the goal of proving that they are all bad.
                   It's possible to do this a bit faster than a search that worries that one
                   of the remaining moves might be good. */
                score = -rb_negamax(-alpha - 1, -alpha, depth - 1);

                /* If the algorithm finds out that it was wrong, and that one of the
                   subsequent moves was better than the first PV move, it has to search again,
                   in the normal alpha-beta manner.  This happens sometimes, and it's a waste of time,
                   but generally not often enough to counteract the savings gained from doing the
                   "bad move proof" search referred to earlier. */
                if ((score > alpha) && (score < beta))
                    /* re-search the move that has failed to be proved to be bad
                       with normal alpha beta score bounds*/
                    score = -rb_negamax(-beta, -alpha, depth - 1);
            }
        }

        // decrement ply
        rb_ply--;

        // decrement repetition index
        rb_repetition_index--;

        // take move back
        rb_take_back();

        // reutrn 0 if time is up
        if (rb_stopped == 1)
            return 0;

        // increment the counter of moves searched so far
        moves_searched++;

        // found a better move
        if (score > alpha)
        {
            // switch hash flag from storing score for fail-low node
            // to the one storing score for PV node
            hash_flag = hash_flag_exact;

            // on quiet moves
            if (get_move_capture(move_list->moves[count]) == 0)
                // store history moves
                rb_history_moves[get_move_piece(move_list->moves[count])][get_move_target(move_list->moves[count])] += depth;

            // PV node (position)
            alpha = score;

            // write PV move
            rb_pv_table[rb_ply][rb_ply] = move_list->moves[count];

            // loop over the next ply
            for (int next_ply = rb_ply + 1; next_ply < rb_pv_length[rb_ply + 1]; next_ply++)
                // copy move from deeper ply into a current ply's line
                rb_pv_table[rb_ply][next_ply] = rb_pv_table[rb_ply + 1][next_ply];

            // adjust PV length
            rb_pv_length[rb_ply] = rb_pv_length[rb_ply + 1];

            // fail-hard beta cutoff
            if (score >= beta)
            {
                // store hash entry with the score equal to beta
                rb_write_hash_entry(beta, depth, hash_flag_beta);

                // on quiet moves
                if (get_move_capture(move_list->moves[count]) == 0)
                {
                    // store killer moves
                    rb_killer_moves[1][rb_ply] = rb_killer_moves[0][rb_ply];
                    rb_killer_moves[0][rb_ply] = move_list->moves[count];
                }

                // node (position) fails high
                return beta;
            }
        }
    }

    // we don't have any legal moves to make in the current postion
    if (legal_moves == 0)
    {
        // king is in check
        if (in_check)
            // return mating score (assuming closest distance to mating position)
            return -mate_value + rb_ply;
        
        // king is not in check
        // return stalemate score
        return 0;
    }

    // store hash entry with the score equal to alpha
    rb_write_hash_entry(alpha, depth, hash_flag);

    // node (position) fails low
    return alpha;
}

// search position for the best move
void lb_search_position(int depth)
{
#ifndef NDEBUG // print only in debug mode
    // print offset
    printf("\nlb\n");
#endif

    // search start time
    int start = lb_get_time_ms();

    // define best score variable
    int score = 0;

    // reset nodes counter
    lb_nodes = 0;

    // reset "time is up" flag
    lb_stopped = 0;

    // reset follow PV flags
    lb_follow_pv = 0;
    lb_score_pv = 0;

    // clear helper data structures for search
    memset(lb_killer_moves, 0, sizeof(lb_killer_moves));
    memset(lb_history_moves, 0, sizeof(lb_history_moves));
    memset(lb_pv_table, 0, sizeof(lb_pv_table));
    memset(lb_pv_length, 0, sizeof(lb_pv_length));

    // define initial alpha beta bounds
    int alpha = -infinity;
    int beta = infinity;

    // iterative deepening
    for (int current_depth = 1; current_depth <= depth; current_depth++)
    {
        // if time is up
        if (lb_stopped == 1)
            // stop calculating and return best move so far
            break;

        // enable follow PV flag
        lb_follow_pv = 1;

        // find best move within a given position
        score = lb_negamax(alpha, beta, current_depth);

        // we fell outside the window, so try again with a full-width window (and the same depth)
        if ((score <= alpha) || (score >= beta))
        {
            alpha = -infinity;
            beta = infinity;
            continue;
        }

        // set up the window for the next iteration
        alpha = score - 50;
        beta = score + 50;

        // if PV is available
        if (lb_pv_length[0])
        {
#ifndef NDEBUG // print only in debug mode
            // print search info
            if (score > -mate_value && score < -mate_score)
                printf("info score mate %d depth %d nodes %lld time %d pv ", -(score + mate_value) / 2 - 1, current_depth, lb_nodes, lb_get_time_ms() - start);

            else if (score > mate_score && score < mate_value)
                printf("info score mate %d depth %d nodes %lld time %d pv ", (mate_value - score) / 2 + 1, current_depth, lb_nodes, lb_get_time_ms() - start);

            else
                printf("info score cp %d depth %d nodes %lld time %d pv ", score, current_depth, lb_nodes, lb_get_time_ms() - start);
#endif
            // loop over the moves within a PV line
            for (int count = 0; count < lb_pv_length[0]; count++)
            {
                // print PV move
                lb_print_move(lb_pv_table[0][count]);
#ifndef NDEBUG // print only in debug mode
                printf(" ");
#endif
            }

#ifndef NDEBUG // print only in debug mode
            // print new line
            printf("\n");
#endif
        }
    }

#ifndef NDEBUG // print only in debug mode
    // print best move
    printf("bestmove ");
    lb_print_move(lb_pv_table[0][0]);
    printf("\n");
#endif
}

void rb_search_position(int depth)
{
#ifndef NDEBUG // print only in debug mode
    // print offset
    printf("\nlb\n");
#endif

    // search start time
    int start = rb_get_time_ms();

    // define best score variable
    int score = 0;

    // reset nodes counter
    rb_nodes = 0;

    // reset "time is up" flag
    rb_stopped = 0;

    // reset follow PV flags
    rb_follow_pv = 0;
    rb_score_pv = 0;

    // clear helper data structures for search
    memset(rb_killer_moves, 0, sizeof(rb_killer_moves));
    memset(rb_history_moves, 0, sizeof(rb_history_moves));
    memset(rb_pv_table, 0, sizeof(rb_pv_table));
    memset(rb_pv_length, 0, sizeof(rb_pv_length));

    // define initial alpha beta bounds
    int alpha = -infinity;
    int beta = infinity;

    // iterative deepening
    for (int current_depth = 1; current_depth <= depth; current_depth++)
    {
        // if time is up
        if (rb_stopped == 1)
            // stop calculating and return best move so far
            break;

        // enable follow PV flag
        rb_follow_pv = 1;

        // find best move within a given position
        score = rb_negamax(alpha, beta, current_depth);

        // we fell outside the window, so try again with a full-width window (and the same depth)
        if ((score <= alpha) || (score >= beta))
        {
            alpha = -infinity;
            beta = infinity;
            continue;
        }

        // set up the window for the next iteration
        alpha = score - 50;
        beta = score + 50;

        // if PV is available
        if (rb_pv_length[0])
        {
#ifndef NDEBUG // print only in debug mode
            // print search info
            if (score > -mate_value && score < -mate_score)
                printf("info score mate %d depth %d nodes %lld time %d pv ", -(score + mate_value) / 2 - 1, current_depth, rb_nodes, rb_get_time_ms() - start);

            else if (score > mate_score && score < mate_value)
                printf("info score mate %d depth %d nodes %lld time %d pv ", (mate_value - score) / 2 + 1, current_depth, rb_nodes, rb_get_time_ms() - start);

            else
                printf("info score cp %d depth %d nodes %lld time %d pv ", score, current_depth, rb_nodes, rb_get_time_ms() - start);
#endif

            // loop over the moves within a PV line
            for (int count = 0; count < rb_pv_length[0]; count++)
            {
                // print PV move
                rb_print_move(rb_pv_table[0][count]);
#ifndef NDEBUG // print only in debug mode
                printf(" ");
#endif
            }

            // print new line
#ifndef NDEBUG // print only in debug mode
            printf("\n");
#endif
        }
    }

#ifndef NDEBUG // print only in debug mode
    // print best move
    printf("bestmove ");
    rb_print_move(rb_pv_table[0][0]);
    printf("\n");
#endif
}

// reset time control variables
void lb_reset_time_control()
{
    // reset timing
    lb_quit = 0;
    lb_movestogo = 30;
    lb_movetime = -1;
    lb_ucitime = -1;
    lb_inc = 0;
    lb_starttime = 0;
    lb_stoptime = 0;
    lb_timeset = 0;
    lb_stopped = 0;
}

void rb_reset_time_control()
{
    // reset timing
    rb_quit = 0;
    rb_movestogo = 30;
    rb_movetime = -1;
    rb_ucitime = -1;
    rb_inc = 0;
    rb_starttime = 0;
    rb_stoptime = 0;
    rb_timeset = 0;
    rb_stopped = 0;
}

/**********************************\
 ==================================
 
              Init all
 
 ==================================
\**********************************/

// init all variables
void lb_init_all()
{
    // init leaper pieces attacks
    lb_init_leapers_attacks();

    // init slider pieces attacks
    lb_init_sliders_attacks(bishop);
    lb_init_sliders_attacks(rook);

    // init random keys for hashing purposes
    lb_init_random_keys();

    // init evaluation masks
    lb_init_evaluation_masks();

    // init hash table with default 128 MB
    lb_init_hash_table(128);
}

void rb_init_all()
{
    // init leaper pieces attacks
    rb_init_leapers_attacks();

    // init slider pieces attacks
    rb_init_sliders_attacks(bishop);
    rb_init_sliders_attacks(rook);

    // init random keys for hashing purposes
    rb_init_random_keys();

    // init evaluation masks
    rb_init_evaluation_masks();

    // init hash table with default 128 MB
    rb_init_hash_table(128);
}

// -----------------------------------------------------------------------
// End BBC code
// -----------------------------------------------------------------------

// -----------------------------------------------------------------------
// Gui code @author pv
// -----------------------------------------------------------------------

// gamestate gui
enum
{
    Input,
    StartGame,
    PlayGame,
    StopGame,
};

// boardtypes gui
enum
{
    left,
    right,
};

// eGameOverState gui
enum
{
    none = 0,
    wMate = 1,    // white wins, black is mated
    wTime = 2,    // white wins, black is out of time
    bMate = 3,    // black wins, white is mated
    bTime = 4,    // black wins, white is out of time
    drawMat = 5,  // draw by material
    wDrawPat = 6, // draw by white king is pat
    bDrawPat = 7, // draw by black king is pat
    DrawRep = 8,  // draw by repeat moves rule
    Draw50 = 9,   // draw by 50 moves rule
};

// -----------------------------------------------------------------------
// Gui variables
// -----------------------------------------------------------------------

// input
int game_player_time = 0;
int game_player_plus_time = 0;
int game_color = 0;
int game_plus = 0;
int game_time = 0;
int game_state = Input;

// draw sizes
const int SQUARE_SIZE = 72;
const int HALF_SQUARE_SIZE = 36;
const int BOARD_SIZE = 576;
const int SCREEN_WIDTH = 1440;
const int SCREEN_HEIGHT = 864;
const int LARGE_PIECE_SIZE = 64;
const int SMALL_PIECE_SIZE = 32;

// pictures
Texture2D table;
Texture2D board;
Texture2D large_pieces[12];
Texture2D small_pieces[12];
Texture2D choice;
Texture2D enterbtn;
Texture2D plusminbtn;
Texture2D chessclock;
Texture2D ai_image;
Texture2D human_image;

// title
const char *title = "ChessPassTrough in Raylib-C (C)2025 Peter Veenendaal; versie: 0.90";

// name of the image pictures
const char *pieces[12] = {
    "./assets/PawnW.png",
    "./assets/KnightW.png",
    "./assets/BishopW.png",
    "./assets/RookW.png",
    "./assets/QueenW.png",
    "./assets/KingW.png",
    "./assets/PawnB.png",
    "./assets/KnightB.png",
    "./assets/BishopB.png",
    "./assets/RookB.png",
    "./assets/QueenB.png",
    "./assets/KingB.png",
};

// text to print when the game is finished
const char *text_game_end[10] = {
    "",
    "wit wint, zwart staat schaakmat",
    "wit wint, zwart heeft geen bedenktijd meer",
    "zwart wint, wit staat schaakmat",
    "zwart wint, wit heeft geen bedenktijd meer",
    "het is remise door materiaal",
    "wit staat pat, het is remise",
    "zwart staat pat, het is remise",
    "het is remise door 3 zetten regel",
    "het is remise door 50 zetten regel"};

// promoted pieces to draw
const int promote_pieces[2][4] = {
    Q,
    R,
    B,
    N,
    q,
    r,
    b,
    n,
};

// rules of the game
const char *rules[26] = {
    "   REGELS DOORGEEF SCHAAK:",
    "   ------------------------------",
    "1. Een speler mag een doorgegeven stuk op het bord zetten in plaats van een gewone zet (het verplaatsen van een stuk).",
    "2. Met het plaatsen mag niet worden geslagen en geen schaak worden gegeven.",
    "3. Een pion mag niet op een promotieveld of op de eigen onderste rij worden geplaatst.",
    "4. Een stuk dat is ontstaan door de promotie van een pion wordt bij het doorgeven weer een pion.",
    "5. De doorgegeven schaakstukken moeten zichtbaar zijn voor de tegenstanders.",
    "6. Pat of remise wegens materiaaltekort is pas definitief als de andere partij afgelopen is.",
    "7. Bij tijdsoverschrijding is de partij remise als de tegenstander de partij niet reglementair kan winnen, ook niet door het",
    "   inzetten van stukken die diens teamgenoot geslagen heeft of nog kan slaan.",
    "8. De wedstrijd eindigt als beide partijen zijn afgelopen, de uitslag is, net als bij andere teamwedstrijden, de optelsom",
    "   van de afzonderlijke partijen, dus 2-0, 1,5-0,5 of 1-1.",
    "",
    "   WERKING PROGRAMA:",
    "   ----------------------",
    "1. Je speelt van onder naar boven dus met wit op het rechter of met zwart op het linker bord.",
    "2. De a.i zal voor de overige spelers de zetten doen.",
    "3. Geslagen en doorgegeven stukken of pionnen zijn zichtbaar boven of onder het bord.",
    "4. Zodra je aan zet bent kun je op een stuk of pion op het bord of eventueel op een doorgegeven stuk of pion klikken met de",
    "   muis, het programma zal dan de valide velden tonen als deze er zijn, zodra je op een valide veld klikt wordt deze zet",
    "   uitgevoerd en gaat de a.i zijn volgende zet berekenen.",
    "5. Hieronder worden de standaard instellingen getoond deze kunnen gewijzigd worden door op de buttons te klikken.",
    "6. Door op de enter button/key te klikken wordt het spel begonnen, de borden worden getoond en de klokken gestart.",
    "",
    "   STANDAARD INSTELLINGEN:",
    "   ------------------------------",
};

// input/output text and varibles
const char *colorstr[2] = {"Speler kleur : Wit", "Speler kleur : Zwart"};
const int colorint[2] = {white, black};
const char *timestr[3] = {
    "Speeltijd : 5 min",
    "Speeltijd : 10 min",
    "Speeltijd : 15 min",
};
const int timeint[3] = {300, 600, 900};
const char *plusstr[4] = {
    "Plus tijd per zet : 0 sec",
    "Plus tijd per zet : 2 sec",
    "Plus tijd per zet : 3 sec",
    "Plus tijd per zet : 5 sec",
};
const int plusint[4] = {0, 2, 3, 5};
const char *x_co[8] = {"a", "b", "c", "d", "e", "f", "g", "h"};
const char *y_co[8] = {"8", "7", "6", "5", "4", "3", "2", "1"};
const char *number[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

// -----------------------------------------------------------------------
// boards
// left board, prefix lb_
// right board, prefix rb_
// -----------------------------------------------------------------------

// variables

// start column board
int lb_brd_col = SQUARE_SIZE;
int rb_brd_col = BOARD_SIZE + SQUARE_SIZE * 3;

// start row board
int lb_brd_row = SQUARE_SIZE * 2;
int rb_brd_row = SQUARE_SIZE * 2;

// used for printing the board in the gui
int lb_drawboard[64];             
int rb_drawboard[64];

// used for printing the captured pieces in the gui
int lb_drawcappieces[11]; 
int rb_drawcappieces[11];         

// square from
int lb_selectpiece = -1;
int rb_selectpiece = -1;

// square to
int lb_selectsquare = -1;
int rb_selectsquare = -1;  

// select cap piece
int lb_capselectpiece = -1;
int rb_capselectpiece = -1;    

// promotion flag
int lb_promotion = 0;
int rb_promotion = 0;  

// promotion choice
int lb_promotionmove = -1;
int rb_promotionmove = -1;       

// current game state
int lb_gamestate = StartGame;
int rb_gamestate = StartGame;  

// reason the gam ends
int lb_game_end = 0;           
int rb_game_end = 0;   

// list of moves used in the gui
moves_t lb_gui_move_list[1];
moves_t rb_gui_move_list[1];    

// bits are set when a piece on the square can move
U64 lb_move_options[1];     
U64 rb_move_options[1];       

// bits are set when the piece move to the square
U64 lb_piece_options[64];
U64 rb_piece_options[64];             

// bits are set when the cap_piece can be put on the board
U64 lb_cap_piece_options[11];     
U64 rb_cap_piece_options[11];    

// color from the ai player
int lb_ai_player;
int rb_ai_player;                

// color from the human player
int lb_human_player;   
int rb_human_player;       

// for counting the moves executed
moves_t lb_game_moves[1]; 
moves_t rb_game_moves[1];        

// variables for the chessclock

// flag if the clock is pressed
int lb_press_clock;     
int rb_press_clock; 

// to handle the used time in seconds
int lb_timer[2];
int rb_timer[2];

// add time after each move in seconds 
int lb_plustimer[2];
int rb_plustimer[2];    

// for drawing the time on the clock
int lb_clocktime[2][5];
int rb_clocktime[2][5]; 

// in ms
int lb_starttimer[2];
int rb_starttimer[2];  

// in ms
int lb_thinktimer[2];
int rb_thinktimer[2]; 

// preventing flashing on the clocks during thinking of a move by the AI
int lb_side2move;     
int rb_side2move;       

// pass through pieces in the gui, preventing memory locks
int lb_gui_pt_pieces[64];
int rb_gui_pt_pieces[64];

// indexers for gui pt pieces
int lb_gui_pt_pieces_count;
int rb_gui_pt_pieces_count;

// variables for multi threading

// thread to run
pthread_t lb_thread;
pthread_t rb_thread;

// flag that indicates if the thread is ready
static int lb_task_ready;
static int rb_task_ready;

// check for draw by repetition

// hashkey after every move
U64 lb_game_hashkey[1000];
U64 rb_game_hashkey[1000];

// index of the array
int lb_game_hashkey_index; 
int rb_game_hashkey_index;  

// methods

// Fill the time for on the chessclock
void fill_clocktime(int side, int col)
{
    if (!side)
    {
        int hr = (int)(lb_timer[col] / 3600);
        int rst = lb_timer[col] - hr * 3600;
        int sec = rst % 60;
        int min = (int)((rst - sec) / 60);
        lb_clocktime[col][4] = sec % 10;
        lb_clocktime[col][3] = (int)((sec - lb_clocktime[col][4]) / 10);
        lb_clocktime[col][2] = min % 10;
        lb_clocktime[col][1] = (int)((min - lb_clocktime[col][2]) / 10);
        lb_clocktime[col][0] = hr;
    }
    else
    {
        int hr = (int)(rb_timer[col] / 3600);
        int rst = rb_timer[col] - hr * 3600;
        int sec = rst % 60;
        int min = (int)((rst - sec) / 60);
        rb_clocktime[col][4] = sec % 10;
        rb_clocktime[col][3] = (int)((sec - rb_clocktime[col][4]) / 10);
        rb_clocktime[col][2] = min % 10;
        rb_clocktime[col][1] = (int)((min - rb_clocktime[col][2]) / 10);
        rb_clocktime[col][0] = hr;
    }
}

// fill the options left board only used for the human player
void lb_fillOptions(moves_t *move_list, U64 *move_options, U64 *piece_options, U64 *cap_piece_options)
{
    // reset move options
    memset(move_options, 0ULL, 8);

    // reset piece options
    memset(piece_options, 0ULL, 512);

    // reset cap piece_options
    memset(cap_piece_options, 0ULL, 88);

    // generate moves
    lb_generate_moves(move_list);

    for (int index = 0; index < move_list->count; ++index)
    {
        // preserve the board
        lb_copy_board();

        // get move data
        int move = move_list->moves[index];
        int sqf = get_move_source(move);
        int sqt = get_move_target(move);
        int piece = get_move_piece(move);

        // test the move
        if (lb_make_move(move, all_moves))
        {
            if (sqf == sqt)
                set_bit(cap_piece_options[piece], sqt);
            else
            {
                set_bit(move_options[0], sqf);
                set_bit(piece_options[sqf], sqt);
            }
        }

        // restore the board
        lb_take_back();
    }
}

void rb_fillOptions(moves_t *move_list, U64 *move_options, U64 *piece_options, U64 *cap_piece_options)
{
    // reset move options
    memset(move_options, 0ULL, 8);

    // reset piece options
    memset(piece_options, 0ULL, 512);

    // reset piece options
    memset(cap_piece_options, 0ULL, 88);

    // generate moves
    rb_generate_moves(move_list);

    for (int index = 0; index < move_list->count; ++index)
    {
        // preserve the board
        rb_copy_board();

        // get move data
        int move = move_list->moves[index];
        int sqf = get_move_source(move);
        int sqt = get_move_target(move);
        int piece = get_move_piece(move);

        // test the move
        if (rb_make_move(move, all_moves))
        {
            if (sqf == sqt)
                set_bit(cap_piece_options[piece], sqt);
            else
            {
                set_bit(move_options[0], sqf);
                set_bit(piece_options[sqf], sqt);
            }
        }

        // restore the board
        rb_take_back();
    }
}

// Get piece on the square left board for drawing
int lb_getPiece(int square)
{
    if (get_bit(lb_occupancies[both], square))
    {
        for (int bb_piece = P; bb_piece <= k; ++bb_piece)
        {
            if (get_bit(lb_bitboards[bb_piece], square))
            {
                return bb_piece;
            }
        }
    }
    return no_sq;
}

int rb_getPiece(int square)
{
    if (get_bit(rb_occupancies[both], square))
    {
        for (int bb_piece = P; bb_piece <= k; ++bb_piece)
        {
            if (get_bit(rb_bitboards[bb_piece], square))
            {
                return bb_piece;
            }
        }
    }
    return no_sq;
}

// get the captured piecs for drawing captured pieces
void lb_getCapturedPieces()
{
    for (int i = 0; i < 11; ++i)
        lb_drawcappieces[i] = 0;
    if (lb_cap_pieces_count > 0)
    {
        for (int i = 0; i < lb_cap_pieces_count; ++i)
        {
            int piece = lb_cap_pieces[i] - 100;
            ++lb_drawcappieces[piece];
        }
    }
}

void rb_getCapturedPieces()
{
    for (int i = 0; i < 11; ++i)
        rb_drawcappieces[i] = 0;
    if (rb_cap_pieces_count > 0)
    {
        for (int i = 0; i < rb_cap_pieces_count; ++i)
        {
            int piece = rb_cap_pieces[i] - 100;
            ++rb_drawcappieces[piece];
        }
    }
}

// make the move from the human player
int lb_doMove(moves_t *move_list, int sqf, int sqt, int promotionpiece)
{
    int move = -1;
    for (int index = 0; index < move_list->count; ++index)
    {
        // add move data
        move = move_list->moves[index];
        int sqrf = get_move_source(move);
        int sqrt = get_move_target(move);
        int pp = get_move_promoted(move);

        // move = promotion ?
        if (promotionpiece > -1)
        {
            if (sqrf == sqf && sqrt == sqt && promotionpiece == pp)
            {
                break;
            }
        }
        // move is not promotion
        else
        {
            if (sqrf == sqf && sqrt == sqt)
            {
                break;
            }
        }
    }

    // no move found
    if (move == -1)
        return 0;

    // test move
    if (!lb_make_move(move, all_moves))
    {
        return 0;
    }

    // move is valid
    lb_add_move(lb_game_moves, move);
    return 1;
}

// make the put move from the human player
int lb_doPutMove(moves_t *move_list, int sqt, int putpiece)
{
    int move = -1;
    for (int index = 0; index < move_list->count; ++index)
    {
        // add move data
        move = move_list->moves[index];
        int sqrf = get_move_source(move);
        int sqrt = get_move_target(move);
        int pp = get_move_piece(move);
        
        if (sqrf == sqt && sqrt == sqt && pp == putpiece )
        {
            break;
        }
    }

    // no move found
    if (move == -1)
        return 0;

    // test move
    if (!lb_make_move(move, all_moves))
    {
        return 0;
    }
    
    // move is valid
    lb_add_move(lb_game_moves, move);
    return 1;
}

int rb_doMove(moves_t *move_list, int sqf, int sqt, int promotionpiece)
{
    int move = -1;
    for (int index = 0; index < move_list->count; ++index)
    {
        // add move data
        move = move_list->moves[index];
        int sqrf = get_move_source(move);
        int sqrt = get_move_target(move);
        int pp = get_move_promoted(move);

        // move = promotion ?
        if (promotionpiece > -1)
        {
            if (sqrf == sqf && sqrt == sqt && promotionpiece == pp)
            {
                break;
            }
        }
        // move is not promotion
        else
        {
            if (sqrf == sqf && sqrt == sqt)
            {
                break;
            }
        }
    }

    // no move found
    if (move == -1)
        return 0;

    // test move
    if (!rb_make_move(move, all_moves))
    {
        return 0;
    }

    // move is valid
    rb_add_move(rb_game_moves, move);
    return 1;
}

// make the put move from the human player
int rb_doPutMove(moves_t *move_list, int sqt, int putpiece)
{
    int move = -1;
    for (int index = 0; index < move_list->count; ++index)
    {
        // add move data
        move = move_list->moves[index];
        int sqrf = get_move_source(move);
        int sqrt = get_move_target(move);
        int pp = get_move_piece(move);
        
        if (sqrf == sqt && sqrt == sqt && pp == putpiece )
        {
            break;
        }
    }

    // no move found
    if (move == -1)
        return 0;

    // test move
    if (!lb_make_move(move, all_moves))
    {
        return 0;
    }
    
    // move is valid
    lb_add_move(lb_game_moves, move);
    return 1;
}

// calculate the best move for the ai in a seperate thread
void *lb_task(void *arg)
{
    lb_task_ready = 0;
    lb_thread_busy = 1;
    int depth = 64;
    if (lb_game_moves->count % 2 == 1)
        lb_movestogo = Max(50 - (lb_game_moves->count - 1) / 2, 10);
    else
        lb_movestogo = Max(50 - lb_game_moves->count / 2, 10);
    lb_ucitime = Max(lb_timer[lb_side] * 1000 / lb_movestogo, 1000);
    if (lb_ucitime > 1500)
        lb_ucitime -= 50;
    lb_timeset = 1;
    lb_starttime = lb_get_time_ms();
    lb_stoptime = lb_starttime + lb_ucitime;
#ifndef NDEBUG // print only in debug mode
    // print debug info
    printf("\nlb\n time: %d  start: %u  stop: %u  depth: %d  timeset: %d\n",
           lb_ucitime, lb_starttime, lb_stoptime, depth, lb_timeset);
#endif
    // search position with depth of 64
    lb_search_position(depth);
    lb_timeset = 0;
    // set the flag so the program knows that the thread is finished
    lb_task_ready = 1;
    return NULL;
}

void *rb_task(void *arg)
{
    rb_task_ready = 0;
    rb_thread_busy = 1;
    int depth = 64;
    if (rb_game_moves->count % 2 == 1)
        rb_movestogo = Max(50 - (rb_game_moves->count - 1) / 2, 10);
    else
        rb_movestogo = Max(50 - rb_game_moves->count / 2, 10);
    rb_ucitime = Max(rb_timer[rb_side] * 1000 / rb_movestogo, 1000);
    if (rb_ucitime > 1500)
        rb_ucitime -= 50;
    rb_timeset = 1;
    rb_starttime = rb_get_time_ms();
    rb_stoptime = rb_starttime + rb_ucitime;
#ifndef NDEBUG // print only in debug mode
    // print debug info
    printf("\nrb\n time: %d  start: %u  stop: %u  depth: %d  timeset: %d\n",
           rb_ucitime, rb_starttime, rb_stoptime, depth, rb_timeset);
#endif
    // search position with depth of 64
    rb_search_position(depth);
    rb_timeset = 0;
    // set the flag so the program knows that the thread is finished
    rb_task_ready = 1;
    return NULL;
}

// draw the board and the pieces for the gui, one board of a time
void draw_board(int side)
{
    int dcol = side == left ? lb_brd_col : rb_brd_col;
    int drow = side == left ? lb_brd_row : rb_brd_row;
    DrawRectangle(
        dcol - 2,
        drow - 2,
        BOARD_SIZE + 4,
        BOARD_SIZE + 4,
        BROWN);
    DrawTexture(
        board,
        dcol,
        drow,
        RAYWHITE);
    // Draw coordinates
    for (int i = 0; i < 8; ++i)
    {
        if (!side)
        {
            DrawText(x_co[7 - i], dcol + i * SQUARE_SIZE + HALF_SQUARE_SIZE, drow - HALF_SQUARE_SIZE, 20, WHITE);
            DrawText(y_co[7 - i], dcol - HALF_SQUARE_SIZE, drow + i * SQUARE_SIZE + HALF_SQUARE_SIZE, 20, WHITE);
        }
        else
        {
            DrawText(x_co[i], dcol + i * SQUARE_SIZE + HALF_SQUARE_SIZE, drow - HALF_SQUARE_SIZE, 20, WHITE);
            DrawText(y_co[i], dcol - HALF_SQUARE_SIZE, drow + i * SQUARE_SIZE + HALF_SQUARE_SIZE, 20, WHITE);
        }
    }
    // Draw pieces
    for (int y = 0; y < 8; ++y)
    {
        for (int x = 0; x < 8; ++x)
        {
            int sqr = side == left ? 63 - (y * 8 + x) : y * 8 + x;
            int piece_row = drow + y * SQUARE_SIZE;
            int piece_col = dcol + x * SQUARE_SIZE;
            int show_options = (!side) ? (lb_human_player == lb_side || lb_human_player == both) && !lb_thread_busy
                                       : (rb_human_player == rb_side || rb_human_player == both) && !rb_thread_busy;
            int pbit, sbit, cbit, piece;
            if (!side)
            {
                pbit = show_options && get_bit(lb_move_options[0], sqr) ? 1 : 0;
                sbit = show_options && lb_selectpiece >= 0 && get_bit(lb_piece_options[lb_selectpiece], sqr) ? 1 : 0;
                cbit = show_options && lb_capselectpiece >= 0 && get_bit(lb_cap_piece_options[lb_capselectpiece], sqr) ? 1 : 0;
                piece = lb_drawboard[sqr];
            }
            else
            {
                pbit = show_options && get_bit(rb_move_options[0], sqr) ? 1 : 0;
                sbit = show_options && rb_selectpiece >= 0 && get_bit(rb_piece_options[rb_selectpiece], sqr) ? 1 : 0;
                cbit = show_options && rb_capselectpiece >= 0 && get_bit(rb_cap_piece_options[rb_capselectpiece], sqr) ? 1 : 0;
                piece = rb_drawboard[sqr];
            }
            if (pbit)
            {
                DrawRectangleLines(
                    piece_col,
                    piece_row,
                    SQUARE_SIZE - 2,
                    SQUARE_SIZE - 2,
                    YELLOW);
                DrawRectangleLines(
                    piece_col + 1,
                    piece_row + 1,
                    SQUARE_SIZE - 4,
                    SQUARE_SIZE - 4,
                    YELLOW);
            }
            if (sbit || cbit)
            {
                DrawRectangleLines(
                    piece_col,
                    piece_row,
                    SQUARE_SIZE - 2,
                    SQUARE_SIZE - 2,
                    GREEN);
                DrawRectangleLines(
                    piece_col + 1,
                    piece_row + 1,
                    SQUARE_SIZE - 4,
                    SQUARE_SIZE - 4,
                    GREEN);
            }
            if (piece >= P && piece <= k)
                DrawTexture(
                    large_pieces[piece],
                    piece_col + 2,
                    piece_row + 2,
                    RAYWHITE);
        }
    }
    // Draw captured pieces
    if (!side) // left board
    {
        for (int bb_piece = P; bb_piece <= Q; ++bb_piece)
        {
            int cnt = lb_drawcappieces[bb_piece];
            if (cnt > 0)
            {
                DrawTexture(
                    small_pieces[bb_piece],
                    lb_brd_col + bb_piece * SQUARE_SIZE,
                    lb_brd_row - SQUARE_SIZE,
                    RAYWHITE);
                if (cnt > 1)
                {
                    DrawText(
                        number[cnt],
                        lb_brd_col + bb_piece * SQUARE_SIZE + HALF_SQUARE_SIZE,
                        lb_brd_row - SQUARE_SIZE,
                        20,
                        RAYWHITE);
                }
            }
        }
        for (int bb_piece = p; bb_piece <= q; ++bb_piece)
        {
            int cnt = lb_drawcappieces[bb_piece];
            if (cnt > 0)
            {
                DrawTexture(
                    small_pieces[bb_piece],
                    lb_brd_col + (bb_piece - 6) * SQUARE_SIZE,
                    lb_brd_row + BOARD_SIZE,
                    RAYWHITE);
                if (cnt > 1)
                {
                    DrawText(
                        number[cnt],
                        lb_brd_col + (bb_piece - 6) * SQUARE_SIZE + HALF_SQUARE_SIZE,
                        lb_brd_row + BOARD_SIZE,
                        20,
                        RAYWHITE);
                }
            }
        }
    }
    else // right board
    {
        for (int bb_piece = P; bb_piece <= Q; ++bb_piece)
        {
            int cnt = rb_drawcappieces[bb_piece];
            if (cnt > 0)
            {
                DrawTexture(
                    small_pieces[bb_piece],
                    rb_brd_col + bb_piece * SQUARE_SIZE,
                    rb_brd_row + BOARD_SIZE,
                    RAYWHITE);
                if (cnt > 1)
                {
                    DrawText(
                        number[cnt],
                        rb_brd_col + bb_piece * SQUARE_SIZE + HALF_SQUARE_SIZE,
                        rb_brd_row + BOARD_SIZE,
                        20,
                        RAYWHITE);
                }
            }
        }
        for (int bb_piece = p; bb_piece <= q; ++bb_piece)
        {
            int cnt = rb_drawcappieces[bb_piece];
            if (cnt > 0)
            {
                DrawTexture(
                    small_pieces[bb_piece],
                    rb_brd_col + (bb_piece - 6) * SQUARE_SIZE,
                    rb_brd_row - SQUARE_SIZE,
                    RAYWHITE);
                if (cnt > 1)
                {
                    DrawText(
                        number[cnt],
                        rb_brd_col + (bb_piece - 6) * SQUARE_SIZE + HALF_SQUARE_SIZE,
                        rb_brd_row - SQUARE_SIZE,
                        20,
                        RAYWHITE);
                }
            }
        }
    }
}

// get the sqrx
int get_sqrx(int col, int min)
{
    int n = min;
    for (int i = 0; i <= 7; ++i)
    {
        n += SQUARE_SIZE;
        if (col < n)
            return i;
    }
    return -9;
}

// get the sqry
int get_sqry(int row)
{
    int n = 0;
    for (int i = -2; i <= 9; ++i)
    {
        n += SQUARE_SIZE;
        if (row < n)
            return i;
    }
    return -9;
}

// is game ended 
// by checkmate, stalemate or 
// a draw by the 50 moves rule 
// or a draw by 3 times repetition 
void lb_game_end_check()
{
    // checkmate or stalemate
    if (lb_gamestate == PlayGame && lb_move_options[0] == 0ULL)
    {
        // no move possible
        lb_gamestate = StopGame;
        if (!lb_side) // white to move
        {
            // white king in check ?
            if (lb_is_square_attacked(lb_get_ls1b_index(lb_bitboards[K]), black))
                lb_game_end = wMate;
            else
                lb_game_end = wDrawPat;
        }
        else // black to move
        {
            // black king in check?
            if (lb_is_square_attacked(lb_get_ls1b_index(lb_bitboards[k]), white))
                lb_game_end = bMate;
            else
                lb_game_end = bDrawPat;
        }
        if (rb_gamestate == StopGame)
            game_state = StopGame;
        return;
    }
    
    // fifty moves rule
    if (lb_fifty >= 100)
    {
        lb_gamestate = StopGame;
        lb_game_end = Draw50;
        if (rb_gamestate == StopGame)
            game_state = StopGame;
        return;
    }
    
    if (lb_game_hashkey_index < 999)
        lb_game_hashkey[lb_game_hashkey_index++] = lb_hash_key;
    else
        PrintAssert(lb_game_hashkey_index < 999);
    
    int counter = 0;
    // loop over game haskey range
    for (int index = 0; index < lb_game_hashkey_index; ++index)
        if (lb_game_hashkey[index] == lb_hash_key)
        {
            ++counter;
            if (counter == 3)
            {
                lb_gamestate = StopGame;
                lb_game_end = DrawRep;
                if (rb_gamestate == StopGame)
                    game_state = StopGame;
                break;
            }
        }
}

// is game ended 
// by checkmate, stalemate or 
// a draw by the 50 moves rule 
// or a draw by 3 times repetition 
void rb_game_end_check()
{
    // checkmate or stalemate ?
    if (rb_gamestate == PlayGame && rb_move_options[0] == 0ULL)
    {
        rb_gamestate = StopGame;
        if (!rb_side) // white to move
        {
            // white king in check ?
            if (rb_is_square_attacked(rb_get_ls1b_index(rb_bitboards[K]), black))
                rb_game_end = wMate;
            else
                rb_game_end = wDrawPat;
        }
        else // black to move
        {
            // black king in check ?
            if (rb_is_square_attacked(rb_get_ls1b_index(rb_bitboards[k]), white))
                rb_game_end = bMate;
            else
                rb_game_end = bDrawPat;
        }
        if (lb_gamestate == StopGame)
            game_state = StopGame;
    }
    
    // fifty moves rule
    if (rb_fifty >= 100)
    {
        rb_gamestate = StopGame;
        rb_game_end = Draw50;
        if (lb_gamestate == StopGame)
            game_state = StopGame;
        return;
    }

    // 3 times repetition ? 
    if (rb_game_hashkey_index < 999)
        rb_game_hashkey[rb_game_hashkey_index++] = rb_hash_key;
    else
        PrintAssert(rb_game_hashkey_index < 999);
    
    int counter = 0;
    
    // loop over game haskey range
    for (int index = 0; index < rb_game_hashkey_index; ++index)
        if (rb_game_hashkey[index] == rb_hash_key)
        {
            ++counter;
            if (counter == 3)
            {
                rb_gamestate = StopGame;
                rb_game_end = DrawRep;
                if (lb_gamestate == StopGame)
                    game_state = StopGame;
                break;
            }
        }
}

// process a move made 
void lb_process_a_move()
{
    lb_selectpiece = -1;
    lb_capselectpiece = -1;
    lb_selectsquare = -1;
    lb_promotion = 0;
    lb_promotionmove = -1;
    if (lb_gamestate == PlayGame)
    {
        for (int i = 0; i < 64; ++i)
            lb_drawboard[i] = lb_getPiece(i);
        if (lb_pt_pieces_count > 0)
        {
            for (int i = 0; i < lb_pt_pieces_count; ++i)
                lb_gui_pt_pieces[lb_gui_pt_pieces_count++] = lb_pt_pieces[i];
            lb_pt_pieces_count = 0;
        }
        lb_fillOptions(lb_gui_move_list, lb_move_options, lb_piece_options, lb_cap_piece_options);
        lb_game_end_check();
        lb_timer[lb_side ^ 1] += lb_plustimer[lb_side ^ 1];
        lb_press_clock = 1;
    }
}

void rb_process_a_move()
{
    rb_selectpiece = -1;
    rb_capselectpiece = -1;
    rb_selectsquare = -1;
    rb_promotion = 0;
    rb_promotionmove = -1;
    if (rb_gamestate == PlayGame)
    {
        for (int i = 0; i < 64; ++i)
            rb_drawboard[i] = rb_getPiece(i);
        if (rb_pt_pieces_count > 0)
        {
            for (int i = 0; i < rb_pt_pieces_count; ++i)
                rb_gui_pt_pieces[rb_gui_pt_pieces_count++] = rb_pt_pieces[i];
            rb_pt_pieces_count = 0;
        }
        rb_fillOptions(rb_gui_move_list, rb_move_options, rb_piece_options, rb_cap_piece_options);
        rb_game_end_check();
        rb_timer[rb_side ^ 1] += rb_plustimer[rb_side ^ 1];
        rb_press_clock = 1;
    }
}

// Handle the mouse click
void process_mouseclick(int side, int x, int y)
{
    // left board
    if (!side)
    {
        if (lb_gamestate != PlayGame)
            return;
        if (x < lb_brd_col || x > lb_brd_col + BOARD_SIZE)
            return;
        int sqrx = get_sqrx(x, SQUARE_SIZE);
        int sqry = get_sqry(y);
#ifndef NDEBUG
        printf("\nlb\nsqrx: %d, sqry: %d\n", sqrx, sqry);
#endif
        int sqr = -1;
        int on_board = 0;
        // select square on the board
        if (sqry >= 0 && sqry <= 7 && sqrx <= 7 && !lb_promotion)
        {
            sqr = 63 - (sqry * 8 + sqrx);
            on_board = 1;
            if (lb_human_player == lb_side || lb_human_player == both)
            {
                if (get_bit(lb_move_options[0], sqr))
                {
                    lb_selectpiece = sqr;
                    lb_capselectpiece = -1;
#ifndef NDEBUG // print only in debug mode
                    printf("\nlb\n selectpiece %s\n", lb_square_to_coordinates[lb_selectpiece]);
#endif
                }
            }
        }
        // select top pieces (white)
        else if (sqry == -1 && !lb_promotion)
        {
            if (lb_side == white && (lb_human_player == lb_side || lb_human_player == both))
            {
                if (sqrx >= 0 && sqrx <= 4 && lb_drawcappieces[sqrx])
                {
                    lb_capselectpiece = sqrx;
                    lb_selectpiece = -1;
                    lb_promotionmove = -1;
                }
            }
        }
        // select bottom pieces (black)
        else if (sqry == 8 && !lb_promotion)
        {
            if (lb_side == black && (lb_human_player == lb_side || lb_human_player == both))
            {
                if (sqrx >= 0 && sqrx <= 4 && lb_drawcappieces[sqrx + 6])
                {
                    lb_capselectpiece = sqrx + 6;
                    lb_selectpiece = -1;
                    lb_promotionmove = -1;
                }
            }
        }
        // select promotion piece bottom (black), top (white)
        else if ((sqry == 9 || sqry == -2) && lb_promotion && lb_promotionmove == -1)
        {
            int col = sqry == -2 ? white : black;
            int pos = (lb_promotion && sqrx >= 0 && sqrx <= 4) ? sqrx : -1;

            if (pos > -1)
                lb_promotionmove = promote_pieces[col][pos];

            if (lb_selectpiece != -1 && lb_selectsquare - 1)
            {
                int result = lb_doMove(lb_gui_move_list, lb_selectpiece, lb_selectsquare, lb_promotionmove);

                if (result)
                    lb_process_a_move();
            }
        }

        // a square on the board is selected
        if (on_board)
        {
            // a piece on the board is selected
            if (lb_selectpiece != -1)
            {
                if (get_bit(lb_piece_options[lb_selectpiece], sqr))
                {
                    lb_selectsquare = sqr;
                    if ((lb_drawboard[lb_selectpiece] == p && lb_get_rank[sqr] == 0) ||
                        (lb_drawboard[lb_selectpiece] == P && lb_get_rank[sqr] == 7))
                    {
                        lb_promotion = 1;
                        lb_promotionmove = -1;
                    }
                    else
                    {
                        lb_promotion = 0;
                        lb_promotionmove = -1;

#ifndef NDEBUG // print only in debug mode
                        printf("\nlb\n selectsquare %s\n", lb_square_to_coordinates[lb_selectsquare]);
#endif
                        int result = lb_doMove(lb_gui_move_list, lb_selectpiece, lb_selectsquare, lb_promotionmove);

                        if (result)
                            lb_process_a_move();
                    }
                }
            }
            else if (lb_capselectpiece != -1)
            {
                if (get_bit(lb_cap_piece_options[lb_capselectpiece], sqr))
                {
                    lb_selectsquare = sqr;
#ifdef _WIN64
#ifndef NDEBUG // print only in debug mode
                    printf("\nlb\n capselectpiece %c\n", lb_ascii_pieces[lb_capselectpiece]);
#endif
#else
#ifndef NDEBUG // print only in debug mode
                    printf("\nlb\n capselectpiece %s\n", lb_unicode_pieces[lb_capselectpiece]);
#endif
#ifndef NDEBUG // print only in debug mode
                    printf(" selectsquare %s\n", lb_square_to_coordinates[lb_selectsquare]);
#endif
#endif
                    int result = lb_doPutMove(lb_gui_move_list, lb_selectsquare, lb_capselectpiece);

                    if (result)
                        lb_process_a_move();
                }
            }
        }
    }
    // right board
    else
    {
        if (rb_gamestate != PlayGame)
            return;
        if (x < rb_brd_col || x > rb_brd_col + BOARD_SIZE)
            return;
        int sqrx = get_sqrx(x, SCREEN_WIDTH / 2 + SQUARE_SIZE);
        int sqry = get_sqry(y);
#ifndef NDEBUG
        printf("\nrb\nsqrx: %d, sqry: %d\n", sqrx, sqry);
#endif
        int sqr = -1;
        int on_board = 0;
        // select square on the board
        if (sqry >= 0 && sqry <= 7 && sqrx <= 7 && !rb_promotion)
        {
            sqr = sqry * 8 + sqrx;
            on_board = 1;
            if (rb_human_player == rb_side || rb_human_player == both)
            {
                if (get_bit(rb_move_options[0], sqr))
                {
                    rb_selectpiece = sqr;
                    rb_capselectpiece = -1;
#ifndef NDEBUG // print only in debug mode
                    printf("\nrb\n selectpiece %s\n", rb_square_to_coordinates[rb_selectpiece]);
#endif
                }
            }
        }
        // select top pieces (black)
        else if (sqry == -1 && !rb_promotion)
        {
            if (rb_side == black && (rb_human_player == black || rb_human_player == both))
            {
                if (sqrx >= 0 && sqrx <= 4 && rb_drawcappieces[sqrx + 6])
                {
                    rb_capselectpiece = sqrx + 6;
                    rb_selectpiece = -1;
                    rb_promotionmove = -1;
                }
            }
        }
        // select bottom pieces (white)
        else if (sqry == 8 && !rb_promotion)
        {
            if ((rb_human_player == rb_side || rb_human_player == both) && rb_side == white)
            {
                if (sqrx >= 0 && sqrx <= 4 && rb_drawcappieces[sqrx])
                {
                    rb_capselectpiece = sqrx;
                    rb_selectpiece = -1;
                    rb_promotionmove = -1;
                }
            }
        }
        // select promotion piece bottom (white), top (black)
        else if ((sqry == 9 || sqry == -2) && rb_promotion && rb_promotionmove == -1)
        {
            int col = sqry == -2 ? black : white;
            int pos = (rb_promotion && sqrx >= 0 && sqrx <= 4) ? sqrx : -1;

            if (pos > -1)
                rb_promotionmove = promote_pieces[col][pos];

            if (rb_selectpiece != -1 && rb_selectsquare != -1)
            {
                int result = rb_doMove(rb_gui_move_list, rb_selectpiece, rb_selectsquare, rb_promotionmove);

                if (result)
                    rb_process_a_move();
            }
        }

        // a square on the board is selected
        if (on_board)
        {
            // a piece on the board is selected
            if (rb_selectpiece != -1)
            {
                if (get_bit(rb_piece_options[rb_selectpiece], sqr))
                {
                    rb_selectsquare = sqr;
                    if ((rb_drawboard[rb_selectpiece] == P && rb_get_rank[sqr] == 7) ||
                        (rb_drawboard[rb_selectpiece] == p && rb_get_rank[sqr] == 0))
                    {
                        rb_promotion = 1;
                        rb_promotionmove = -1;
                    }
                    else
                    {
                        rb_promotion = 0;
                        rb_promotionmove = -1;
#ifndef NDEBUG // print only in debug mode
                        printf("\nrb\n selectsquare %s\n", rb_square_to_coordinates[rb_selectsquare]);
#endif
                        int result = rb_doMove(rb_gui_move_list, rb_selectpiece, rb_selectsquare, rb_promotionmove);

                        if (result)
                            rb_process_a_move();
                    }
                }
            }
            else if (rb_capselectpiece != -1)
            {
                if (get_bit(rb_cap_piece_options[rb_capselectpiece], sqr))
                {
                    rb_selectsquare = sqr;
#ifdef _WIN64
#ifndef NDEBUG // print only in debug mode
                    printf("\nrb\n capselectpiece %c\n", rb_ascii_pieces[rb_capselectpiece]);
#endif
#else
#ifndef NDEBUG // print only in debug mode
                    printf("\nrb\n capselectpiece %s\n", rb_unicode_pieces[rb_capselectpiece]);
#endif
#ifndef NDEBUG // print only in debug mode
                    printf(" selectsquare %s\n", rb_square_to_coordinates[rb_selectsquare]);
#endif
#endif
                    int result = rb_doPutMove(rb_gui_move_list, rb_selectsquare, rb_capselectpiece);

                    if (result)
                        rb_process_a_move();
                }
            }
        }
    }
}

// Draw clock time
void draw_clocktime(int brdside, int side, int posx, int posy, Color col)
{
    DrawTexture(chessclock, posx, posy, BROWN);
    DrawRectangle(posx + 16, posy + 30, 115, 20, BROWN);
    char *mid = ":";
    if (!brdside)
    {
        DrawText(number[lb_clocktime[side][0]], posx + 18, posy + 30, 20, col);
        DrawText(mid, posx + 38, posy + 30, 20, col);
        DrawText(number[lb_clocktime[side][1]], posx + 48, posy + 30, 20, col);
        DrawText(number[lb_clocktime[side][2]], posx + 68, posy + 30, 20, col);
        DrawText(mid, posx + 88, posy + 30, 20, col);
        DrawText(number[lb_clocktime[side][3]], posx + 98, posy + 30, 20, col);
        DrawText(number[lb_clocktime[side][4]], posx + 118, posy + 30, 20, col);
    }
    else
    {
        DrawText(number[rb_clocktime[side][0]], posx + 18, posy + 30, 20, col);
        DrawText(mid, posx + 38, posy + 30, 20, col);
        DrawText(number[rb_clocktime[side][1]], posx + 48, posy + 30, 20, col);
        DrawText(number[rb_clocktime[side][2]], posx + 68, posy + 30, 20, col);
        DrawText(mid, posx + 88, posy + 30, 20, col);
        DrawText(number[rb_clocktime[side][3]], posx + 98, posy + 30, 20, col);
        DrawText(number[rb_clocktime[side][4]], posx + 118, posy + 30, 20, col);
    }
}

// set the game data
void setup_game()
{
    lb_selectpiece = -1;
    lb_capselectpiece = -1;
    lb_selectsquare = -1;
    lb_promotion = 0;
    lb_promotionmove = -1;
    lb_game_end = 0;
    lb_game_hashkey_index = 0;
    lb_gui_pt_pieces_count = 0;
    memset(lb_game_hashkey, 0ULL, sizeof(lb_game_hashkey));
    memset(lb_game_moves, 0, sizeof(lb_game_moves));
    memset(lb_gui_move_list, 0, sizeof(lb_gui_move_list));
    memset(lb_gui_pt_pieces, 0, sizeof(lb_gui_pt_pieces));
    
    rb_selectpiece = -1;
    rb_capselectpiece = -1;
    rb_selectsquare = -1;
    rb_promotion = 0;
    rb_promotionmove = -1;
    rb_game_end = 0;
    rb_game_hashkey_index = 0;
    rb_gui_pt_pieces_count = 0;
    memset(rb_game_hashkey, 0ULL, sizeof(rb_game_hashkey));
    memset(rb_game_moves, 0, sizeof(rb_game_moves));
    memset(rb_gui_move_list, 0, sizeof(rb_gui_move_list));
    memset(rb_gui_pt_pieces, 0, sizeof(rb_gui_pt_pieces));
    
    // set timer settings for the clock
    lb_timer[white] = lb_timer[black] = game_player_time;
    rb_timer[white] = rb_timer[black] = game_player_time;
    lb_plustimer[white] = lb_plustimer[black] = game_player_plus_time;
    rb_plustimer[white] = rb_plustimer[black] = game_player_plus_time;
    // fill the clock settings
    fill_clocktime(left, white);
    fill_clocktime(left, black);
    fill_clocktime(right, white);
    fill_clocktime(right, black);

    if (!game_color)
    {
        rb_human_player = white;
        rb_ai_player = black;
        lb_human_player = -1;
        lb_ai_player = both;
    }
    else
    {
        lb_human_player = black;
        lb_ai_player = white;
        rb_human_player = -1;
        rb_ai_player = both;
    }

    lb_gamestate = PlayGame;
    rb_gamestate = PlayGame;
    lb_thinktimer[white] = lb_thinktimer[black] = 0;
    rb_thinktimer[white] = rb_thinktimer[black] = 0;
    lb_press_clock = 1;
    rb_press_clock = 1;
    lb_stop_game_flag = 0;
    rb_stop_game_flag = 0;
    // setup new game
    lb_parse_fen(start_position);
    rb_parse_fen(start_position);
    // clear hash table
    lb_clear_hash_table();
    rb_clear_hash_table();
    // fill the options bitboards used in the gui for both boards because the color choice is not known yet
    lb_fillOptions(lb_gui_move_list, lb_move_options, lb_piece_options, lb_cap_piece_options);
    rb_fillOptions(rb_gui_move_list, rb_move_options, rb_piece_options, rb_cap_piece_options);
    lb_game_hashkey[lb_game_hashkey_index++] = lb_hash_key;
    rb_game_hashkey[rb_game_hashkey_index++] = lb_hash_key;
    // initial set up of the draw board
    for (int i = 0; i < 64; ++i)
        lb_drawboard[i] = lb_getPiece(i);
    for (int i = 0; i < 64; ++i)
        rb_drawboard[i] = rb_getPiece(i);
    game_state = StartGame;
}

// Start program
int main()
{
    // set engine on or off
    int use_lb_engine = 1;
    int use_rb_engine = 1;

    // init all
    lb_init_all();
    rb_init_all();

    // initialize raylib
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title);
    // load images and set sizes
    table = LoadTexture("./assets/Table.png");
    table.width = SCREEN_WIDTH;
    table.height = SCREEN_HEIGHT;
    board = LoadTexture("./assets/Board.png");
    board.width = BOARD_SIZE;
    board.height = BOARD_SIZE;
    for (int i = 0; i < 12; ++i)
    {
        large_pieces[i] = LoadTexture(pieces[i]);
        large_pieces[i].width = LARGE_PIECE_SIZE;
        large_pieces[i].height = LARGE_PIECE_SIZE;
        small_pieces[i] = LoadTexture(pieces[i]);
        small_pieces[i].width = SMALL_PIECE_SIZE;
        small_pieces[i].height = SMALL_PIECE_SIZE;
    }
    choice = LoadTexture("./assets/Choice.png");
    choice.width = LARGE_PIECE_SIZE;
    choice.height = LARGE_PIECE_SIZE;
    enterbtn = LoadTexture("assets/Enter.png");
    enterbtn.height = SQUARE_SIZE;
    enterbtn.width = SQUARE_SIZE * 2;
    plusminbtn = LoadTexture("assets/Plusmin.png");
    plusminbtn.height = SQUARE_SIZE;
    plusminbtn.width = SQUARE_SIZE * 2;
    chessclock = LoadTexture("assets/Clock.png");
    chessclock.width = SQUARE_SIZE * 2;
    chessclock.height = SQUARE_SIZE;
    ai_image = LoadTexture("assets/AI.png");
    ai_image.width = SQUARE_SIZE;
    ai_image.height = SQUARE_SIZE;
    human_image = LoadTexture("assets/HUMAN.png");
    human_image.width = SQUARE_SIZE;
    human_image.height = SQUARE_SIZE;
    // set frames per second
    SetTargetFPS(10);

    // mainloop
    while (!WindowShouldClose())
    {
        // update
        if (lb_press_clock)
        {
            lb_side2move = lb_side;
            lb_starttimer[lb_side2move] = (int)(GetTime() * 1000);
            lb_press_clock = 0;
        }
        if (rb_press_clock)
        {
            rb_side2move = rb_side;
            rb_starttimer[rb_side2move] = (int)(GetTime() * 1000);
            rb_press_clock = 0;
        }
        if (lb_gamestate == PlayGame)
        {
            // time is up?
            if (lb_timer[lb_side2move] == 0)
            {
                lb_gamestate = StopGame;
                lb_game_end = lb_side2move == white ? bTime : wTime;
                if (rb_gamestate == StopGame)
                    game_state = StopGame;
            }
            else // update timer
            {
                int duration = (int)(GetTime() * 1000.0);
                int plustime = Max(duration - lb_starttimer[lb_side2move], 0);
                lb_thinktimer[lb_side2move] += plustime;
                lb_starttimer[lb_side2move] = duration;
                if (lb_thinktimer[lb_side2move] >= 1000)
                {
                    --lb_timer[lb_side2move];
                    lb_thinktimer[lb_side2move] %= 1000;
                }
            }
        }
        if (rb_gamestate == PlayGame)
        {
            // time is up?
            if (rb_timer[rb_side2move] == 0)
            {
                rb_gamestate = StopGame;
                rb_game_end = rb_side2move == white ? bTime : wTime;
                if (lb_gamestate == StopGame)
                    game_state = StopGame;
            }
            else
            {
                int duration = (int)(GetTime() * 1000.0);
                int plustime = Max(duration - rb_starttimer[rb_side2move], 0);
                rb_thinktimer[rb_side2move] += plustime;
                rb_starttimer[rb_side2move] = duration;
                if (rb_thinktimer[rb_side2move] >= 1000)
                {
                    --rb_timer[rb_side2move];
                    rb_thinktimer[rb_side2move] %= 1000;
                }
            }
        }
        fill_clocktime(left, lb_side2move);
        fill_clocktime(left, lb_side2move ^ 1);
        fill_clocktime(right, rb_side2move);
        fill_clocktime(right, rb_side2move ^ 1);

        if (!lb_thread_busy)
        {
            if (rb_gui_pt_pieces_count > 0)
            {
                for (int i = 0; i < rb_gui_pt_pieces_count; ++i)
                    lb_cap_pieces[lb_cap_pieces_count++] = rb_gui_pt_pieces[i];
                rb_gui_pt_pieces_count = 0;
                lb_getCapturedPieces();
            }
        }

        if (!rb_thread_busy)
        {
            if (lb_gui_pt_pieces_count > 0)
            {
                for (int i = 0; i < lb_gui_pt_pieces_count; ++i)
                    rb_cap_pieces[rb_cap_pieces_count++] = lb_gui_pt_pieces[i];
                lb_gui_pt_pieces_count = 0;
                rb_getCapturedPieces();
            }
        }
            
        // draw
        BeginDrawing();
        ClearBackground(RAYWHITE);
        if (game_state == Input)
        {
            DrawRectangle(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT, BLUE);
            // rules passthrough chess / operating program
            for (int i = 0; i < 26; i++)
            {
                DrawText(rules[i], 8, 24 * i + 24, 20, WHITE);
            }
            // color choice
            DrawText(
                colorstr[game_color],
                SQUARE_SIZE,
                SQUARE_SIZE * 9 + HALF_SQUARE_SIZE,
                20,
                YELLOW);
            // time choice
            DrawTexture(choice, SQUARE_SIZE * 5, SQUARE_SIZE * 9, RAYWHITE);
            DrawText(
                timestr[game_time],
                SQUARE_SIZE,
                SQUARE_SIZE * 10 + HALF_SQUARE_SIZE,
                20,
                YELLOW);
            // plus time choice
            DrawTexture(plusminbtn, SQUARE_SIZE * 5, SQUARE_SIZE * 10, RAYWHITE);
            DrawText(
                plusstr[game_plus],
                SQUARE_SIZE,
                SQUARE_SIZE * 11 + HALF_SQUARE_SIZE,
                20,
                YELLOW);
            DrawTexture(plusminbtn, SQUARE_SIZE * 5, SQUARE_SIZE * 11, RAYWHITE);
            // enter button
            DrawTexture(enterbtn, SQUARE_SIZE * 9, SQUARE_SIZE * 11, RAYWHITE);
        }
        else
        {
            // Draw table
            DrawTexture(
                table,
                0,
                0,
                RAYWHITE);
            // Draw board left side
            draw_board(left);
            // Draw board right side
            draw_board(right);
            // Draw clocks

            // Draw players
            // left board
            // bottom
            DrawTexture(
                lb_ai_player == both ? ai_image : human_image,
                lb_brd_col + BOARD_SIZE,
                lb_brd_row + BOARD_SIZE - SQUARE_SIZE,
                RAYWHITE) ;
            // top
            DrawTexture(
                lb_human_player == both ? human_image : ai_image,
                lb_brd_col + BOARD_SIZE,
                lb_brd_row,
                RAYWHITE);
            // right board
            // top
            DrawTexture(
                rb_human_player == both ? human_image : ai_image,
                rb_brd_col + BOARD_SIZE,
                rb_brd_row,
                RAYWHITE);
            // bottom
            DrawTexture(
                rb_ai_player == both ? ai_image : human_image,
                rb_brd_col + BOARD_SIZE,
                rb_brd_row + BOARD_SIZE - SQUARE_SIZE,
                RAYWHITE);
            
            // left board
            int clock_col = lb_brd_col + BOARD_SIZE - SQUARE_SIZE * 2;
            int clock_row = (!lb_side2move) ? 0 : SCREEN_HEIGHT - SQUARE_SIZE;
            int xclock_row = (!lb_side2move) ? SCREEN_HEIGHT - SQUARE_SIZE : 0;
            Color color = (!lb_side2move) ? RAYWHITE : BLACK;
            draw_clocktime(left, lb_side2move, clock_col, clock_row, color);
            draw_clocktime(left, lb_side2move ^ 1, clock_col, xclock_row, RED);
            // right board
            clock_col = rb_brd_col + BOARD_SIZE - SQUARE_SIZE * 2;
            clock_row = (!rb_side2move) ? SCREEN_HEIGHT - SQUARE_SIZE : 0;
            xclock_row = (!rb_side2move) ? 0 : SCREEN_HEIGHT - SQUARE_SIZE;
            color = (!rb_side2move) ? RAYWHITE : BLACK;
            draw_clocktime(right, rb_side2move, clock_col, clock_row, color);
            draw_clocktime(right, rb_side2move ^ 1, clock_col, xclock_row, RED);

            if (!lb_thread_busy)
                // choice promotion piece
                if (lb_promotion && lb_promotionmove == -1 && (lb_human_player == lb_side || lb_human_player == both))
                {
                    Color wcol = {255, 255, 255, 64};
                    DrawRectangle(lb_brd_col, lb_brd_row - SQUARE_SIZE, BOARD_SIZE, BOARD_SIZE + SQUARE_SIZE * 2, wcol);
                    int row = (lb_side == white) ? lb_brd_row - SQUARE_SIZE * 2 : lb_brd_row + BOARD_SIZE + SQUARE_SIZE;

                    for (int index = 0; index < 4; ++index)
                    {
                        int piece = promote_pieces[lb_side][index];
                        DrawTexture(
                            large_pieces[piece],
                            lb_brd_col + index * SQUARE_SIZE,
                            row,
                            RAYWHITE);
                    }
                    DrawText(
                        "Kies",
                        lb_brd_col - SQUARE_SIZE,
                        row,
                        20,
                        YELLOW);
                }

            if (!rb_thread_busy)
                // choice promotion piece
                if (rb_promotion && rb_promotionmove == -1 && (rb_human_player == rb_side || rb_human_player == both))
                {
                    Color wcol = {255, 255, 255, 64};
                    DrawRectangle(rb_brd_col, rb_brd_row - SQUARE_SIZE, BOARD_SIZE, BOARD_SIZE + SQUARE_SIZE * 2, wcol);
                    int row = (rb_side == white) ? rb_brd_row + BOARD_SIZE + SQUARE_SIZE : rb_brd_row - SQUARE_SIZE * 2;

                    for (int index = 0; index < 4; ++index)
                    {
                        int piece = promote_pieces[rb_side][index];
                        DrawTexture(
                            large_pieces[piece],
                            rb_brd_col + index * SQUARE_SIZE,
                            row,
                            RAYWHITE);
                    }
                    DrawText(
                        "Kies",
                        rb_brd_col - SQUARE_SIZE,
                        row,
                        20,
                        YELLOW);
                }

            // draw text when the game is finished
            if (lb_gamestate == StopGame)
            {
                DrawText(
                    text_game_end[lb_game_end],
                    lb_brd_col,
                    HALF_SQUARE_SIZE,
                    20,
                    YELLOW);
                if (game_state == StopGame)
                    DrawText(
                        "Druk op F5 voor opnieuw spelen",
                        lb_brd_col,
                        SCREEN_HEIGHT - HALF_SQUARE_SIZE,
                        20,
                        YELLOW);
                else
                    DrawText(
                        "Wacht op het andere bord",
                        lb_brd_col,
                        SCREEN_HEIGHT - HALF_SQUARE_SIZE,
                        20,
                        YELLOW);
            }

            // draw text when the game is finished
            if (rb_gamestate == StopGame)
            {
                DrawText(
                    text_game_end[rb_game_end],
                    rb_brd_col,
                    HALF_SQUARE_SIZE,
                    20,
                    YELLOW);
                if (game_state == StopGame)
                    DrawText(
                        "Druk op F5 voor opnieuw spelen",
                        rb_brd_col,
                        SCREEN_HEIGHT - HALF_SQUARE_SIZE,
                        20,
                        YELLOW);
                else
                    DrawText(
                        "Wacht op het andere bord",
                        rb_brd_col,
                        SCREEN_HEIGHT - HALF_SQUARE_SIZE,
                        20,
                        YELLOW);
            }
        }

        EndDrawing();

        // keypress
        if (IsKeyPressed(KEY_ENTER))
        {
            if (game_state == Input)
            {
                game_player_time = timeint[game_time];
                game_player_plus_time = plusint[game_plus];
                setup_game();
            }
        }

        if (IsKeyPressed(KEY_F5))
        {
            if (game_state == StopGame)
                game_state = Input;
        }

        if (IsKeyPressed(KEY_F7))
        {
            if (game_state == Input)
            {
                game_player_time = timeint[game_time];
                game_player_plus_time = plusint[game_plus];
                setup_game();
                lb_human_player = both;
                rb_human_player = both;
                lb_ai_player = -1;
                rb_ai_player = -1;
            }
        }

        // Mouse Press
        if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
        {
            int x = GetMouseX();
            int y = GetMouseY();
            if (game_state == Input)
            {
                if (x >= SQUARE_SIZE * 5 &&
                    x < SQUARE_SIZE * 6 &&
                    y >= SQUARE_SIZE * 9 &&
                    y < SQUARE_SIZE * 10)
                {
                    game_color ^= 1;
                }
                else if (x >= SQUARE_SIZE * 5 &&
                         x < SQUARE_SIZE * 6 &&
                         y >= SQUARE_SIZE * 10 &&
                         y < SQUARE_SIZE * 11)
                {
                    if (game_time < 2)
                    {
                        game_time++;
                    }
                }
                else if (x >= SQUARE_SIZE * 6 &&
                         x < SQUARE_SIZE * 7 &&
                         y >= SQUARE_SIZE * 10 &&
                         y < SQUARE_SIZE * 11)
                {
                    if (game_time > 0)
                    {
                        game_time--;
                    }
                }
                else if (x >= SQUARE_SIZE * 5 &&
                         x < SQUARE_SIZE * 6 &&
                         y >= SQUARE_SIZE * 11 &&
                         y < SQUARE_SIZE * 12)
                {
                    if (game_plus < 3)
                    {
                        game_plus++;
                    }
                }
                else if (x >= SQUARE_SIZE * 6 &&
                         x < SQUARE_SIZE * 7 &&
                         y >= SQUARE_SIZE * 11 &&
                         y < SQUARE_SIZE * 12)
                {
                    if (game_plus > 0)
                    {
                        game_plus--;
                    }
                }
                else if (x >= SQUARE_SIZE * 9 &&
                         x < SQUARE_SIZE * 11 &&
                         y >= SQUARE_SIZE * 11 &&
                         y < SQUARE_SIZE * 13)
                {
                    game_player_time = timeint[game_time];
                    game_player_plus_time = plusint[game_plus];
                    setup_game();
                }
                continue;
            }

            if (lb_human_player == lb_side || lb_human_player == both)
            {
                process_mouseclick(left, x, y);
            }
            if (rb_human_player == rb_side || rb_human_player == both)
            {
                process_mouseclick(right, x, y);
            }
        }

        // AI player left board
        if (lb_gamestate == PlayGame && (lb_ai_player == lb_side || lb_ai_player == both))
        {
            if (use_lb_engine)
            {
                if (!lb_thread_busy)
                {
                    // start the thread
                    pthread_create(&lb_thread, NULL, lb_task, NULL);
                    // forget that the thread is started, the task lb_task_ready indicates when the thread is finished 
                    pthread_detach(lb_thread);
                }
                else if (lb_task_ready) // thread is finished
                {
                    int bestmove = lb_pv_table[0][0];
                    lb_add_move(lb_game_moves, bestmove);
#ifndef NDEBUG // print only in debug mode
                    // print offset
                    printf("\nlb\n");
                    lb_print_move(bestmove);
                    printf("\n");
#endif
                    lb_make_move(bestmove, all_moves);
                    // check for draw by 50 moves rule or 3 times repetiton rule
                    lb_process_a_move();
                    lb_thread_busy = 0;
                }
            }
        }
        // AI player right board
        
        if (rb_gamestate == PlayGame && (rb_ai_player == rb_side || rb_ai_player == both))
        {
            if (use_rb_engine)
            {
                if (!rb_thread_busy)
                {
                    // start the thread
                    pthread_create(&rb_thread, NULL, rb_task, NULL);
                    // forget that the thread is started
                    pthread_detach(rb_thread);
                }
                else if (rb_task_ready) // the thread is finished
                {
                    int bestmove = rb_pv_table[0][0];
                    rb_add_move(rb_game_moves, bestmove);
#ifndef NDEBUG // print only in debug mode
               // print offset
                    printf("\nrb\n");
                    rb_print_move(bestmove);
                    printf("\n");
#endif
                    rb_make_move(bestmove, all_moves);
                    rb_process_a_move();
                    rb_thread_busy = 0;
                }
            }
        }
    }

    // clean up
    // to stop searching rapidly when a thread is started
    lb_stop_game_flag = 1;

    // wait until the thread is finished
    if (lb_thread_busy)
    {
        while (!lb_task_ready)
        {
            sleep(1);
        }
    }

    // to stop searching rapidly when a thread is started
    rb_stop_game_flag = 1;

    // wait until the thread is finished
    if (rb_thread_busy)
    {
        while (!rb_task_ready)
        {
            sleep(1);
        }
    }

    // unload pictures
    for (int i = 0; i < 12; ++i)
    {
        UnloadTexture(large_pieces[i]);
        UnloadTexture(small_pieces[i]);
    }
    UnloadTexture(choice);
    UnloadTexture(table);
    UnloadTexture(board);
    UnloadTexture(enterbtn);
    UnloadTexture(plusminbtn);
    UnloadTexture(chessclock);
    UnloadTexture(ai_image);
    UnloadTexture(human_image);

    // close the raylib window
    CloseWindow();

    // free hash table memory on exit
    free(lb_hash_table);
    free(rb_hash_table);

    return 0;
}

// -----------------------------------------------------------------------
// End Gui code
// -----------------------------------------------------------------------
