#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "raylib.h"
#include <sys/time.h>
#include <pthread.h>

#define TEST 0
#define USE_ENGINE 1
#define SIZE 10
#define INFINITY 50000
#define MAX_PLY 64

// define bitboard data type
#define U64 unsigned long long

// set/get/pop bit macros
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

// draw sizes
#define SCREEN_WIDTH 816
#define SCREEN_HEIGHT 850
#define BOARD_ROW 68
#define BOARD_COL 68
#define BOARD_SIZE 680
#define SQUARE_SIZE 68
#define HALF_SQUARE_SIZE 34

// time settings
#define MAX_TIME 60
#define MIN_TIME 5
#define MAX_PLUS 30
#define MIN_PLUS 0

#define title "Draughts in Raylib-C (C)2025 Peter Veenendaal; versie: 0.80"

// define min max macros
#define Max(a, b) ((a) >= (b) ? (a) : (b))
#define Min(a, b) ((a) <= (b) ? (a) : (b))

#ifdef NDEBUG // release mode
#define PrintAssert(ignore) (void *)0
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

// search values
#define infinity 50000

// no hash entry found constant
#define no_hash_entry 100000

// transposition table hash flags
#define hash_flag_exact 0
#define hash_flag_alpha 1
#define hash_flag_beta 2

// colors
enum
{
    white,
    black,
    both,
};

// pieces
enum
{
    wPawn,
    wKing,
    bPawn,
    bKing,
    no_piece,
};

// directions
enum
{
    nw,
    ne,
    sw,
    se,
};

// move_kind
enum
{
    all_moves,
    only_captures
};

// game_state
enum
{
    Game_start,
    Game_play,
    Game_stop
};

// game_end
enum
{
    nop,
    bTime,   // white wins because black has no more time left
    wTime,   // black wins because white has no more time left
    bMove,   // white wins because black has no moves to make
    wMove,   // black wins because white has no moves to make
    bResign, // white wins because black resigned the game
    wResign, // black wins because white resigned the game
    draw50,  // draw by the 50 moves rule
    draw3,   // draw by repetition
};

// squares
//    0  1  2  3  4  5  6  7  8  9
// 0  -  1  -  2  -  3  -  4  -  5
// 1  6  -  7  -  8  -  9  - 10  -
// 2  - 11  - 12  - 13  - 14  - 15
// 3 16  - 17  - 18  - 19  - 20  -
// 4  - 21  - 22  - 23  - 24  - 25
// 5 26  - 27  - 28  - 29  - 30  -
// 6  - 31  - 32  - 33  - 34  - 35
// 7 36  - 37  - 38  - 39  - 40  -
// 8  - 41  - 42  - 43  - 44  - 45
// 9 46  - 47  - 48  - 49  - 50  -

typedef struct
{
    U64 moves[256];
    int counter;
    int caplength;
} moves_t;

typedef struct
{
    U64 hash_keys[512];
    int gamelength;
} game_t;

// transposition table data structure
typedef struct
{
    U64 hash_key; // "almost" unique chess position identifier
    int depth;    // current search depth
    int flag;     // flag the type of node (fail-low/fail-high/PV)
    int score;    // score (alpha/beta/PV)
} tt;             // transposition table (TT aka hash table)

// squares
//    0  1  2  3  4  5  6  7  8  9
// 0  -  1  -  2  -  3  -  4  -  5
// 1  6  -  7  -  8  -  9  - 10  -
// 2  - 11  - 12  - 13  - 14  - 15
// 3 16  - 17  - 18  - 19  - 20  -
// 4  - 21  - 22  - 23  - 24  - 25
// 5 26  - 27  - 28  - 29  - 30  -
// 6  - 31  - 32  - 33  - 34  - 35
// 7 36  - 37  - 38  - 39  - 40  -
// 8  - 41  - 42  - 43  - 44  - 45
// 9 46  - 47  - 48  - 49  - 50  -

const int squares[10][10] = {
    {0, 1, 0, 2, 0, 3, 0, 4, 0, 5},
    {6, 0, 7, 0, 8, 0, 9, 0, 10, 0},
    {0, 11, 0, 12, 0, 13, 0, 14, 0, 15},
    {16, 0, 17, 0, 18, 0, 19, 0, 20, 0},
    {0, 21, 0, 22, 0, 23, 0, 24, 0, 25},
    {26, 0, 27, 0, 28, 0, 29, 0, 30, 0},
    {0, 31, 0, 32, 0, 33, 0, 34, 0, 35},
    {36, 0, 37, 0, 38, 0, 39, 0, 40, 0},
    {0, 41, 0, 42, 0, 43, 0, 44, 9, 45},
    {46, 0, 47, 0, 48, 0, 49, 0, 50, 0},
};

const int rowcol[51][2] = {
    {0, 0},
    {0, 1},
    {0, 3},
    {0, 5},
    {0, 7},
    {0, 9}, //  1..5
    {1, 0},
    {1, 2},
    {1, 4},
    {1, 6},
    {1, 8}, //  6..10
    {2, 1},
    {2, 3},
    {2, 5},
    {2, 7},
    {2, 9}, // 11..15
    {3, 0},
    {3, 2},
    {3, 4},
    {3, 6},
    {3, 8}, // 16..20
    {4, 1},
    {4, 3},
    {4, 5},
    {4, 7},
    {4, 9}, // 21..25
    {5, 0},
    {5, 2},
    {5, 4},
    {5, 6},
    {5, 8}, // 26..30
    {6, 1},
    {6, 3},
    {6, 5},
    {6, 7},
    {6, 9}, // 31..35
    {7, 0},
    {7, 2},
    {7, 4},
    {7, 6},
    {7, 8}, // 36..40
    {8, 1},
    {8, 3},
    {8, 5},
    {8, 7},
    {8, 9}, // 41..45
    {9, 0},
    {9, 2},
    {9, 4},
    {9, 6},
    {9, 8}, // 46..50
};

static char *squarenumber[51] =
    {"",
     "01", "02", "03", "04", "05",
     "06", "07", "08", "09", "10",
     "11", "12", "13", "14", "15",
     "16", "17", "18", "19", "20",
     "21", "22", "23", "24", "25",
     "26", "27", "28", "29", "30",
     "31", "32", "33", "34", "35",
     "36", "37", "38", "39", "40",
     "41", "42", "43", "44", "45",
     "46", "47", "48", "49", "50"};

const int square_rank[51] =
    {
        0,
        1, 1, 1, 1, 1,
        2, 2, 2, 2, 2,
        3, 3, 3, 3, 3,
        4, 4, 4, 4, 4,
        5, 5, 5, 5, 5,
        6, 6, 6, 6, 6,
        7, 7, 7, 7, 7,
        8, 8, 8, 8, 8,
        9, 9, 9, 9, 9,
        10, 10, 10, 10, 10};

const int dir[4][52] = {
    {                    // nw
     0,                  // 0 at start
     0, 0, 0, 0, 0,      // 01 - 05
     0, 1, 2, 3, 4,      // 06 - 10
     6, 7, 8, 9, 10,     // 11 - 15
     0, 11, 12, 13, 14,  // 16 - 20
     16, 17, 18, 19, 20, // 21 - 25
     0, 21, 22, 23, 24,  // 26 - 30
     26, 27, 28, 29, 30, // 31 - 35
     0, 31, 32, 33, 34,  // 36 - 40
     36, 37, 38, 39, 40, // 41 - 45
     0, 41, 42, 43, 44,  // 46 - 50
     0},
    {
        // ne
        0,                  // 0 at start
        0, 0, 0, 0, 0,      //  01 - 05
        1, 2, 3, 4, 5,      //  06 - 10
        7, 8, 9, 10, 0,     //  11 - 15
        11, 12, 13, 14, 15, //  16 - 20
        17, 18, 19, 20, 0,  //  21 - 25
        21, 22, 23, 24, 25, //  26 - 30
        27, 28, 29, 30, 0,  //  31 - 35
        31, 32, 33, 34, 35, //  36 - 40
        37, 38, 39, 40, 0,  //  41 - 45
        41, 42, 43, 44, 45, //  46 - 50
        0                   // 0 at end
    },
    {
        // se
        0,                  // 0 at start
        7, 8, 9, 10, 0,     // 01 - 05
        11, 12, 13, 14, 15, // 06 - 10
        17, 18, 19, 20, 0,  // 11 - 15
        21, 22, 23, 24, 25, // 16 - 20
        27, 28, 29, 30, 0,  // 21 - 25
        31, 32, 33, 34, 35, // 26 - 30
        37, 38, 39, 40, 0,  // 31 - 35
        41, 42, 43, 44, 45, // 36 - 40
        47, 48, 49, 50, 0,  // 41 - 45
        0, 0, 0, 0, 0,      // 46 - 50
        0                   // 0 at end
    },
    {
        // sw
        0,                  // 0 at start
        6, 7, 8, 9, 10,     // 01 - 05
        0, 11, 12, 13, 14,  // 06 - 10
        16, 17, 18, 19, 20, // 11 - 15
        0, 21, 22, 23, 24,  // 16 - 20
        26, 27, 28, 29, 30, // 21 - 25
        0, 31, 32, 33, 34,  // 26 - 30
        36, 37, 38, 39, 40, // 31 - 35
        0, 41, 42, 43, 44,  // 36 - 40
        46, 47, 48, 49, 50, // 41 - 45
        0, 0, 0, 0, 0,      // 46 - 50
        0                   // 0 at end
    },
};

/**********************************\
 ==================================

    Draughts board

 ==================================
\**********************************/

