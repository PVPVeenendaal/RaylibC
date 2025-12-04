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
#define SIZE 8
#define INFINITY 50000
#define MAX_PLY 64

// define bitboard data type
#define U64 unsigned long long

// set/get/pop bit macros
#define set_bit(bitboard, square) ((bitboard) |= (1ULL << (square)))
#define get_bit(bitboard, square) ((bitboard) & (1ULL << (square)))
#define pop_bit(bitboard, square) ((bitboard) &= ~(1ULL << (square)))

#define SCREEN_WIDTH 990
#define SCREEN_HEIGHT 810
#define BOARD_ROW 80
#define BOARD_COL 40
#define BOARD_SIZE 640
#define SQUARE_SIZE 80
#define HALF_SQUARE_SIZE 40

#define title "Checkers in Raylib-C (C)2025 Peter Veenendaal; versie: 0.90"

// define min max macros
#define Max(a, b) ((a) >= (b) ? (a) : (b))
#define Min(a, b) ((a) <= (b) ? (a) : (b))

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

// squares
//    0  1  2  3  4  5  6  7
// 0  -  1  -  3  -  5  -  7
// 1  8  - 10  - 12  - 14  -
// 2  - 17  - 19  - 21  - 23
// 3 24  - 26  - 28  - 30  -
// 4  - 33  - 35  - 37  - 39
// 5 40  - 42  - 44  - 46  -
// 6  - 49  - 51  - 53  - 55
// 7 56  - 58  - 60  - 62  -

typedef struct
{
    char sqf;
    char sqt;
    U64 cap;
} move_t;

typedef struct
{
    move_t moves[256];
    int counter;
    int caplength;
} moves_t;

// squares
//    0  1  2  3  4  5  6  7
// 0  -  1  -  3  -  5  -  7
// 1  8  - 10  - 12  - 14  -
// 2  - 17  - 19  - 21  - 23
// 3 24  - 26  - 28  - 30  -
// 4  - 33  - 35  - 37  - 39
// 5 40  - 42  - 44  - 46  -
// 6  - 49  - 51  - 53  - 55
// 7 56  - 58  - 60  - 62  -

const int squares[8][8] = {
    {0, 1, 0, 3, 0, 5, 0, 7},
    {8, 0, 10, 0, 12, 0, 14, 0},
    {0, 17, 0, 19, 0, 21, 0, 23},
    {24, 0, 26, 0, 28, 0, 30, 0},
    {0, 33, 0, 35, 0, 37, 0, 39},
    {40, 0, 42, 0, 44, 0, 46, 0},
    {0, 49, 0, 51, 0, 53, 0, 55},
    {56, 0, 58, 0, 60, 0, 62, 0},
};

const int single_sqaures[32] = {
    1,
    3,
    5,
    7,
    8,
    10,
    12,
    14,
    17,
    19,
    21,
    23,
    24,
    26,
    28,
    30,
    33,
    35,
    37,
    39,
    40,
    42,
    44,
    46,
    49,
    51,
    53,
    55,
    56,
    58,
    60,
    62,
};

const int rowcol[64][2] = {
    {0, 0},
    {0, 1},
    {0, 0},
    {0, 3},
    {0, 0},
    {0, 5},
    {0, 0},
    {0, 7}, //  0..7
    {1, 0},
    {0, 0},
    {1, 2},
    {0, 0},
    {1, 4},
    {0, 0},
    {1, 6},
    {0, 0}, //  8..15
    {0, 0},
    {2, 1},
    {0, 0},
    {2, 3},
    {0, 0},
    {2, 5},
    {0, 0},
    {2, 7}, // 16..23
    {3, 0},
    {0, 0},
    {3, 2},
    {0, 0},
    {3, 4},
    {0, 0},
    {3, 6},
    {0, 0}, // 24..31
    {0, 0},
    {4, 1},
    {0, 0},
    {4, 3},
    {0, 0},
    {4, 5},
    {0, 0},
    {4, 7}, // 32..39
    {5, 0},
    {0, 0},
    {5, 2},
    {0, 0},
    {5, 4},
    {0, 0},
    {5, 6},
    {0, 0}, // 40..47
    {0, 0},
    {6, 1},
    {0, 0},
    {6, 3},
    {0, 0},
    {6, 5},
    {0, 0},
    {6, 7}, // 48..55
    {7, 0},
    {0, 0},
    {7, 2},
    {0, 0},
    {7, 4},
    {0, 0},
    {7, 6},
    {0, 0}, // 56..63
};

