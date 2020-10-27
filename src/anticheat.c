#include "anticheat.h"
#include <stdlib.h>

#define BLACK_BISHOP 'b'
#define BLACK_KING 'k'
#define BLACK_KNIGHT 'n'
#define BLACK_PAWN 'p'
#define BLACK_QUEEN 'q'
#define BLACK_ROOK 'r'

#define BLANK ' '

#define WHITE_BISHOP 'B'
#define WHITE_KING 'K'
#define WHITE_KNIGHT 'N'
#define WHITE_PAWN 'P'
#define WHITE_QUEEN 'Q'
#define WHITE_ROOK 'R'

char starting_lineup(int i, int j) {
    if (i == 1) return BLACK_PAWN;
    else if (i == 6) return WHITE_PAWN;
    else if (i == 0 && (j == 0 || j == 7)) return BLACK_ROOK;
    else if (i == 7 && (j == 0 || j == 7)) return WHITE_ROOK;
    else if (i == 0 && (j == 1 || j == 6)) return BLACK_KNIGHT;
    else if (i == 7 && (j == 1 || j == 6)) return WHITE_KNIGHT;
    else if (i == 0 && (j == 2 || j == 5)) return BLACK_BISHOP;
    else if (i == 7 && (j == 2 || j == 5)) return WHITE_BISHOP;
    else if (i == 0 && j == 3) return BLACK_QUEEN;
    else if (i == 7 && j == 3) return WHITE_QUEEN;
    else if (i == 0 && j == 4) return BLACK_KING;
    else if (i == 7 && j == 4) return WHITE_KING;
    return BLANK;
}

char** create_board() {
    char** board = malloc(sizeof(char*) * 8);
    for (int i = 0; i < 8; ++i) {
        board[i] = malloc(sizeof(char) * 8);
        for (int j = 0; j < 8; ++j)
            board[i][j] = starting_lineup(i, j);
    }
    return board;
}

void free_board(char** board) {
    for (int i = 0; i < 8; ++i)
        free(board[i]);
    free(board);
}

int check_trim(char **board, char old_x, char old_y, char new_x, char new_y) {
    if (old_x == new_x && old_y == new_y) return 0;
    if ((board[old_x][old_y] == WHITE_PAWN || board[old_x][old_y] == BLACK_PAWN) &&
        old_x == new_x &&
        board[new_x][new_y] != BLANK)
        return 0;

    if (new_y >= old_y && new_x >= old_x) {
        for (char i = old_x; i <= new_x; i++)
            for (char j = old_y; j <= new_y; j++)
                if (old_x != new_x && old_y != new_y && board[old_x][old_x] != BLANK) return 0;
    } else if (new_y >= old_y && new_x <= old_x) {
        for (char i = old_x; i >= new_x; i--)
            for (char j = old_y; j <= new_y; j++)
                if (old_x != new_x && old_y != new_y && board[old_x][old_x] != BLANK) return 0;
    } else if (new_y <= old_y && new_x >= old_x) {
        for (char i = old_x; i <= new_x; i++)
            for (char j = old_y; j >= new_y; j--)
                if (old_x != new_x && old_y != new_y && board[old_x][old_x] != BLANK) return 0;
    } else if (new_y <= old_y && new_x <= old_x) {
        for (char i = old_x; i >= new_x; i--)
            for (char j = old_y; j >= new_y; j--)
                if (old_x != new_x && old_y != new_y && board[old_x][old_x] != BLANK) return 0;
    }
    return 1;
}

int check_bounds(char c) {
    return c >= 0 && c < 8;
}

int check_simple_move(char **board, char old_x, char old_y, char new_x, char new_y) {

    if (check_bounds(old_x) && check_bounds(old_y) && check_bounds(new_x) && check_bounds(new_y)) {
        if (board[old_x][old_y] == BLACK_BISHOP || board[old_x][old_y] == WHITE_BISHOP) {
            if (abs(new_x - old_x) == abs(new_y - old_y))
                return check_trim(board, old_x, old_y, new_x, new_y);
        } else if (board[old_x][old_y] == BLACK_KING || board[old_x][old_y] == WHITE_KING) {
            if (abs(new_x - old_x) == 1 || abs(new_y - old_y) == 1)
                return check_trim(board, old_x, old_y, new_x, new_y);
        } else if (board[old_x][old_y] == BLACK_KNIGHT || board[old_x][old_y] == WHITE_KNIGHT) {
            if ((abs(new_x - old_x) == 2 && abs(new_y - old_y) == 1) ||
                (abs(new_x - old_x) == 1 && abs(new_y - old_y) == 2))
                return 1;
        } else if (board[old_x][old_y] == BLACK_PAWN || board[old_x][old_y] == WHITE_PAWN) {
            if (board[old_x][old_y] == BLACK_PAWN && old_y == 1) {
                if (new_y - old_y == 1 || new_y - old_y == 2)
                    return check_trim(board, old_x, old_y, new_x, new_y);
            } else if (board[old_x][old_y] == WHITE_PAWN && old_y == 6) {
                if (old_y - new_y == 1 || old_y - new_y == 2)
                    return check_trim(board, old_x, old_y, new_x, new_y);
            } else if (abs(new_y - old_y) == 1)
                return check_trim(board, old_x, old_y, new_x, new_y);
        } else if (board[old_x][old_y] == BLACK_QUEEN || board[old_x][old_y] == WHITE_QUEEN) {
            if ((abs(new_x - old_x) == abs(new_y - old_y)) || ((new_x == old_x) || (new_y == old_y)))
                return check_trim(board, old_x, old_y, new_x, new_y);
        } else if (board[old_x][old_y] == BLACK_ROOK || board[old_x][old_y] == WHITE_ROOK) {
            if ((new_x == old_x) || (new_y == old_y))
                return check_trim(board, old_x, old_y, new_x, new_y);
        }
    }
    return 0;
}