/*
     White pawns

  0  0 0 0 0 0 0 0 0
  1  0 0 0 0 0 0 0 0
  2  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 1
  4  1 1 1 1 1 1 1 1
  5  1 1 1 1 1 1 1 1
  6  1 1 1 0 0 0 0 0
  7  0 0 0 0 0 0 0 0

     0 1 2 3 4 5 6 7

     Bitboard: 2251797666201600d

     White kings

  0  0 0 0 0 0 0 0 0
  1  0 0 0 0 0 0 0 0
  2  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0
  7  0 0 0 0 0 0 0 0

     0 1 2 3 4 5 6 7

     Bitboard: 0d

     Black pawns

  0  0 1 1 1 1 1 1 1
  1  1 1 1 1 1 1 1 1
  2  1 1 1 1 1 0 0 0
  3  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0
  7  0 0 0 0 0 0 0 0

     0 1 2 3 4 5 6 7

     Bitboard: 2097150d

     Black kings

  0  0 0 0 0 0 0 0 0
  1  0 0 0 0 0 0 0 0
  2  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0
  7  0 0 0 0 0 0 0 0

     0 1 2 3 4 5 6 7

     Bitboard: 0d

     White occupancies

  0  0 0 0 0 0 0 0 0
  1  0 0 0 0 0 0 0 0
  2  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 1
  4  1 1 1 1 1 1 1 1
  5  1 1 1 1 1 1 1 1
  6  1 1 1 0 0 0 0 0
  7  0 0 0 0 0 0 0 0

     0 1 2 3 4 5 6 7

     Bitboard: 2251797666201600d

     Black occupancies

  0  0 1 1 1 1 1 1 1
  1  1 1 1 1 1 1 1 1
  2  1 1 1 1 1 0 0 0
  3  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0
  7  0 0 0 0 0 0 0 0

     0 1 2 3 4 5 6 7

     Bitboard: 2097150d

     Both occupancies

  0  0 1 1 1 1 1 1 1
  1  1 1 1 1 1 1 1 1
  2  1 1 1 1 1 0 0 0
  3  0 0 0 0 0 0 0 1
  4  1 1 1 1 1 1 1 1
  5  1 1 1 1 1 1 1 1
  6  1 1 1 0 0 0 0 0
  7  0 0 0 0 0 0 0 0

     0 1 2 3 4 5 6 7

     Bitboard: 2251797668298750d

     All squares
  0  0 1 1 1 1 1 1 1
  1  1 1 1 1 1 1 1 1
  2  1 1 1 1 1 1 1 1
  3  1 1 1 1 1 1 1 1
  4  1 1 1 1 1 1 1 1
  5  1 1 1 1 1 1 1 1
  6  1 1 1 0 0 0 0 0
  7  0 0 0 0 0 0 0 0

     0 1 2 3 4 5 6 7

     Bitboard: 6172840429334713770d
*/

// piece bitboards
U64 bitboards[4];

// occupancy bitboards
U64 occupancies[3];

// board filled with all active squares
U64 all_squares = 0ULL;

// "almost" unique position identifier aka hash key or position key
U64 hash_key;

// table with hashkeys for every position 1 to check for repetition
game_t game_keys[1];

// side to move
int side;

// fifty moves counter
int fifty;

// directions on the board for every square => 0 is outside the board
U64 bb_dir[4][52];

// set occupancies
void set_occupancies()
{
    occupancies[white] = bitboards[wPawn] | bitboards[wKing];
    occupancies[black] = bitboards[bPawn] | bitboards[bKing];
    occupancies[both] = occupancies[white] | occupancies[black];
}

// Print the board
void printBoard()
{
#ifndef NDEBUG // print only in debug mode
    printf("   ");
    for (int c = 0; c < SIZE; c++)
        printf(" %d ", c);
    printf("\n");

    for (int r = 0; r < SIZE; r++)
    {
        printf(" %d ", r);
        for (int c = 0; c < SIZE; c++)
        {
            int sq = squares[r][c];
            if (sq == 0)
                printf("   ");
            else if (get_bit(bitboards[wPawn], sq))
                printf(" w ");
            else if (get_bit(bitboards[bPawn], sq))
                printf(" b ");
            else if (get_bit(bitboards[wKing], sq))
                printf(" W ");
            else if (get_bit(bitboards[bKing], sq))
                printf(" B ");
            else
                printf(" . ");
        }
        printf("\n");
    }
#endif
}

// preserve board state
#define copy_board()                                           \
    U64 bitboards_copy[4], occupancies_copy[3], hash_key_copy; \
    int side_copy, fifty_copy;                                 \
    memcpy(bitboards_copy, bitboards, 32);                     \
    memcpy(occupancies_copy, occupancies, 24);                 \
    hash_key_copy = hash_key;                                  \
    fifty_copy = fifty;                                        \
    side_copy = side;

// restore board state
#define take_back()                            \
    memcpy(bitboards, bitboards_copy, 32);     \
    memcpy(occupancies, occupancies_copy, 24); \
    hash_key = hash_key_copy;                  \
    side = side_copy;                          \
    fifty = fifty_copy;

/**********************************\
 ==================================

    Tools

 ==================================
\**********************************/

// get time in milliseconds
int get_time_ms()
{
    struct timeval time_value;
    gettimeofday(&time_value, NULL);
    return time_value.tv_sec * 1000 + time_value.tv_usec / 1000;
}

// integer to str
void intToStr(int N, char *str)
{
    int i = 0;

    // Save the copy of the number for sign
    int sign = N;

    // If the number is negative, make it positive
    if (N < 0)
        N = -N;

    // Extract digits from the number and add them to the
    // string
    while (N > 0)
    {

        // Convert integer digit to character and store
        // it in the str
        str[i++] = N % 10 + '0';
        N /= 10;
    }

    // If the number was negative, add a minus sign to the
    // string
    if (sign < 0)
    {
        str[i++] = '-';
    }

    // Null-terminate the string
    str[i] = '\0';

    // Reverse the string to get the correct order
    for (int j = 0, k = i - 1; j < k; j++, k--)
    {
        char temp = str[j];
        str[j] = str[k];
        str[k] = temp;
    }
}

// concatenate 2 strings
char *concat(const char *s1, const char *s2)
{
    char *result = malloc(strlen(s1) + strlen(s2) + 1); // +1 for the null-terminator
    strcpy(result, s1);
    strcat(result, s2);
    return result;
}

/**********************************\
 ==================================

    Bitboards

 ==================================
\**********************************/

// count bits within a bitboard (Brian Kernighan's way)
static inline int count_bits(U64 bitboard)
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
static inline int get_ls1b_index(U64 bitboard)
{
    // make sure bitboard is not 0
    if (bitboard)
    {
        // count trailing bits before LS1B
        return count_bits((bitboard & -bitboard) - 1);
    }
    // otherwise
    else
        // return illegal index
        return -1;
}

// print bitboard
void print_bitboard(U64 bitboard)
{
#ifndef NDEBUG // print only in debug mode
    // print offset
    printf("\n");

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
                printf("  %d ", rank);

            // print bit state (either 1 or 0)
            printf(" %d", get_bit(bitboard, square) ? 1 : 0);
        }

        // print new line every rank
        printf("\n");
    }

    // print board files
    printf("\n     0 1 2 3 4 5 6 7\n\n");

    // print bitboard as unsigned decimal number
    printf("     Bitboard: %llud\n\n", bitboard);
#endif
}

/**********************************\
 ==================================

           Random numbers

 ==================================
\**********************************/

// pseudo random number state
unsigned int random_state = 1804289383;

// generate 32-bit pseudo legal numbers
unsigned int get_random_U32_number()
{
    // get current state
    unsigned int number = random_state;

    // XOR shift algorithm
    number ^= number << 13;
    number ^= number >> 17;
    number ^= number << 5;

    // update random number state
    random_state = number;

    // return random number
    return number;
}

// generate 64-bit pseudo legal numbers
U64 get_random_U64_number()
{
    // define 4 random numbers
    U64 n1, n2, n3, n4;

    // init random numbers slicing 16 bits from MS1B side
    n1 = (U64)(get_random_U32_number()) & 0xFFFF;
    n2 = (U64)(get_random_U32_number()) & 0xFFFF;
    n3 = (U64)(get_random_U32_number()) & 0xFFFF;
    n4 = (U64)(get_random_U32_number()) & 0xFFFF;

    // return random number
    return n1 | (n2 << 16) | (n3 << 32) | (n4 << 48);
}

// generate magic number candidate
U64 generate_magic_number()
{
    return get_random_U64_number() & get_random_U64_number() & get_random_U64_number();
}

/**********************************\
 ==================================

            Zobrist keys

 ==================================
\**********************************/

// random piece keys [piece][square]
U64 piece_keys[4][51];

// random side key
U64 side_key;

// init random hash keys
void init_random_keys()
{
    // update pseudo random number state
    random_state = 1804289383;

    // loop over piece codes
    for (int piece = wPawn; piece <= bKing; piece++)
    {
        // loop over board squares
        for (int square = 1; square <= 50; square++)
            // init random piece keys
            piece_keys[piece][square] = get_random_U64_number();
    }

    // init random side key
    side_key = get_random_U64_number();
}

// generate "almost" unique position ID aka hash key from scratch
U64 generate_hash_key()
{
    // final hash key
    U64 final_key = 0ULL;

    // temp piece bitboard copy
    U64 bitboard;

    // loop over piece bitboards
    for (int piece = wPawn; piece <= bKing; piece++)
    {
        // init piece bitboard copy
        bitboard = bitboards[piece];

        // loop over the pieces within a bitboard
        while (bitboard)
        {
            // init square occupied by the piece
            int square = get_ls1b_index(bitboard);

            // hash piece
            final_key ^= piece_keys[piece][square];

            // pop LS1B
            pop_bit(bitboard, square);
        }
    }

    // hash the side only if black is to move
    if (side == black)
        final_key ^= side_key;

    // return generated hash key
    return final_key;
}

/**********************************\
 ==================================

    Fen

 ==================================
\**********************************/

// wrong fenstring
void wrong_fenstr(char *fenstr)
{
    printf("Wrong fenstr: %s", fenstr);
}

// fill the bitboards from the fenstring
void set_game_bitboards(int color, int king, int sq)
{
    if (color == white)
    {
        if (king)
            set_bit(bitboards[wKing], sq);
        else
            set_bit(bitboards[wPawn], sq);
    }
    else
    {
        if (king)
            set_bit(bitboards[bKing], sq);
        else
            set_bit(bitboards[bPawn], sq);
    }
}