static char *squarenumber[64] =
    {"", "01", "", "02", "", "03", "", "04",
     "05", "", "06", "", "07", "", "08", "",
     "", "09", "", "10", "", "11", "", "12",
     "13", "", "14", "", "15", "", "16", "",
     "", "17", "", "18", "", "19", "", "20",
     "21", "", "22", "", "23", "", "24", "",
     "", "25", "", "26", "", "27", "", "28",
     "29", "", "30", "", "31", "", "32", ""};

const int square_rank[64] =
    {
        1, 1, 1, 1, 1, 1, 1, 1,
        2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3,
        4, 4, 4, 4, 4, 4, 4, 4,
        5, 5, 5, 5, 5, 5, 5, 5,
        6, 6, 6, 6, 6, 6, 6, 6,
        7, 7, 7, 7, 7, 7, 7, 7,
        8, 8, 8, 8, 8, 8, 8, 8};

const int nx = 0;
const int dir[4][64] = {
    {
        // nw
        nx,
        0,
        nx,
        0,
        nx,
        0,
        nx,
        0,
        0,
        nx,
        1,
        nx,
        3,
        nx,
        5,
        nx,
        nx,
        8,
        nx,
        10,
        nx,
        12,
        nx,
        14,
        0,
        nx,
        17,
        nx,
        19,
        nx,
        21,
        nx,
        nx,
        24,
        nx,
        26,
        nx,
        28,
        nx,
        30,
        0,
        nx,
        33,
        nx,
        35,
        nx,
        37,
        nx,
        nx,
        40,
        nx,
        42,
        nx,
        44,
        nx,
        46,
        0,
        nx,
        49,
        nx,
        51,
        nx,
        53,
        nx,
    },
    {
        // ne
        nx,
        0,
        nx,
        0,
        nx,
        0,
        nx,
        0,
        1,
        nx,
        3,
        nx,
        5,
        nx,
        7,
        nx,
        nx,
        10,
        nx,
        12,
        nx,
        14,
        nx,
        0,
        17,
        nx,
        19,
        nx,
        21,
        nx,
        23,
        nx,
        nx,
        26,
        nx,
        28,
        nx,
        30,
        nx,
        0,
        33,
        nx,
        35,
        nx,
        37,
        nx,
        39,
        nx,
        nx,
        42,
        nx,
        44,
        nx,
        46,
        nx,
        0,
        49,
        nx,
        51,
        nx,
        53,
        nx,
        55,
        nx,
    },
    {
        // sw
        nx,
        8,
        nx,
        10,
        nx,
        12,
        nx,
        14,
        0,
        nx,
        17,
        nx,
        19,
        nx,
        21,
        nx,
        nx,
        24,
        nx,
        26,
        nx,
        28,
        nx,
        30,
        0,
        nx,
        33,
        nx,
        35,
        nx,
        37,
        nx,
        nx,
        40,
        nx,
        42,
        nx,
        44,
        nx,
        46,
        0,
        nx,
        49,
        nx,
        51,
        nx,
        53,
        nx,
        nx,
        56,
        nx,
        58,
        nx,
        60,
        nx,
        62,
        0,
        nx,
        0,
        nx,
        0,
        nx,
        0,
        nx,
    },
    {
        // se
        nx,
        10,
        nx,
        12,
        nx,
        14,
        nx,
        0,
        17,
        nx,
        19,
        nx,
        21,
        nx,
        23,
        nx,
        nx,
        26,
        nx,
        28,
        nx,
        30,
        nx,
        0,
        33,
        nx,
        35,
        nx,
        37,
        nx,
        39,
        nx,
        nx,
        42,
        nx,
        44,
        nx,
        46,
        nx,
        0,
        49,
        nx,
        51,
        nx,
        53,
        nx,
        55,
        nx,
        nx,
        58,
        nx,
        60,
        nx,
        62,
        nx,
        0,
        0,
        nx,
        0,
        nx,
        0,
        nx,
        0,
        nx,
    },
};