// read a fenstring
void read_fen(char *fenstr)
{
    memset(bitboards, 0, sizeof(bitboards));
    memset(occupancies, 0, sizeof(occupancies));

    char number[3] = "\0\0";
    int first = 1;
    int king = 0;
    int color = white;
    while (*fenstr != '\0')
    {
        if (first)
        {
            if (*fenstr == 'W')
                side = white;
            else if (*fenstr == 'B')
                side = black;
            else
            {
                wrong_fenstr(fenstr);
                break;
            }
            first = 0;
            ++fenstr;
        }

        if (*fenstr >= '0' && *fenstr <= '9')
        {
            if (number[0] == '\0')
                number[0] = *fenstr;
            else
                number[1] = *fenstr;
        }
        else if (*fenstr == 'K')
        {
            king = 1;
        }
        else if (*fenstr == ':')
        {
            if (number[0] != '\0')
            {
                int sq = atoi(number);
                if (sq < 1 || sq > 50)
                {
                    wrong_fenstr(fenstr);
                    break;
                }
                set_game_bitboards(color, king, sq);
                king = 0;
                number[0] = '\0';
                number[1] = '\0';
            }
            ++fenstr;
            if (*fenstr == 'W')
                color = white;
            else if (*fenstr == 'B')
                color = black;
            else
            {
                wrong_fenstr(fenstr);
                break;
            }
        }
        else if (*fenstr == ',')
        {
            if (number[0] == '\0')
            {
                wrong_fenstr(fenstr);
                break;
            }
            int sq = atoi(number);
            if (sq < 1 || sq > 50)
            {
                wrong_fenstr(fenstr);
                break;
            }
            set_game_bitboards(color, king, sq);
            king = 0;
            number[0] = '\0';
            number[1] = '\0';
        }
        ++fenstr;
    }
    if (number[0] != '\0')
    {
        int sq = atoi(number);
        if (sq < 1 || sq > 50)
        {
            wrong_fenstr(fenstr);
            return;
        }
        set_game_bitboards(color, king, sq);
        king = 0;
        number[0] = '\0';
        number[1] = '\0';
    }

    set_occupancies();
    hash_key = generate_hash_key();
}

// write a fenstr form the current position
void write_fen()
{
#ifndef NDEBUG // print only in debug mode
    printf("fen: ");
    if (side == white)
        printf("W:W");
    else
        printf("B:W");

    U64 bb = occupancies[white];
    int max = count_bits(bb);
    int cnt = 0;
    char num[3];

    while (bb)
    {
        int sq = get_ls1b_index(bb);
        intToStr(sq, num);
        ++cnt;
        if (get_bit(bitboards[wKing], sq))
            printf("K");
        if (cnt < max)
            printf("%s,", num);
        else
            printf("%s:B", num);
        pop_bit(bb, sq);
    }

    bb = occupancies[black];
    max = count_bits(bb);
    cnt = 0;

    while (bb)
    {
        int sq = get_ls1b_index(bb);
        intToStr(sq, num);
        ++cnt;
        if (get_bit(bitboards[bKing], sq))
            printf("K");
        if (cnt < max)
            printf("%s,", num);
        else
            printf("%s", num);
        pop_bit(bb, sq);
    }

    printf("\n");
#endif
}

/**********************************\
 ==================================

    Init Board

 ==================================
\**********************************/

// Initialize the board with starting positions
void init_board()
{
    read_fen("W:W31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50:B1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20");

    all_squares = 0ULL;
    for (int i = 1; i <= 50; ++i)
        set_bit(all_squares, i);

    for (int i = 0; i < 4; ++i)
        print_bitboard(bitboards[i]);
    for (int i = 0; i < 3; ++i)
        print_bitboard(occupancies[i]);
    print_bitboard(all_squares);

    // initialize random keys
    init_random_keys();
    // reset 50 moves counter
    fifty = 0;
}

/**********************************\
 ==================================

    Move generator

 ==================================
\**********************************/

// encode move
#define encode_move(source, target, capture) \
    ((U64)(capture) |                             \
     ((U64)(source) << 52) |                      \
     ((U64)(target) << 58))

// extract source square
#define get_move_source(move) (((move) >> 52) & 63)

// extract target square
#define get_move_target(move) (((move) >> 58) & 63)

// extract capture
#define get_move_capture(move) (((move) & all_squares))

// initialize bitboards dir
void init_bb_dir()
{
    memset(bb_dir, 0ULL, sizeof(bb_dir));

    for (int d = nw; d <= se; ++d)
        for (int sq = 1; sq <= 50; ++sq)
            if (dir[d][sq] == 0)
                continue;
            else
                set_bit(bb_dir[d][sq], dir[d][sq]);
}

// generate next captures
void generate_next_captures(moves_t *move_list)
{
    moves_t old_list[1];

    memcpy(old_list, move_list, sizeof(moves_t));
    memset(move_list, 0, sizeof(moves_t));

    U64 capture = side == white ? occupancies[black] : occupancies[white];
    U64 empty = ~occupancies[both] & all_squares;
    int sqf, sqt, sqo, piece;

    U64 b, e, cap;

    for (int i = 0; i < old_list->counter; ++i)
    {
        U64 m = old_list->moves[i];
        sqf = get_move_source(m);
        sqt = get_move_target(m);
        cap = get_move_capture(m);
        if (get_bit(bitboards[wPawn], sqf))
            piece = wPawn;
        else if (get_bit(bitboards[bPawn], sqf))
            piece = bPawn;
        else if (get_bit(bitboards[wKing], sqf))
            piece = wKing;
        else if (get_bit(bitboards[bKing], sqf))
            piece = bKing;
        sqf = sqt;
        if (piece == wPawn || piece == bPawn)
        {
            for (int d = nw; d <= se; ++d)
            {
                b = capture & bb_dir[d][sqf];
                if (b)
                {
                    sqo = get_ls1b_index(b);
                    if (get_bit(cap, sqo)) // can't jump over a piece a second time
                        continue;
                    e = empty & bb_dir[d][sqo];
                    if (e) // next capture found
                    {
                        sqt = get_ls1b_index(e);
                        U64 newcap = cap;
                        set_bit(newcap, sqo);
                        move_list->moves[move_list->counter] = encode_move(sqf, sqt, newcap);
                        ++move_list->counter;
                        move_list->caplength = old_list->caplength + 1;
                    }
                }
            }
        }
        else
        {
            // white or black king
            for (int d = nw; d <= se; ++d)
            {
                e = empty & bb_dir[d][sqf];
                b = capture & bb_dir[d][sqf];
                while (e)
                {
                    sqo = get_ls1b_index(e);
                    e = empty & bb_dir[d][sqo];
                    b = capture & bb_dir[d][sqo];
                }
                if (b)
                {
                    sqo = get_ls1b_index(b);
                    if (get_bit(cap, sqo)) // can't jump over a piece a second time
                        continue;
                    e = empty & bb_dir[d][sqo];
                    while (e) // next capture(s) found
                    {
                        sqt = get_ls1b_index(e);
                        U64 newcap = cap;
                        set_bit(newcap, sqo);
                        move_list->moves[move_list->counter] = encode_move(sqf, sqt, newcap);
                        ++move_list->counter;
                        move_list->caplength = old_list->caplength + 1;
                        e = empty & bb_dir[d][sqt];
                    }
                }
            }
        }
    }

    if (move_list->counter > 0)
        generate_next_captures(move_list);
    else
        memcpy(move_list, old_list, sizeof(moves_t));
}

// generate moves
void generate_moves(moves_t *movelist)
{
    memset(movelist, 0, sizeof(moves_t));

    U64 capture = side == white ? occupancies[black] : occupancies[white];
    U64 empty = ~occupancies[both] & all_squares;
    U64 pawns = side == white ? bitboards[wPawn] : bitboards[bPawn];
    U64 kings = side == white ? bitboards[wKing] : bitboards[bKing];
    int dirP1 = side == white ? nw : sw;
    int dirP2 = side == white ? ne : se;
    int sqf, sqt, sqo;

    U64 b, e, cap;

    while (pawns)
    {
        sqf = get_ls1b_index(pawns);
        for (int d = nw; d <= se; ++d)
        {
            b = capture & bb_dir[d][sqf];
            if (b)
            {
                sqo = get_ls1b_index(b);
                e = empty & bb_dir[d][sqo];
                if (e) // capture found
                {
                    sqt = get_ls1b_index(e);
                    cap = 0ULL;
                    set_bit(cap, sqo);
                    if (movelist->caplength == 0)
                        movelist->counter = 0;
                    movelist->moves[movelist->counter] = encode_move(sqf, sqt, cap);
                    ++movelist->counter;
                    movelist->caplength = 1;
                }
            }
            else if (d == dirP1 || d == dirP2)
            {
                e = empty & bb_dir[d][sqf];
                if (e && movelist->caplength == 0)
                {
                    sqt = get_ls1b_index(e);
                    movelist->moves[movelist->counter] = encode_move(sqf, sqt, 0ULL);
                    ++movelist->counter;
                }
            }
        }
        pop_bit(pawns, sqf);
    }

    while (kings)
    {
        sqf = get_ls1b_index(kings);
        for (int d = nw; d <= se; ++d)
        {
            e = empty & bb_dir[d][sqf];
            b = capture & bb_dir[d][sqf];
            while (e)
            {
                sqo = get_ls1b_index(e);
                if (movelist->caplength == 0)
                {
                    movelist->moves[movelist->counter] = encode_move(sqf, sqo, 0ULL);
                    ++movelist->counter;
                }
                e = empty & bb_dir[d][sqo];
                b = capture & bb_dir[d][sqo];
            }
            if (b)
            {
                sqo = get_ls1b_index(b);
                e = empty & bb_dir[d][sqo];
                while (e) // capture(s) found
                {
                    sqt = get_ls1b_index(e);
                    cap = 0ULL;
                    set_bit(cap, sqo);
                    if (movelist->caplength == 0)
                        movelist->counter = 0;
                    movelist->moves[movelist->counter] = encode_move(sqf, sqt, cap);
                    ++movelist->counter;
                    movelist->caplength = 1;
                    e = empty & bb_dir[d][sqt];
                }
            }
        }
        pop_bit(kings, sqf);
    }

    if (movelist->caplength == 1)
        generate_next_captures(movelist);
}

// print_move
void print_move(U64 mov)
{
    int sqf = get_move_source(mov);
    int sqt = get_move_target(mov);
    U64 cap = get_move_capture(mov);

    char *mvp = (cap) ? " x " : " - ";
#ifndef NDEBUG // print only in debug mode
    printf(" (%s%s%s)", squarenumber[sqf], mvp, squarenumber[sqt]);
#endif
}

// print_movelist
void print_movelist(moves_t *movelist)
{
#ifndef NDEBUG // print only in debug mode
    printf("\nmoves: %d =>", movelist->counter);
    for (int i = 0; i < movelist->counter; ++i)
        print_move(movelist->moves[i]);
    printf("\n");
#endif
}

// make a move
int make_move(U64 move, int move_type)
{
    int sqf = get_move_source(move);
    int sqt = get_move_target(move);
    U64 cap = get_move_capture(move);
    int piece = no_piece;
    int xside = no_piece;
    int result = 1;

    if (move_type == all_moves)
    {
        if (get_bit(bitboards[wPawn], sqf))
            piece = wPawn;
        else if (get_bit(bitboards[bPawn], sqf))
            piece = bPawn;
        else if (get_bit(bitboards[wKing], sqf))
            piece = wKing;
        else if (get_bit(bitboards[bKing], sqf))
            piece = bKing;

        // move
        pop_bit(bitboards[piece], sqf);
        set_bit(bitboards[piece], sqt);
        ++fifty;

        // capture
        if (cap > 0ULL)
        {
            fifty = 0; // reset fifty moves counter
            U64 c = cap;
            while (c)
            {
                int sqo = get_ls1b_index(c);
                if (get_bit(bitboards[wPawn], sqo))
                    xside = wPawn;
                else if (get_bit(bitboards[bPawn], sqo))
                    xside = bPawn;
                else if (get_bit(bitboards[wKing], sqo))
                    xside = wKing;
                else if (get_bit(bitboards[bKing], sqo))
                    xside = bKing;

                pop_bit(bitboards[xside], sqo);
                pop_bit(c, sqo);
            }
        }

        // Promotion
        if (piece == bPawn && square_rank[sqt] == SIZE)
        {
            pop_bit(bitboards[bPawn], sqt);
            set_bit(bitboards[bKing], sqt);
            fifty = 0; // reset fifty moves counter
        }
        if (piece == wPawn && square_rank[sqt] == 1)
        {
            pop_bit(bitboards[wPawn], sqt);
            set_bit(bitboards[wKing], sqt);
            fifty = 0; // reset fifty moves counter
        }

        set_occupancies();
        side ^= 1;
        hash_key = generate_hash_key();
    }
    else
    {
        if (cap > 0ULL)
            result = make_move(move, all_moves);
        else
            result = 0;
    }
    return result;
}

/**********************************\
 ==================================

    Evaluation

 ==================================
\**********************************/

// piece score table
const int PST_P[51] =
    {0,                  // 0 at start
     0, 0, 0, 0, 0,      //  01 - 05   piece promotion line
     45, 50, 55, 50, 45, //  06 - 10
     40, 45, 50, 45, 40, //  11 - 15
     35, 40, 45, 40, 35, //  16 - 20
     25, 30, 30, 25, 30, //  21 - 25   small threshold to prevent to optimistic behaviour
     25, 30, 35, 30, 25, //  26 - 30
     20, 15, 25, 20, 25, //  31 - 35
     20, 15, 25, 20, 15, //  36 - 40
     10, 15, 25, 20, 15, //  41 - 45
     5, 10, 15, 10, 5};  //  46 - 50

const int PST_K[51] =
    {0,                        // 0 at start
     050, 050, 050, 050, 050,  //  01 - 05
     050, 050, 050, 050, 050,  //  06 - 10
     050, 050, 050, 050, 050,  //  11 - 15
     050, 050, 050, 050, 050,  //  16 - 20
     050, 050, 050, 050, 050,  //  21 - 25
     050, 050, 050, 050, 050,  //  26 - 30
     050, 050, 050, 050, 050,  //  31 - 35
     050, 050, 050, 050, 050,  //  36 - 40
     050, 050, 050, 050, 050,  //  41 - 45
     050, 050, 050, 050, 050}; //  46 - 50

// evaluate the position on the board
static inline int evaluate()
{
    int score = 0;
    U64 bb;

    // white
    bb = occupancies[white];
    if (bb == 0ULL)
        score -= 9000;
    else
    {
        while (bb)
        {
            int sq = get_ls1b_index(bb);
            if (get_bit(bitboards[wPawn], sq))
                score += 1000 + PST_P[sq];
            else if (get_bit(bitboards[wKing], sq))
                score += 3000 + PST_K[sq];
            pop_bit(bb, sq);
        }
    }

    // black
    bb = occupancies[black];
    if (bb == 0ULL)
        score += 9000;
    else
    {
        while (bb)
        {
            int sq = get_ls1b_index(bb);
            if (get_bit(bitboards[bPawn], sq))
                score -= (1000 + PST_P[51 - sq]);
            else if (get_bit(bitboards[bKing], sq))
                score -= (3000 + PST_K[51 - sq]);
            pop_bit(bb, sq);
        }
    }

    return (side == white) ? score : -score;
}

/**********************************\
 ==================================

    Search

 ==================================
\**********************************/

// number of nodes searched
U64 nodes = 0ULL;

// half moves counter
int ply = 0;

// PV length [ply]
int pv_length[MAX_PLY];

// PV table [ply][ply]
U64 pv_table[MAX_PLY][MAX_PLY];

// follow PV & score PV move
int follow_pv, score_pv;

// timer
// flag if the clock is pressed
int press_clock;

// to handle the used time in seconds
int timer[2];

// add time after each move in seconds
int plustimer[2];

// for drawing the time on the clock
int clocktime[2][5];

// start the clock for the player to move
int starttimer[2];

// thinktime per move for the player to move
int thinktimer[2];

// number of moves to go for calculation the move time
int movestogo = 0;

// thinktime in ms
int ucitime = 0;

// flag that indicates that the timer is on
static int timeset = 0;

// stating thinktime in ms
static int starttime = 0;

// stopping thinktime in ms
static int stoptime = 0;

// flag that forces the thinking to stop
static int stopped = 0;

// falg that indicates that the game is stopped
static int stop_game_flag = 0;

// evalaation score
static int score = 0;

/**********************************\
 ==================================

        Transposition table

 ==================================
\**********************************/

// number hash table entries
int hash_entries = 0;

// define TT instance
tt *hash_table = NULL;

// clear TT (hash table)
void clear_hash_table()
{
    // init hash table entry pointer
    tt *hash_entry;

    // loop over TT elements
    for (hash_entry = hash_table; hash_entry < hash_table + hash_entries; hash_entry++)
    {
        // reset TT inner fields
        hash_entry->hash_key = 0;
        hash_entry->depth = 0;
        hash_entry->flag = 0;
        hash_entry->score = 0;
    }
}

// dynamically allocate memory for hash table
void init_hash_table(int mb)
{
    // init hash size
    int hash_size = 0x100000 * mb;

    // init number of hash entries
    hash_entries = hash_size / sizeof(tt);

    // free hash table if not empty
    if (hash_table != NULL)
    {
#ifndef NDEBUG // print only in debug mode
        printf("\n    Clearing hash memory...\n");
#endif
        // free hash table dynamic memory
        free(hash_table);
    }

    // allocate memory
    hash_table = (tt *)malloc(hash_entries * sizeof(tt));

    // if allocation has failed
    if (hash_table == NULL)
    {
#ifndef NDEBUG // print only in debug mode
        printf("   Couldn't allocate memory for hash table, tryinr %dMB...", mb / 2);
#endif

        // try to allocate with half size
        init_hash_table(mb / 2);
    }

    // if allocation succeeded
    else
    {
        // clear hash table
        clear_hash_table();
#ifndef NDEBUG // print only in debug mode
        printf("   Hash table is initialied with %d entries\n", hash_entries);
#endif
    }
}