/**********************************\
 ==================================

    Checker board

 ==================================
\**********************************/

/*
     White pawns

  0  0 0 0 0 0 0 0 0
  1  0 0 0 0 0 0 0 0
  2  0 0 0 0 0 0 0 0
  3  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0
  5  1 0 1 0 1 0 1 0
  6  0 1 0 1 0 1 0 1
  7  1 0 1 0 1 0 1 0

     0 1 2 3 4 5 6 7

     Bitboard: 6172839697753047040d

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

  0  0 1 0 1 0 1 0 1
  1  1 0 1 0 1 0 1 0
  2  0 1 0 1 0 1 0 1
  3  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0
  7  0 0 0 0 0 0 0 0

     0 1 2 3 4 5 6 7

     Bitboard: 11163050d

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
  3  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0
  5  1 0 1 0 1 0 1 0
  6  0 1 0 1 0 1 0 1
  7  1 0 1 0 1 0 1 0

     0 1 2 3 4 5 6 7

     Bitboard: 6172839697753047040d

     Black occupancies

  0  0 1 0 1 0 1 0 1
  1  1 0 1 0 1 0 1 0
  2  0 1 0 1 0 1 0 1
  3  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0
  5  0 0 0 0 0 0 0 0
  6  0 0 0 0 0 0 0 0
  7  0 0 0 0 0 0 0 0

     0 1 2 3 4 5 6 7

     Bitboard: 11163050d

     Both occupancies

  0  0 1 0 1 0 1 0 1
  1  1 0 1 0 1 0 1 0
  2  0 1 0 1 0 1 0 1
  3  0 0 0 0 0 0 0 0
  4  0 0 0 0 0 0 0 0
  5  1 0 1 0 1 0 1 0
  6  0 1 0 1 0 1 0 1
  7  1 0 1 0 1 0 1 0

     0 1 2 3 4 5 6 7

     Bitboard: 6172839697764210090d

     All squares
  0  0 1 0 1 0 1 0 1
  1  1 0 1 0 1 0 1 0
  2  0 1 0 1 0 1 0 1
  3  1 0 1 0 1 0 1 0
  4  0 1 0 1 0 1 0 1
  5  1 0 1 0 1 0 1 0
  6  0 1 0 1 0 1 0 1
  7  1 0 1 0 1 0 1 0

     0 1 2 3 4 5 6 7

     Bitboard: 6172840429334713770d
*/

// piece bitboards
U64 bitboards[4];

// occupancy bitboards
U64 occupancies[3];

// board filled with all active squares
U64 all_squares = 0ULL;

// side to move
int side;

// directions on the board
U64 bb_dir[4][64];

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
#define copy_board()                            \
    U64 bitboards_copy[4], occupancies_copy[3]; \
    int side_copy;                              \
    memcpy(bitboards_copy, bitboards, 32);      \
    memcpy(occupancies_copy, occupancies, 24);  \
    side_copy = side;

// restore board state
#define take_back()                            \
    memcpy(bitboards, bitboards_copy, 32);     \
    memcpy(occupancies, occupancies_copy, 24); \
    side = side_copy;

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

    Fen

 ==================================
\**********************************/

// wrong fenstring
void wrong_fenstr(char *fenstr)
{
    printf("Wrong fenstr: %s", fenstr);
}

// 32 squares -> bitboards
int sqbb[33] = {
    0,
    1, 3, 5, 7,
    8, 10, 12, 14,
    17, 19, 21, 23,
    24, 26, 28, 30,
    33, 35, 37, 39,
    40, 42, 44, 46,
    49, 51, 53, 55,
    56, 58, 60, 62};

// fill the bitboards from the fenstring
void set_game_bitboards(int color, int king, int sq)
{
    if (color == white)
    {
        if (king)
            set_bit(bitboards[wKing], sqbb[sq]);
        else
            set_bit(bitboards[wPawn], sqbb[sq]);
    }
    else
    {
        if (king)
            set_bit(bitboards[bKing], sqbb[sq]);
        else
            set_bit(bitboards[bPawn], sqbb[sq]);
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
                if (sq < 1 || sq > 32)
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
            if (sq < 1 || sq > 32)
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
        if (sq < 1 || sq > 32)
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
    read_fen("B:W21,22,23,24,25,26,27,28,29,30,31,32:B1,2,3,4,5,6,7,8,9,10,11,12");

    all_squares = 0ULL;
    for (int i = 0; i < 32; ++i)
    {
        set_bit(all_squares, single_sqaures[i]);
    }

    for (int i = 0; i < 4; ++i)
        print_bitboard(bitboards[i]);
    for (int i = 0; i < 3; ++i)
        print_bitboard(occupancies[i]);
    print_bitboard(all_squares);
}

/**********************************\
 ==================================

    Move generator

 ==================================
\**********************************/

// initialize bitboards dir
void init_bb_dir()
{
    memset(bb_dir, 0ULL, sizeof(bb_dir));

    for (int d = nw; d <= se; ++d)
        for (int sq = 0; sq < 64; ++sq)
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
    int dirP1 = side == white ? nw : sw;
    int dirP2 = side == white ? ne : se;
    int sqf, sqt, sqo, piece;

    for (int i = 0; i < old_list->counter; ++i)
    {
        move_t m = old_list->moves[i];
        if (get_bit(bitboards[wPawn], m.sqf))
            piece = wPawn;
        else if (get_bit(bitboards[bPawn], m.sqf))
            piece = bPawn;
        else if (get_bit(bitboards[wKing], m.sqf))
            piece = wKing;
        else if (get_bit(bitboards[bKing], m.sqf))
            piece = bKing;
        sqf = m.sqt;
        if (piece == wPawn || piece == bPawn)
        {
            dirP1 = side == white ? nw : sw;
            dirP2 = side == white ? ne : se;
        }
        else
        {
            dirP1 = nw;
            dirP2 = se;
        }
        for (int d = dirP1; d <= dirP2; ++d)
        {
            U64 b = capture & bb_dir[d][sqf];
            if (b)
            {
                sqo = get_ls1b_index(b);
                if (get_bit(m.cap, sqo)) // can't jump over a piece a second time
                    continue;
                U64 e = empty & bb_dir[d][sqo];
                if (e) // next capture found
                {
                    sqt = get_ls1b_index(e);
                    U64 newcap = m.cap;
                    set_bit(newcap, sqo);
                    move_list->moves[move_list->counter].sqf = m.sqf;
                    move_list->moves[move_list->counter].sqt = sqt;
                    move_list->moves[move_list->counter].cap = newcap;
                    ++move_list->counter;
                    move_list->caplength = old_list->caplength + 1;
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

    while (pawns)
    {
        sqf = get_ls1b_index(pawns);
        for (int d = dirP1; d <= dirP2; ++d)
        {
            U64 b = capture & bb_dir[d][sqf];
            if (b)
            {
                sqo = get_ls1b_index(b);
                U64 e = empty & bb_dir[d][sqo];
                if (e) // capture found
                {
                    sqt = get_ls1b_index(e);
                    if (movelist->caplength == 0)
                        movelist->counter = 0;
                    movelist->moves[movelist->counter].sqf = sqf;
                    movelist->moves[movelist->counter].sqt = sqt;
                    set_bit(movelist->moves[movelist->counter].cap, sqo);
                    ++movelist->counter;
                    movelist->caplength = 1;
                }
            }
            else
            {
                U64 e = empty & bb_dir[d][sqf];
                if (e && movelist->caplength == 0)
                {
                    sqt = get_ls1b_index(e);
                    movelist->moves[movelist->counter].sqf = sqf;
                    movelist->moves[movelist->counter].sqt = sqt;
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
            U64 b = capture & bb_dir[d][sqf];
            if (b)
            {
                sqo = get_ls1b_index(b);
                U64 e = empty & bb_dir[d][sqo];
                if (e) // capture found
                {
                    sqt = get_ls1b_index(e);
                    if (movelist->caplength == 0)
                        movelist->counter = 0;
                    movelist->moves[movelist->counter].sqf = sqf;
                    movelist->moves[movelist->counter].sqt = sqt;
                    set_bit(movelist->moves[movelist->counter].cap, sqo);
                    ++movelist->counter;
                    movelist->caplength = 1;
                }
            }
            else
            {
                U64 e = empty & bb_dir[d][sqf];
                if (e && movelist->caplength == 0)
                {
                    sqt = get_ls1b_index(e);
                    movelist->moves[movelist->counter].sqf = sqf;
                    movelist->moves[movelist->counter].sqt = sqt;
                    ++movelist->counter;
                }
            }
        }

        pop_bit(kings, sqf);
    }

    if (movelist->caplength == 1)
        generate_next_captures(movelist);
}

// print_move
void print_move(move_t mov)
{
    int sqf = mov.sqf;
    int sqt = mov.sqt;
    U64 cap = mov.cap;

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
int make_move(move_t move, int move_type)
{
    int sqf = move.sqf;
    int sqt = move.sqt;
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

        // capture
        if (move.cap > 0ULL)
        {
            U64 c = move.cap;
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
        }
        if (piece == wPawn && square_rank[sqt] == 1)
        {
            pop_bit(bitboards[wPawn], sqt);
            set_bit(bitboards[wKing], sqt);
        }

        set_occupancies();
        side ^= 1;
    }
    else
    {
        if (move.cap > 0ULL)
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

// evaluate the position on the board
static inline int evaluate()
{
    int score = 0;
    int white_rank = -99;
    int black_rank = -99;
    int white_has_king = 0;
    int black_has_king = 0;
    U64 bb;

    if (occupancies[black] != 0ULL)
    {
        int sq = get_ls1b_index(occupancies[black]);
        black_rank = square_rank[sq];
        if (bitboards[bKing])
            black_has_king = 1;
    }

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
            {
                score += 1000;
                // centrum
                if (sq == 26 || sq == 28 || sq == 35 || sq == 37)
                    score += 5;
                // passed checker
                if (square_rank[sq] < black_rank)
                    score += (black_has_king) ? 10 : 50;
                if (square_rank[sq] > white_rank)
                    white_rank = square_rank[sq];
            }
            else if (get_bit(bitboards[wKing], sq))
            {
                score += 3000;
                white_has_king = 1;
            }
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
            {
                score -= 1000;
                // centrum
                if (sq == 26 || sq == 28 || sq == 35 || sq == 37)
                    score -= 5;
                // passed checker
                if (square_rank[sq] > white_rank)
                    score -= (white_has_king) ? 10 : 50;
            }
            else if (get_bit(bitboards[bKing], sq))
                score -= 3000;
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
move_t pv_table[MAX_PLY][MAX_PLY];

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

// is move equal
static inline int move_is_equal(move_t move1, move_t move2)
{
    return move1.sqf == move2.sqf && move1.sqt == move2.sqt && move1.cap == move2.cap;
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
        if (move_is_equal(pv_table[0][ply], move_list->moves[count]))
        {
            // enable move scoring
            score_pv = 1;

            // enable following PV
            follow_pv = 1;
        }
    }
}

// score form a move
static inline int score_move(move_t move)
{
    // if PV move scoring is allowed
    if (score_pv)
    {
        // make sure we are dealing with PV move
        if (move_is_equal(pv_table[0][ply], move))
        {
            // disable score PV flag
            score_pv = 0;

            // give PV move the highest score to search it first
            return 20000;
        }
    }
    if (move.cap > 0ULL)
    {
        return 10000 + count_bits(move.cap);
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
                move_t temp_move = move_list->moves[current_move];
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
    int score;

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
                // node (position) fails high
                return beta;
            }
        }
    }

    // we don't have any legal moves to make in the current postion
    if (legal_moves == 0)
        return evaluate();

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
U64 square_options[64];

// quick check which of the pieces can captured
// when the human player is to move
// square has a red border
U64 cap_options[64];

// square from the selected piece
// when the human player is to move
int selected_piece = 99;

// sqaure from the selected square where the selected piece can go to
// when the human player is to move
int selected_square = 99;

// movelist used in the GUI
moves_t gui_movelist[1];

// all played moves in the game
moves_t game_movelist[2];

// TASK
// flag that indicates that the thinking task is ready
static int task_ready = 0;

// flag that indicates that the ai is thinking
static int thread_busy = 0;

// fill the gui_board for drawing
void fill_gui_board()
{
    for (int row = 0; row < SIZE; ++row)
        for (int col = 0; col < SIZE; ++col)
            if ((row + col) % 2 == 0)
                guiboard[row][col] = 'x';
            else
            {
                int sq = row * 8 + col;
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
        move_t m = movelist->moves[index];
        set_bit(move_options, (int)m.sqf);
        set_bit(square_options[(int)m.sqf], (int)m.sqt);
        cap_options[(int)m.sqf] |= m.cap;
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

// process the move
void process_move(const int selected_piece, const int selected_square, moves_t *movelist)
{
    for (int i = 0; i < movelist->counter; ++i)
    {
        if (movelist->moves[i].sqf == selected_piece && movelist->moves[i].sqt == selected_square)
        {
            game_movelist[side].moves[game_movelist[side].counter++] = movelist->moves[i];
            make_move(movelist->moves[i], all_moves);
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
    int sqr = -1;
    // select square on the board
    sqr = reversed ? 63 - (sqry * 8 + sqrx) : sqry * 8 + sqrx;
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
    timer[white] = timer[black] = 900;
    plustimer[white] = plustimer[black] = 3;
    thinktimer[white] = thinktimer[black] = 0;
    // fill the clock settings
    fill_clocktime(white);
    fill_clocktime(black);
    press_clock = 1;
    init_board();
    gui_side = side;
    generate_moves(movelist);
    fill_gui_board();
    fill_options(movelist);
    memset(game_movelist, 0, sizeof(moves_t) * 2);
    game_state = state;
}

// calculate the best move for the ai in a seperate thread
void *task(void *arg)
{
    thread_busy = 1;
    task_ready = 0;
    int depth = 64;
    movestogo = Max(50 - (game_movelist[0].counter) / 2, 10);
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

//-----------------------------------------------
// MAIN
//-----------------------------------------------

int main()
{
    init_bb_dir();
    new_game(Game_start, gui_movelist);
    int counter = 0;

    if (TEST)
    {
        char *testfen = "B:W9,10,24:B8,K14,21";
        read_fen(testfen);
        generate_moves(gui_movelist);
        fill_gui_board();
        fill_options(gui_movelist);
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
    img_clock.width = SQUARE_SIZE * 2;

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
        if (gui_movelist->counter == 0)
        {
            if (side == white)
            {
                game_winner = black;
                game_state = Game_stop;
            }
            else
            {
                game_winner = white;
                game_state = Game_stop;
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
        DrawRectangle(
            BOARD_COL,
            BOARD_ROW,
            BOARD_SIZE,
            BOARD_SIZE,
            GRAY);
        for (int row = 0; row < SIZE; ++row)
            for (int col = 0; col < SIZE; ++col)
                if ((row + col) % 2 == 0)
                    continue;
                else
                {
                    Texture2D img = img_empty;
                    if (reversed)
                    {
                        if (guiboard[7 - row][7 - col] == 'w')
                            img = img_wpawn;
                        else if (guiboard[7 - row][7 - col] == 'b')
                            img = img_bpawn;
                        else if (guiboard[7 - row][7 - col] == 'W')
                            img = img_wking;
                        else if (guiboard[7 - row][7 - col] == 'B')
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
                        LIGHTGRAY);
                    if (game_state == Game_play && (human_player == gui_side || human_player == both))
                    {
                        int sq = (reversed) ? (7 - row) * 8 + 7 - col : row * 8 + col;
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
        for (int row = 0; row < SIZE; ++row)
            for (int col = 0; col < SIZE; ++col)
            {
                int sq = reversed ? 63 - (row * 8 + col) : row * 8 + col;
                DrawText(
                    squarenumber[sq],
                    BOARD_COL + SQUARE_SIZE * col + 2,
                    BOARD_ROW + SQUARE_SIZE * row + 2,
                    10,
                    RAYWHITE);
            }
        // game start, choose color
        if (game_state != Game_play)
        {
            DrawText(
                "Kies: F5=Wit, F6=Zwart, F7=Beide, F8=Auto",
                BOARD_COL,
                SCREEN_HEIGHT - 25,
                20,
                YELLOW);
        }
        // instructions
        if (game_state == Game_play)
        {
            if (human_player == gui_side || human_player == both)
                DrawText(
                    (selected_piece == 99) ? "Maak je zet, click op een groen gemarkeerd veld, click op x om op te geven"
                                           : "Click op een blauw gemarkeerd veld, click op x om op te geven",
                    BOARD_COL,
                    SCREEN_HEIGHT - 25,
                    20,
                    YELLOW);
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
            DrawText(
                (game_winner == white) ? "Wit wint!" : "Zwart wint",
                BOARD_COL,
                SCREEN_HEIGHT - 50,
                20,
                PURPLE);
        }
        // draw game_list
        DrawRectangle(
            BOARD_COL + BOARD_SIZE + HALF_SQUARE_SIZE - 4,
            BOARD_ROW - 4,
            178,
            650,
            GRAY);
        DrawText(
            "Zwart",
            BOARD_COL + BOARD_SIZE + HALF_SQUARE_SIZE + 30,
            BOARD_ROW,
            18,
            BLACK);
        DrawText(
            "Wit",
            BOARD_COL + BOARD_SIZE + HALF_SQUARE_SIZE + 120,
            BOARD_ROW,
            18,
            WHITE);

        int minidx = 0;
        int cnt = game_movelist[black].counter;
        for (int n = 5; n >= 1; --n)
        {
            if (cnt > 30 * n)
            {
                minidx = game_movelist[black].counter - 30 * n;
                break;
            }
        }
        int rowidx = -1;
        for (int index = minidx; index < game_movelist[black].counter; ++index)
        {
            ++rowidx;
            move_t m = game_movelist[black].moves[index];
            char text0[3];
            intToStr(index + 1, text0);
            char *text1 = squarenumber[(int)m.sqf];
            char *text2 = squarenumber[(int)m.sqt];
            char *text3 = m.cap ? "x" : "-";
            DrawText(
                text0,
                BOARD_COL + BOARD_SIZE + HALF_SQUARE_SIZE,
                BOARD_ROW + HALF_SQUARE_SIZE + rowidx * 20,
                18,
                PURPLE);
            DrawText(
                text1,
                BOARD_COL + BOARD_SIZE + HALF_SQUARE_SIZE + 30,
                BOARD_ROW + HALF_SQUARE_SIZE + rowidx * 20,
                18,
                BLACK);
            DrawText(
                text3,
                BOARD_COL + BOARD_SIZE + HALF_SQUARE_SIZE + 50,
                BOARD_ROW + HALF_SQUARE_SIZE + rowidx * 20,
                18,
                BLACK);
            DrawText(
                text2,
                BOARD_COL + BOARD_SIZE + HALF_SQUARE_SIZE + 60,
                BOARD_ROW + HALF_SQUARE_SIZE + rowidx * 20,
                18,
                BLACK);
            if (game_movelist[white].moves[index].sqf >= 0 && game_movelist[white].moves[index].sqt >= 0)
            {
                move_t m = game_movelist[white].moves[index];
                char *text1 = squarenumber[(int)m.sqf];
                char *text2 = squarenumber[(int)m.sqt];
                char *text3 = m.cap ? "x" : "-";
                DrawText(
                    text1,
                    BOARD_COL + BOARD_SIZE + HALF_SQUARE_SIZE + 120,
                    BOARD_ROW + HALF_SQUARE_SIZE + rowidx * 20,
                    18,
                    WHITE);
                DrawText(
                    text3,
                    BOARD_COL + BOARD_SIZE + HALF_SQUARE_SIZE + 140,
                    BOARD_ROW + HALF_SQUARE_SIZE + rowidx * 20,
                    18,
                    WHITE);
                DrawText(
                    text2,
                    BOARD_COL + BOARD_SIZE + HALF_SQUARE_SIZE + 150,
                    BOARD_ROW + HALF_SQUARE_SIZE + rowidx * 20,
                    18,
                    WHITE);
            }
        }

        char *number[10] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"};

        // Draw clock time
        int posx = BOARD_COL + SQUARE_SIZE * 6;
        int posy = 0;
        DrawTexture(img_clock, posx, posy, DARKBROWN);
        DrawRectangle(posx + 16, posy + 30, 130, 20, BROWN);
        char *mid = ":";
        DrawText(number[clocktime[black][0]], posx + 18, posy + 34, 12, BLACK);
        DrawText(mid, posx + 28, posy + 34, 12, BLACK);
        DrawText(number[clocktime[black][1]], posx + 34, posy + 34, 12, BLACK);
        DrawText(number[clocktime[black][2]], posx + 44, posy + 34, 12, BLACK);
        DrawText(mid, posx + 54, posy + 34, 12, BLACK);
        DrawText(number[clocktime[black][3]], posx + 60, posy + 34, 12, BLACK);
        DrawText(number[clocktime[black][4]], posx + 70, posy + 34, 12, BLACK);
        DrawText(number[clocktime[white][0]], posx + 86, posy + 34, 12, WHITE);
        DrawText(mid, posx + 96, posy + 34, 12, WHITE);
        DrawText(number[clocktime[white][1]], posx + 100, posy + 34, 12, WHITE);
        DrawText(number[clocktime[white][2]], posx + 110, posy + 34, 12, WHITE);
        DrawText(mid, posx + 120, posy + 34, 12, WHITE);
        DrawText(number[clocktime[white][3]], posx + 124, posy + 34, 12, WHITE);
        DrawText(number[clocktime[white][4]], posx + 134, posy + 34, 12, WHITE);

        EndDrawing();

        // key press
        if (IsKeyPressed(KEY_F5) && game_state != Game_play)
        {
            reversed = 0;
            human_player = white;
            ai_player = black;
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
            if (game_state == Game_stop)
                new_game(Game_play, gui_movelist);
            else
                game_state = Game_play;
        }
        else if (IsKeyPressed(KEY_X) && game_state == Game_play)
        {
            game_state = Game_stop;
            game_winner = side ^= 1;
        }
        else if (IsKeyPressed(KEY_ENTER) && game_state == Game_play)
        {
            if (gui_movelist->caplength > 0)
            {
                move_t move = gui_movelist->moves[0];
                process_move(move.sqf, move.sqt, gui_movelist);
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
                    if (counter % 5 == 0)
                    {
                        move_t bestmove = gui_movelist->moves[0];
                        process_move(bestmove.sqf, bestmove.sqt, gui_movelist);
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
                            process_move(pv_table[0][0].sqf, pv_table[0][0].sqt, gui_movelist);
                            selected_piece = 99;
                            selected_square = 99;
                            fill_gui_board();
                            fill_options(gui_movelist);
                            thread_busy = 0;
                        }
                    }
                    else if (counter % 5 == 0)
                    {
                        move_t bestmove = gui_movelist->moves[GetRandomValue(0, gui_movelist->counter - 1)];
                        process_move(bestmove.sqf, bestmove.sqt, gui_movelist);
                        selected_piece = 99;
                        selected_square = 99;
                        fill_gui_board();
                        fill_options(gui_movelist);
                    }
                }
            }
        }
    }
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

    // unload the images
    UnloadTexture(img_empty);
    UnloadTexture(img_wpawn);
    UnloadTexture(img_bpawn);
    UnloadTexture(img_wking);
    UnloadTexture(img_bking);
    // close the raylib window
    CloseWindow();

    return 0;
}