// read hash entry data
static inline int read_hash_entry(int alpha, int beta, int depth)
{
    // create a TT instance pointer to particular hash entry storing
    // the scoring data for the current board position if available
    tt *hash_entry = &hash_table[hash_key % hash_entries];

    // make sure we're dealing with the exact position we need
    if (hash_entry->hash_key == hash_key)
    {
        // make sure that we match the exact depth our search is now at
        if (hash_entry->depth >= depth)
        {
            // extract stored score from TT entry
            int score = hash_entry->score;

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
static inline void write_hash_entry(int score, int depth, int hash_flag)
{
    // create a TT instance pointer to particular hash entry storing
    // the scoring data for the current board position if available
    tt *hash_entry = &hash_table[hash_key % hash_entries];

    // write hash entry data
    hash_entry->hash_key = hash_key;
    hash_entry->score = score;
    hash_entry->flag = hash_flag;
    hash_entry->depth = depth;
}

// enable PV move scoring
static inline void enable_pv_scoring(moves_t *move_list)
{
    // disable following PV
    follow_pv = 0;

    // loop over the moves within a move list
    for (int count = 0; count < move_list->counter; ++count)
    {
        // make sure we hit PV move
        if (pv_table[0][ply] == move_list->moves[count])
        {
            // enable move scoring
            score_pv = 1;

            // enable following PV
            follow_pv = 1;
        }
    }
}

// score form a move
static inline int score_move(U64 move)
{
    // if PV move scoring is allowed
    if (score_pv)
    {
        // make sure we are dealing with PV move
        if (pv_table[0][ply] == move)
        {
            // disable score PV flag
            score_pv = 0;

            // give PV move the highest score to search it first
            return 20000;
        }
    }

    if (get_move_capture(move) > 0ULL)
    {
        return 10000 + count_bits(get_move_capture(move));
    }
    return GetRandomValue(0, 1000);
}

// sort moves in descending order
static inline int sort_moves(moves_t *move_list)
{
    // move scores
    int move_scores[move_list->counter];

    // score all the moves within a move list
    for (int count = 0; count < move_list->counter; ++count)
        // score move
        move_scores[count] = score_move(move_list->moves[count]);

    // loop over current move within a move list
    for (int current_move = 0; current_move < move_list->counter; ++current_move)
    {
        // loop over next move within a move list
        for (int next_move = current_move + 1; next_move < move_list->counter; ++next_move)
        {
            // compare current and next move scores
            if (move_scores[current_move] < move_scores[next_move])
            {
                // swap scores
                int temp_score = move_scores[current_move];
                move_scores[current_move] = move_scores[next_move];
                move_scores[next_move] = temp_score;

                // swap moves
                U64 temp_move = move_list->moves[current_move];
                move_list->moves[current_move] = move_list->moves[next_move];
                move_list->moves[next_move] = temp_move;
            }
        }
    }
    return 0;
}

// quiescence search. only captures
static inline int quiescence(int alpha, int beta)
{
    // every stop_thinking flag nodes
    if ((nodes & 2047) == 0)
    {
        if ((timeset == 1 && get_time_ms() > stoptime) || stop_game_flag)
        {
            // tell engine to stop calculating
            stopped = 1;
        }
    }

    // increment nodes count
    ++nodes;

    // we are too deep, hence there's an overflow of arrays relying on max ply constant
    if (ply > MAX_PLY - 1)
        // evaluate position
        return evaluate();

    // evaluate position
    int evaluation = evaluate();

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
    generate_moves(move_list);

    // sort moves
    sort_moves(move_list);

    // loop over moves within a movelist
    for (int count = 0; count < move_list->counter; ++count)
    {
        // preserve board state
        copy_board();

        // increment ply
        ++ply;

        // make sure to make only legal moves
        if (make_move(move_list->moves[count], only_captures) == 0)
        {
            // decrement ply
            --ply;

            // skip to next move
            continue;
        }

        // score current move, recursive call
        int score = -quiescence(-beta, -alpha);

        // decrement ply
        --ply;

        // take move back
        take_back();

        // return 0 if time is up
        if (stopped == 1)
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

// negamax alpha beta search
static inline int negamax(int alpha, int beta, int depth)
{
    // init PV length
    pv_length[ply] = ply;

    // variable to store current move's score (from the static evaluation perspective)
    int score = 0;

    // define hash flag
    int hash_flag = hash_flag_alpha;

    // a hack by Pedro Castro to figure out whether the current node is PV node or not
    int pv_node = beta - alpha > 1;

    // read hash entry if we're not in a root ply and hash entry is available
    // and current node is not a PV node
    if (ply && (score = read_hash_entry(alpha, beta, depth)) != no_hash_entry && pv_node == 0)
    // if the move has already been searched (hence has a value)
    // we just return the score for this move without searching it
    {
#ifndef NDEBUG
        printf("hash_entry found: %d\n", score);
#endif
        return score;
    }

    // every stop thinking flag nodes
    if ((nodes & 2047) == 0)
    {
        if ((timeset == 1 && get_time_ms() > stoptime) || stop_game_flag)
        {
            // tell engine to stop calculating
            stopped = 1;
        }
    }

    // recursion escapre condition
    if (depth == 0)
        // run quiescence search
        return quiescence(alpha, beta);

    // we are too deep, hence there's an overflow of arrays relying on max ply constant
    if (ply > MAX_PLY - 1)
        // evaluate position
        return evaluate();

    // increment nodes count
    ++nodes;

    // legal moves counter
    int legal_moves = 0;

    // create move list instance
    moves_t move_list[1];

    // generate moves
    generate_moves(move_list);

    // if we are now following PV line
    if (follow_pv)
        // enable PV move scoring
        enable_pv_scoring(move_list);

    // sort moves
    sort_moves(move_list);

    // loop over moves within a movelist
    for (int count = 0; count < move_list->counter; ++count)
    {
        // preserve board state
        copy_board();

        // increment ply
        ++ply;

        // make sure to make only legal moves
        if (make_move(move_list->moves[count], all_moves) == 0)
        {
#ifndef NDEBUG
            print_move(move_list->moves[count]);
            printf("Is not valid");
#endif

            // decrement ply
            --ply;

            // skip to next move
            continue;
        }

        // increment legal moves
        ++legal_moves;

        // do normal alpha beta search
        score = -negamax(-beta, -alpha, depth - 1);

        // decrement ply
        --ply;

        // take move back
        take_back();

        // return 0 if time is up
        if (stopped == 1)
            return 0;

        // found a better move
        if (score > alpha)
        {
            // switch hash flag from storing score for fail-low node
            // to the one storing score for PV node
            hash_flag = hash_flag_exact;

            // PV node (position)
            alpha = score;

            // write PV move
            pv_table[ply][ply] = move_list->moves[count];

            // loop over the next ply
            for (int next_ply = ply + 1; next_ply < pv_length[ply + 1]; next_ply++)
                // copy move from deeper ply into a current ply's line
                pv_table[ply][next_ply] = pv_table[ply + 1][next_ply];

            // adjust PV length
            pv_length[ply] = pv_length[ply + 1];

            // fail-hard beta cutoff
            if (score >= beta)
            {
                // store hash entry with the score equal to beta
                write_hash_entry(beta, depth, hash_flag_beta);

                // node (position) fails high
                return beta;
            }
        }
    }

    // we don't have any legal moves to make in the current postion
    if (legal_moves == 0)
        return evaluate();

    write_hash_entry(alpha, depth, hash_flag);

    // node (position) fails low
    return alpha;
}

// search current position
static inline int search_position(int depth)
{
    // reset nodes counter
    nodes = 0;

    // reset "time is up" flag
    stopped = 0;

    // reset follow PV flags
    follow_pv = 0;
    score_pv = 0;

    // reset ply
    ply = 0;

    // clear helper data structures for search
    memset(pv_table, 0, sizeof(pv_table));
    memset(pv_length, 0, sizeof(pv_length));

    // define initial alpha beta bounds
    int alpha = -INFINITY;
    int beta = INFINITY;

    // iterative deepening
    for (int current_depth = 1; current_depth <= depth; current_depth++)
    {
        // if time is up
        if (stopped == 1)
            // stop calculating and return best move so far
            break;

        // enable follow PV flag
        follow_pv = 1;

        // find best move within a given position
        score = negamax(alpha, beta, current_depth);

        // we fell outside the window, so try again with a full-width window (and the same depth)
        if ((score <= alpha) || (score >= beta))
        {
            alpha = -INFINITY;
            beta = INFINITY;
            continue;
        }

        // set up the window for the next iteration
        alpha = score - 50;
        beta = score + 50;

        // if PV is available
        if (pv_length[0])
        {
#ifndef NDEBUG // print only in debug mode
            printf("info score cp %d depth %d nodes %lld time %d pv ", score, current_depth, nodes, get_time_ms() - starttime);
            // loop over the moves within a PV line
            for (int count = 0; count < pv_length[0]; ++count)
            {
                // print PV move
                print_move(pv_table[0][count]);
                printf(" ");
            }
            // print new line
            printf("\n");
#endif
        }
    }

#ifndef NDEBUG // print only in debug mode
    // print best move
    printf("bestmove ");
    print_move(pv_table[0][0]);
    printf(" score: %d\n", score);
#endif

    return 0;
}

// Fill the time for on the chessclock for the color white or black
void fill_clocktime(int col)
{
    int hr = (int)(timer[col] / 3600);
    int rst = timer[col] - hr * 3600;
    int sec = rst % 60;
    int min = (int)((rst - sec) / 60);
    clocktime[col][4] = sec % 10;
    clocktime[col][3] = (int)((sec - clocktime[col][4]) / 10);
    clocktime[col][2] = min % 10;
    clocktime[col][1] = (int)((min - clocktime[col][2]) / 10);
    clocktime[col][0] = hr;
}

/**********************************\
 ==================================

    GUI

 ==================================
\**********************************/

// used for drawing the gui, preventing flashing during thinking
char guiboard[SIZE][SIZE];

// side to move, preventing flashing during thinking
int gui_side;

// game start
int game_state = Game_start;

// end game reason
int game_end_reason = nop;

// winner of the game
int game_winner = both;

// color of the human player
int human_player = both;

// color of the ai plaer
int ai_player = -1;

// flag that indicates if the board is turned
int reversed = 0;

// quick check which of the pieces can move
// when the human player is to move
// square has a green border
U64 move_options = 0ULL;

// quick check which of the squares the selected piece can move to
// when the human player is to move
// square has a blue border
U64 square_options[51];

// quick check which of the pieces can captured
// when the human player is to move
// square has a red border
U64 cap_options[51];

// square from the selected piece
// when the human player is to move
int selected_piece = 99;

// sqaure from the selected square where the selected piece can go to
// when the human player is to move
int selected_square = 99;

// movelist used in the GUI
moves_t gui_movelist[1];

// for showing the last played move
U64 last_move[2];

// count the played moves in the game for white and black
int move_counter[2];

// TASK
// flag that indicates that the thinking task is ready
static int task_ready = 0;

// flag that indicates that the ai is thinking
static int thread_busy = 0;

// game notation
FILE *game_ptr = NULL;

// file for notation
char file_name[20];

// show text in the terminal
void show_text()
{
    // Create a file pointer and open the file "GFG.txt" in
    // read mode.
    FILE *file = fopen("./ned.md", "r");

    // Buffer to store each line of the file.
    char line[256];

    // Check if the file was opened successfully.
    if (file != NULL)
    {
        // Read each line from the file and store it in the 'line' buffer.
        while (fgets(line, sizeof(line), file))
        {
            // Print each line to the standard output.
            printf("%s", line);
        }

        // Close the file stream once all lines have been read.
        fclose(file);
    }
    else
    {
        // Print an error message to the standard error
        // stream if the file cannot be opened.
        fprintf(stderr, "Unable to open file!\n");
    }
}

// fill the gui_board for drawing
void fill_gui_board()
{
    int sq = 0;
    for (int row = 0; row < SIZE; ++row)
        for (int col = 0; col < SIZE; ++col)
            if ((row + col) % 2 == 0)
                guiboard[row][col] = 'x';
            else
            {
                ++sq;
                if (get_bit(bitboards[wPawn], sq))
                    guiboard[row][col] = 'w';
                else if (get_bit(bitboards[bPawn], sq))
                    guiboard[row][col] = 'b';
                else if (get_bit(bitboards[wKing], sq))
                    guiboard[row][col] = 'W';
                else if (get_bit(bitboards[bKing], sq))
                    guiboard[row][col] = 'B';
                else
                    guiboard[row][col] = '.';
            }
}

// fill the options for drawing
void fill_options(moves_t *movelist)
{
    move_options = 0ULL;
    memset(square_options, 0ULL, sizeof(square_options));
    memset(cap_options, 0ULL, sizeof(cap_options));

    for (int index = 0; index < movelist->counter; ++index)
    {
        U64 m = movelist->moves[index];
        int sqf = get_move_source(m);
        int sqt = get_move_target(m);
        U64 cap = get_move_capture(m);
        set_bit(move_options, sqf);
        set_bit(square_options[sqf], sqt);
        cap_options[sqf] |=cap;
    }
}

// get the sqrx
int get_sqrx(int col, int min)
{
    int n = min;
    for (int i = 0; i < SIZE; ++i)
    {
        n += SQUARE_SIZE;
        if (col < n)
            return i;
    }
    return 8;
}

// get the sqry
int get_sqry(int row, int min)
{
    int n = min;
    for (int i = 0; i < SIZE; ++i)
    {
        n += SQUARE_SIZE;
        if (row < n)
            return i;
    }
    return 8;
}

// test if the position on the boaard is repeated 3 times
int test_for_draw_repetition()
{
    int cnt = 0;
    for (int i = 0; i < game_keys->gamelength; ++i)
    {
        if (hash_key == game_keys->hash_keys[i])
            ++cnt;
    }
    if (cnt >= 3)
    {
        game_state = Game_stop;
        game_end_reason = draw3;
        if (game_ptr)
        {
            fprintf(game_ptr, "\n\nremise door herhaling van zetten\n");
            fclose(game_ptr);
            game_ptr = NULL;
        }
        return 1;
    }
    return 0;
}

// process the move
void process_move(const int selected_piece, const int selected_square, moves_t *movelist)
{
    for (int i = 0; i < movelist->counter; ++i)
    {
        if (get_move_source(movelist->moves[i]) == selected_piece && get_move_target(movelist->moves[i]) == selected_square)
        {
            last_move[side] = movelist->moves[i];
            ++move_counter[side];
            char text0[3];
            intToStr(move_counter[side], text0);
            char *text1 = squarenumber[get_move_source(last_move[side])];
            char *text2 = squarenumber[get_move_target(last_move[side])];
            char *text3 = get_move_capture(last_move[side]) ? "x" : "-";
            if (game_ptr != NULL)
            {
                if (side == white)
                    fprintf(game_ptr, "%3s ", text0);
                fprintf(game_ptr, "%s", text1);
                fprintf(game_ptr, "%s", text3);
                fprintf(game_ptr, "%s ", text2);
                if (side == black)
                    fprintf(game_ptr, "\n");
            }
            else
            {
                printf("Could not open file %s\n", file_name);
            }
            make_move(movelist->moves[i], all_moves);
            game_keys->hash_keys[game_keys->gamelength++] = hash_key;
            test_for_draw_repetition();
            break;
        }
    }
    gui_side = side;
    generate_moves(movelist);
    write_fen();
#ifndef NDEBUG // print only in debug mode
    printf("side to move: %d, eval: %d\n", gui_side, evaluate());
#endif
    print_movelist(movelist);
    timer[side ^ 1] += plustimer[side ^ 1];
    press_clock = 1;
}

// Handle the mouse click
void process_mouseclick(const int x, const int y, moves_t *movelist)
{
    if (game_state != Game_play)
        return;
    if (x < BOARD_COL || x > BOARD_COL + BOARD_SIZE || y < BOARD_ROW || y > BOARD_ROW + BOARD_SIZE)
        return;

    int sqrx = get_sqrx(x, BOARD_COL);
    int sqry = get_sqry(y, BOARD_ROW);
    int sqn = 0;
    for (int i = 1; i <= 50; ++i)
    {
        if (rowcol[i][0] == sqry && rowcol[i][1] == sqrx)
        {
            sqn = i;
            break;
        }
    }
    // select square on the board
    int sqr = reversed ? 51 - sqn : sqn;
    if (human_player == gui_side || human_player == both)
    {
        if (selected_piece == 99)
        {
            if (get_bit(move_options, sqr))
            {
                selected_piece = sqr;
#ifndef NDEBUG // print only in debug mode
                printf("\n selectpiece (row %d col %d)\n", rowcol[sqr][0], rowcol[sqr][1]);
#endif
            }
        }
        else
        {
            if (get_bit(square_options[selected_piece], sqr))
            {
                selected_square = sqr;
#ifndef NDEBUG // print only in debug mode
                printf("\n selectsquare (row %d col %d)\n", rowcol[sqr][0], rowcol[sqr][1]);
#endif
                process_move(selected_piece, selected_square, movelist);
                selected_piece = 99;
                selected_square = 99;
                fill_gui_board();
                fill_options(movelist);
            }
            else if (get_bit(move_options, sqr))
            {
                selected_piece = sqr;
#ifndef NDEBUG // print only in debug mode
                printf("\n selectpiece (row %d col %d)\n", rowcol[sqr][0], rowcol[sqr][1]);
#endif
            }
        }
    }
}

// start a new game
void new_game(const int state, moves_t *movelist)
{
    // set timer settings for the clock
    timer[white] = timer[black] = 600;
    plustimer[white] = plustimer[black] = 0;
    thinktimer[white] = thinktimer[black] = 0;
    // fill the clock settings
    fill_clocktime(white);
    fill_clocktime(black);
    press_clock = 1;
    init_board();
    // clear the hash table
    clear_hash_table();
    gui_side = side;
    generate_moves(movelist);
#ifndef NDEBUG
    print_movelist(movelist);
#endif
    fill_gui_board();
    fill_options(movelist);
    memset(last_move, 0, sizeof(U64) * 2);
    move_counter[black] = 0;
    move_counter[white] = 0;
    game_state = state;
}

// calculate the best move for the ai in a seperate thread
void *task(void *arg)
{
    thread_busy = 1;
    task_ready = 0;
    int depth = 64;
    movestogo = Max(50 - move_counter[side], 10);
    ucitime = Max(timer[side] * 1000 / movestogo, 1000);
    if (ucitime > 1500)
        ucitime -= 50;
    timeset = 1;
    starttime = get_time_ms();
    stoptime = starttime + ucitime;
#ifndef NDEBUG // print only in debug mode
    // print debug info
    printf("\n time: %d  start: %u  stop: %u  depth: %d  timeset: %d\n",
           ucitime, starttime, stoptime, depth, timeset);
#endif
    // search position with depth of 64
    int sc = search_position(depth);
#ifndef NDEBUG // print only in debug mode
    printf("task score %d\n", sc);
#endif
    timeset = 0;
    // set the flag so the program knows that the thread is finished
    task_ready = 1;
    return NULL;
}

// open file for game writing
void start_writing(int who, int xwho)
{
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    strftime(file_name, sizeof(file_name) - 1, "./%Y%m%d_%H%M.ntb", t);
    game_ptr = fopen(file_name, "w");
    if (game_ptr == NULL)
        return;
    if (who == human_player)
        fprintf(game_ptr, "human - ");
    else if (who == ai_player)
        fprintf(game_ptr, "ai - ");
    if (xwho == human_player)
        fprintf(game_ptr, "human\n");
    else if (xwho == ai_player)
        fprintf(game_ptr, "ai\n");
    fprintf(game_ptr, "Bedenktijd: %d min %d sec/zet\n--------------------\n", (timer[white] / 60), plustimer[white]);
}

//-----------------------------------------------
// MAIN
//-----------------------------------------------

int main()
{
    init_bb_dir();
    init_hash_table(128);
    new_game(Game_start, gui_movelist);
    int counter = 0;

    if (TEST)
    {
        char *testfen = "W:WK41:B18,28,29,38,43";
        read_fen(testfen);
        generate_moves(gui_movelist);
        fill_gui_board();
        fill_options(gui_movelist);
        human_player = white;
        ai_player = black;
        reversed = 0;
        game_state = Game_play;
    }

    // initialize raylib
    InitWindow(SCREEN_WIDTH, SCREEN_HEIGHT, title);
    // load images
    Texture2D img_empty = LoadTexture("./assets/empty.png");
    img_empty.height = img_empty.width = SQUARE_SIZE;
    Texture2D img_wpawn = LoadTexture("./assets/pwhite.png");
    img_wpawn.height = img_wpawn.width = SQUARE_SIZE;
    Texture2D img_bpawn = LoadTexture("./assets/pblack.png");
    img_bpawn.height = img_bpawn.width = SQUARE_SIZE;
    Texture2D img_wking = LoadTexture("./assets/kwhite.png");
    img_wking.height = img_wking.width = SQUARE_SIZE;
    Texture2D img_bking = LoadTexture("./assets/kblack.png");
    img_bking.height = img_bking.width = SQUARE_SIZE;
    Texture2D img_clock = LoadTexture("./assets/Clock.png");
    img_clock.height = SQUARE_SIZE;
    img_clock.width = SQUARE_SIZE * 2 + HALF_SQUARE_SIZE;
    Texture2D img_back = LoadTexture("./assets/back.png");
    img_back.height = img_back.width = BOARD_SIZE;
    Texture2D img_choice = LoadTexture("./assets/Choice.png");
    img_choice.height = img_choice.width = SQUARE_SIZE;

    // thread to run
    pthread_t thread;

    // set frames per second
    SetTargetFPS(10);

    // mainloop
    while (!WindowShouldClose())
    {
        // update
        ++counter;
        if (press_clock)
        {
            starttimer[gui_side] = get_time_ms();
            press_clock = 0;
        }
        if (fifty >= 100)
        {
            game_winner = both;
            game_state = Game_stop;
            game_end_reason = draw50;
            if (game_ptr)
            {
                fprintf(game_ptr, "\n\nremise door de 50 zetten regel\n");
                fclose(game_ptr);
                game_ptr = NULL;
            }
        }
        if (gui_movelist->counter == 0)
        {
            if (side == white)
            {
                game_winner = black;
                game_state = Game_stop;
                game_end_reason = wMove;
                if (game_ptr)
                {
                    fprintf(game_ptr, "\n\nzwart wint doordat wit niet meer kan zetten\n");
                    fclose(game_ptr);
                    game_ptr = NULL;
                }
            }
            else
            {
                game_winner = white;
                game_state = Game_stop;
                game_end_reason = bMove;
                if (game_ptr)
                {
                    fprintf(game_ptr, "\n\nwit wint doordat zwart niet meer kan zetten\n");
                    fclose(game_ptr);
                    game_ptr = NULL;
                }
            }
        }
        if (game_state == Game_play)
        {
            int changed = 0;
            // time is up?
            if (timer[gui_side] == 0)
            {
                game_state = Game_stop;
                game_winner = gui_side ^ 1;
                if (game_winner == black)
                {
                    game_end_reason = wTime;
                    if (game_ptr)
                    {
                        fprintf(game_ptr, "\n\nzwart wint doordat wit geen tijd meer heeft\n");
                        fclose(game_ptr);
                        game_ptr = NULL;
                    }
                }
                else
                {
                    game_end_reason = bTime;
                    if (game_ptr)
                    {
                        fprintf(game_ptr, "\n\nwit wint doordat zwart geen tijd meer heeft\n");
                        fclose(game_ptr);
                        game_ptr = NULL;
                    }
                }
            }
            else // update timer
            {
                int duration = get_time_ms();
                int plustime = Max(duration - starttimer[gui_side], 0);
                thinktimer[gui_side] += plustime;
                starttimer[gui_side] = duration;
                if (thinktimer[gui_side] >= 1000)
                {
                    changed = 1;
                    --timer[gui_side];
                    thinktimer[gui_side] %= 1000;
                }
            }
            if (changed)
            {
                fill_clocktime(gui_side);
            }
        }

        // draw
        BeginDrawing();
        ClearBackground(DARKBROWN);

        // Draw board
        DrawTexture(
            img_back,
            BOARD_COL,
            BOARD_ROW,
            RAYWHITE);
        int sqr = 0;
        for (int row = 0; row < SIZE; ++row)
            for (int col = 0; col < SIZE; ++col)
                if ((row + col) % 2 == 0)
                    continue;
                else
                {
                    ++sqr;
                    Texture2D img = img_empty;
                    if (reversed)
                    {
                        if (guiboard[9 - row][9 - col] == 'w')
                            img = img_wpawn;
                        else if (guiboard[9 - row][9 - col] == 'b')
                            img = img_bpawn;
                        else if (guiboard[9 - row][9 - col] == 'W')
                            img = img_wking;
                        else if (guiboard[9 - row][9 - col] == 'B')
                            img = img_bking;
                    }
                    else
                    {
                        if (guiboard[row][col] == 'w')
                            img = img_wpawn;
                        else if (guiboard[row][col] == 'b')
                            img = img_bpawn;
                        else if (guiboard[row][col] == 'W')
                            img = img_wking;
                        else if (guiboard[row][col] == 'B')
                            img = img_bking;
                    }
                    DrawTexture(
                        img,
                        BOARD_COL + col * SQUARE_SIZE,
                        BOARD_ROW + row * SQUARE_SIZE,
                        RAYWHITE);
                    if (game_state == Game_play && (human_player == gui_side || human_player == both))
                    {
                        int sq = (reversed) ? 51 - sqr : sqr;
                        U64 pbit = selected_piece == 99 ? get_bit(move_options, sq) : selected_piece == sq ? 1ULL
                                                                                                           : 0ULL;
                        U64 sbit = selected_piece == 99 ? 0ULL : get_bit(square_options[selected_piece], sq);
                        U64 cbit = selected_piece == 99 ? 0ULL : get_bit(cap_options[selected_piece], sq);
                        if (pbit)
                        {
                            for (int d = 0; d < 3; ++d)
                                DrawRectangleLines(
                                    BOARD_COL + col * SQUARE_SIZE + d,
                                    BOARD_ROW + row * SQUARE_SIZE + d,
                                    SQUARE_SIZE - d * 2,
                                    SQUARE_SIZE - d * 2,
                                    GREEN);
                        }
                        if (sbit)
                        {
                            for (int d = 0; d < 3; ++d)
                                DrawRectangleLines(
                                    BOARD_COL + col * SQUARE_SIZE + d,
                                    BOARD_ROW + row * SQUARE_SIZE + d,
                                    SQUARE_SIZE - d * 2,
                                    SQUARE_SIZE - d * 2,
                                    BLUE);
                        }
                        if (cbit)
                        {
                            for (int d = 0; d < 3; ++d)
                                DrawRectangleLines(
                                    BOARD_COL + col * SQUARE_SIZE + d,
                                    BOARD_ROW + row * SQUARE_SIZE + d,
                                    SQUARE_SIZE - d * 2,
                                    SQUARE_SIZE - d * 2,
                                    RED);
                        }
                    }
                }
        // draw coordinates
        int sq = reversed ? 51 : 0;
        for (int row = 0; row < SIZE; ++row)
            for (int col = 0; col < SIZE; ++col)
            {
                if ((row + col) % 2 == 0)
                    continue;
                sq = reversed ? sq - 1 : sq + 1;
                DrawText(
                    squarenumber[sq],
                    BOARD_COL + SQUARE_SIZE * col + 2,
                    BOARD_ROW + SQUARE_SIZE * row + 2,
                    10,
                    RAYWHITE);
            }
        // game start, choose color and time
        if (game_state != Game_play)
        {
            DrawTexture(
                img_choice,
                0,
                SCREEN_HEIGHT - SQUARE_SIZE,
                RAYWHITE);
            DrawText(
                "Kies: F5=Wit, F6=Zwart, F7=Beide, F8=Auto",
                BOARD_COL,
                SCREEN_HEIGHT - 25,
                20,
                YELLOW);
            DrawText(
                "F1 = show helptext in the terminal",
                BOARD_COL,
                5,
                20,
                PURPLE);
            DrawText(
                "Tijd per spel: A=+5min, B=-5min, C=+1min, D=-1min",
                BOARD_COL,
                BOARD_ROW + BOARD_SIZE + 5,
                20,
                YELLOW);
            DrawText(
                "Plustijd per zet: F=+3sec, G=-3sec, H=+1sec, I=-1sec",
                BOARD_COL,
                BOARD_ROW + BOARD_SIZE + 25,
                20,
                YELLOW);
        }
        // instructions
        if (game_state == Game_play)
        {
            if (human_player == gui_side || human_player == both)
            {
                DrawText(
                    (selected_piece == 99) ? "Click op een groen gemarkeerd veld, click op x om op te geven"
                                           : "Click op een blauw gemarkeerd veld, click op x om op te geven",
                    BOARD_COL,
                    SCREEN_HEIGHT - 25,
                    20,
                    YELLOW);
                DrawText(
                    "F1 = show helptext in the terminal",
                    BOARD_COL,
                    5,
                    20,
                    PURPLE);
            }
            else if (ai_player == gui_side || ai_player == both)
                DrawText(
                    "Ik denk na over mijn zet...",
                    BOARD_COL,
                    SCREEN_HEIGHT - 25,
                    20,
                    YELLOW);
        }
        // game ends
        if (game_state == Game_stop)
        {
            char *text;
            switch (game_end_reason)
            {
            case wTime:
                text = "zwart wint doordat wit geen tijd meer heeft";
                break;
            case bTime:
                text = "wit wint doordat zwart geen tijd meer heeft";
                break;
            case wMove:
                text = "zwart wint doordat wit niet meer kan zetten";
                break;
            case bMove:
                text = "wit wint doordat zwart niet meer kan zetten";
                break;
            case wResign:
                text = "zwart wint omdat wit op geeft";
                break;
            case bResign:
                text = "wit wint omdat zwart op geeft";
                break;
            case draw50:
                text = "remise door de 50 zetten regel";
                break;
            case draw3:
                text = "remise door herhaling van zetten";
                break;
            }
            DrawText(
                text,
                BOARD_COL,
                SCREEN_HEIGHT - 50,
                20,
                PURPLE);
        }

        // draw the last move played
        // for black
        if (game_state == Game_play)
        {
            if (move_counter[black] > 0)
            {
                char text0[3];
                intToStr(move_counter[black], text0);
                char *text1 = squarenumber[get_move_source(last_move[black])];
                char *text2 = squarenumber[get_move_target(last_move[black])];
                char *text3 = get_move_capture(last_move[black]) ? "x" : "-";
                int posx = BOARD_COL;
                int posy = (reversed) ? BOARD_ROW + BOARD_SIZE + 4 : BOARD_ROW - 20;
                DrawText(
                    text0,
                    posx,
                    posy,
                    18,
                    LIGHTGRAY);
                DrawText(
                    text1,
                    posx + 30,
                    posy,
                    18,
                    LIGHTGRAY);
                DrawText(
                    text3,
                    posx + 50,
                    posy,
                    18,
                    LIGHTGRAY);
                DrawText(
                    text2,
                    posx + 60,
                    posy,
                    18,
                    LIGHTGRAY);
            }

            // for white
            if (move_counter[white] > 0)
            {
                char text0[3];
                intToStr(move_counter[white], text0);
                char *text1 = squarenumber[get_move_source(last_move[white])];
                char *text2 = squarenumber[get_move_target(last_move[white])];
                char *text3 = get_move_capture(last_move[white]) ? "x" : "-";
                int posx = BOARD_COL;
                int posy = (reversed) ? BOARD_ROW - 20 : BOARD_ROW + BOARD_SIZE + 4;
                DrawText(
                    text0,
                    posx,
                    posy,
                    18,
                    WHITE);
                DrawText(
                    text1,
                    posx + 30,
                    posy,
                    18,
                    WHITE);
                DrawText(
                    text3,
                    posx + 50,
                    posy,
                    18,
                    WHITE);
                DrawText(
                    text2,
                    posx + 60,
                    posy,
                    18,
                    WHITE);
            }
        }

        // Draw clock time
        char *number[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

        int posx = BOARD_COL + SQUARE_SIZE * 6;
        int posy = 0;
        DrawTexture(img_clock, posx, posy, DARKBROWN);
        DrawRectangle(posx + 16, posy + 30, 130, 20, BROWN);
        char *mid = ":";
        DrawText(number[clocktime[black][0]], posx + 18, posy + 34, 15, BLACK);
        DrawText(mid, posx + 28, posy + 34, 15, BLACK);
        DrawText(number[clocktime[black][1]], posx + 34, posy + 34, 15, BLACK);
        DrawText(number[clocktime[black][2]], posx + 44, posy + 34, 15, BLACK);
        DrawText(mid, posx + 54, posy + 34, 15, BLACK);
        DrawText(number[clocktime[black][3]], posx + 60, posy + 34, 15, BLACK);
        DrawText(number[clocktime[black][4]], posx + 70, posy + 34, 15, BLACK);
        DrawText(number[clocktime[white][0]], posx + 86, posy + 34, 15, WHITE);
        DrawText(mid, posx + 96, posy + 34, 15, WHITE);
        DrawText(number[clocktime[white][1]], posx + 100, posy + 34, 15, WHITE);
        DrawText(number[clocktime[white][2]], posx + 110, posy + 34, 15, WHITE);
        DrawText(mid, posx + 120, posy + 34, 12, WHITE);
        DrawText(number[clocktime[white][3]], posx + 124, posy + 34, 15, WHITE);
        DrawText(number[clocktime[white][4]], posx + 134, posy + 34, 15, WHITE);

        char plus[3];
        intToStr(plustimer[white], plus);
        char *txt = plustimer[white] ? concat("+", plus) : "+0";
        char *txt1 = concat(txt, " sec per zet");
        DrawText(
            txt1,
            posx + 150,
            posy + 34,
            15,
            YELLOW);

        EndDrawing();

        // key press
        if (IsKeyPressed(KEY_F1) && (human_player == gui_side || human_player == both))
            show_text();
        else if (IsKeyPressed(KEY_F5) && game_state != Game_play)
        {
            reversed = 0;
            human_player = white;
            ai_player = black;
            start_writing(human_player, ai_player);
            if (game_state == Game_stop)
                new_game(Game_play, gui_movelist);
            else
                game_state = Game_play;
        }
        else if (IsKeyPressed(KEY_F6) && game_state != Game_play)
        {
            reversed = 1;
            human_player = black;
            ai_player = white;
            start_writing(ai_player, human_player);
            if (game_state == Game_stop)
                new_game(Game_play, gui_movelist);
            else
                game_state = Game_play;
        }
        else if (IsKeyPressed(KEY_F7) && game_state != Game_play)
        {
            reversed = 0;
            human_player = both;
            ai_player = -1;
            start_writing(human_player, human_player);
            if (game_state == Game_stop)
                new_game(Game_play, gui_movelist);
            else
                game_state = Game_play;
        }
        else if (IsKeyPressed(KEY_F8) && game_state != Game_play)
        {
            reversed = 0;
            human_player = -1;
            ai_player = both;
            start_writing(ai_player, ai_player);
            if (game_state == Game_stop)
                new_game(Game_play, gui_movelist);
            else
                game_state = Game_play;
        }
        else if (IsKeyPressed(KEY_A) && game_state != Game_play)
        {
            if (timer[white] + 5 * 60 <= MAX_TIME * 60)
            {
                timer[white] = timer[black] = timer[white] + 5 * 60;
                fill_clocktime(white);
                fill_clocktime(black);
            }
        }
        else if (IsKeyPressed(KEY_B) && game_state != Game_play)
        {
            if (timer[white] - 5 * 60 >= MIN_TIME * 60)
            {
                timer[white] = timer[black] = timer[white] - 5 * 60;
                fill_clocktime(white);
                fill_clocktime(black);
            }
        }
        else if (IsKeyPressed(KEY_C) && game_state != Game_play)
        {
            if (timer[white] + 1 * 60 <= MAX_TIME * 60)
            {
                timer[white] = timer[black] = timer[white] + 1 * 60;
                fill_clocktime(white);
                fill_clocktime(black);
            }
        }
        else if (IsKeyPressed(KEY_D) && game_state != Game_play)
        {
            if (timer[white] - 1 * 60 >= MIN_TIME * 60)
            {
                timer[white] = timer[black] = timer[white] - 1 * 60;
                fill_clocktime(white);
                fill_clocktime(black);
            }
        }
        else if (IsKeyPressed(KEY_F) && game_state != Game_play)
        {
            if (plustimer[white] + 3 <= MAX_PLUS)
                plustimer[white] = plustimer[black] = plustimer[white] + 3;
        }
        else if (IsKeyPressed(KEY_G) && game_state != Game_play)
        {
            if (plustimer[white] - 3 >= MIN_PLUS)
                plustimer[white] = plustimer[black] = plustimer[white] - 3;
        }
        else if (IsKeyPressed(KEY_H) && game_state != Game_play)
        {
            if (plustimer[white] + 1 <= MAX_PLUS)
                plustimer[white] = plustimer[black] = plustimer[white] + 1;
        }
        else if (IsKeyPressed(KEY_I) && game_state != Game_play)
        {
            if (plustimer[white] - 1 >= MIN_PLUS)
                plustimer[white] = plustimer[black] = plustimer[white] - 1;
        }
        else if (IsKeyPressed(KEY_X) && game_state == Game_play)
        {
            game_state = Game_stop;
            game_winner = side ^= 1;
            if (game_winner == black)
            {
                game_end_reason = wResign;
                if (game_ptr)
                {
                    fprintf(game_ptr, "\n\nzwart wint omdat wit op geeft\n");
                    fclose(game_ptr);
                    game_ptr = NULL;
                }
            }
            else
            {
                game_end_reason = bResign;
                if (game_ptr)
                {
                    fprintf(game_ptr, "\n\nwit wint omdat zwart op geeft\n");
                    fclose(game_ptr);
                    game_ptr = NULL;
                }
            }
        }
        else if (IsKeyPressed(KEY_ENTER) && game_state == Game_play)
        {
            if (gui_movelist->caplength > 0)
            {
                U64 move = gui_movelist->moves[0];
                process_move(get_move_source(move), get_move_target(move), gui_movelist);
                selected_piece = 99;
                selected_square = 99;
                fill_gui_board();
                fill_options(gui_movelist);
            }
        }

        // Mouse Press
        if (human_player == gui_side || human_player == both)
        {
            if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
            {
                int x = GetMouseX();
                int y = GetMouseY();

                process_mouseclick(x, y, gui_movelist);
            }
        }

        // ai_move
        if (game_state == Game_play && (ai_player == gui_side || ai_player == both))
        {
            if (gui_movelist->counter > 0)
            {
                if (gui_movelist->counter == 1)
                {
                    if (counter % 10 == 0)
                    {
                        U64 bestmove = gui_movelist->moves[0];
                        process_move(get_move_source(bestmove), get_move_target(bestmove), gui_movelist);
                        selected_piece = 99;
                        selected_square = 99;
                        fill_gui_board();
                        fill_options(gui_movelist);
                    }
                }
                else
                {
                    if (USE_ENGINE)
                    {
                        if (!thread_busy)
                        {
                            // start the thread
                            pthread_create(&thread, NULL, task, NULL);
                            // forget that the thread is started, the task task_ready indicates when the thread is finished
                            pthread_detach(thread);
                        }
                        else if (task_ready) // thread is finished
                        {
                            process_move(get_move_source(pv_table[0][0]), get_move_target(pv_table[0][0]), gui_movelist);
                            selected_piece = 99;
                            selected_square = 99;
                            fill_gui_board();
                            fill_options(gui_movelist);
                            thread_busy = 0;
                        }
                    }
                    else if (counter % 10 == 0)
                    {
                        U64 bestmove = gui_movelist->moves[GetRandomValue(0, gui_movelist->counter - 1)];
                        process_move(get_move_source(bestmove), get_move_target(bestmove), gui_movelist);
                        selected_piece = 99;
                        selected_square = 99;
                        fill_gui_board();
                        fill_options(gui_movelist);
                    }
                }
            }
        }
    }

    if (game_ptr != NULL)
        fclose(game_ptr);

    // clean up
    // to stop searching rapidly when a thread is started
    stop_game_flag = 1;

    // wait until the thread is finished
    if (thread_busy)
    {
        while (!task_ready)
        {
            sleep(1);
        }
    }

    // free hash table memory on exit
    free(hash_table);

    // unload the images
    UnloadTexture(img_empty);
    UnloadTexture(img_wpawn);
    UnloadTexture(img_bpawn);
    UnloadTexture(img_wking);
    UnloadTexture(img_bking);
    UnloadTexture(img_back);
    UnloadTexture(img_choice);
    UnloadTexture(img_clock);
    // close the raylib window
    CloseWindow();

    return 0;
}