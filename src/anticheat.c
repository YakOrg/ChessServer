// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++, C#, and Java: http://www.viva64.com

#include "anticheat.h"
#include <stdlib.h>
#include <stdio.h>

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


typedef struct position {
    char x;
    char y;
    struct position *next;
} position;

void print_board(char **board) {
    for (int i = 0; i < 8; ++i) {
        for (int j = 0; j < 8; ++j)
            printf("%c ", board[j][i]);
        printf("\n");
    }
}

int check_simple_move(char **board, char old_x, char old_y, char new_x, char new_y);

char starting_lineup(int j, int i) {
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

position *append(position *moves, char x, char y) {
    if (!moves) {
        moves = malloc(sizeof(position));
    } else {
        moves->next = malloc(sizeof(position));
        moves = moves->next;
    }
    moves->x = x;
    moves->y = y;
    moves->next = 0;
    return moves;
}

void free_positions(position *head) {
    position *cur;
    while ((cur = head)) {
        head = head->next;
        free(cur);
    }
}

position *get_movements(char old_x, char old_y, char new_x, char new_y) {
    position *moves = 0;
    position *head = 0;
    int on_right = old_x < new_x;
    int main_diagonal = old_x == old_y;
    int is_down = old_y < new_y;
    if (old_x == new_x) {
        if (is_down) {
            for (; old_y >= new_y; old_y++) {
                moves = append(moves, old_x, old_y);
                if (!head)
                    head = moves;
            }
        } else {
            for (; old_y <= new_y; old_y--) {
                moves = append(moves, old_x, old_y);
                if (!head)
                    head = moves;
            }
        }
    } else if (old_y == new_y) {
        if (on_right) {
            for (; old_x >= new_x; old_x++) {
                moves = append(moves, old_x, old_y);
                if (!head)
                    head = moves;
            }
        } else {
            for (; old_x <= new_x; old_x--) {
                moves = append(moves, old_x, old_y);
                if (!head)
                    head = moves;
            }
        }
    } else {
        if (main_diagonal) {
            if (on_right) {
                for (; old_x >= new_x && old_y >= new_y; old_x++, old_y++)
                    moves = append(moves, old_x, old_y);
                if (!head)
                    head = moves;
            } else {
                for (; old_x <= new_x && old_y <= new_y; old_x--, old_y--)
                    moves = append(moves, old_x, old_y);
                if (!head)
                    head = moves;
            }
        } else if (on_right) {
            for (; old_x >= new_x && old_y <= new_y; old_x++, old_y--)
                moves = append(moves, old_x, old_y);
            if (!head)
                head = moves;
        } else {
            for (; old_x <= new_x && old_y >= new_y; old_x--, old_y++)
                moves = append(moves, old_x, old_y);
            if (!head)
                head = moves;
        }
    }
    return head;
}

//TODO:
int is_check(char **board) {
    return 0;
}

char **create_board() {
    char **board = malloc(sizeof(char *) * 8);
    for (int i = 0; i < 8; ++i) {
        board[i] = malloc(sizeof(char) * 8);
        for (int j = 0; j < 8; ++j)
            board[i][j] = starting_lineup(i, j);
    }
    return board;
}

void free_board(char **board) {
    for (int i = 0; i < 8; ++i)
        free(board[i]);
    free(board);
}

int transform_board_with_move(char **board, char *white_turn, char x_old, char y_old, char x_new, char y_new,
                              char *old_pawn_x, char *old_pawn_y) {
    print_board(board);
    if (check_simple_move(board, x_old, y_old, x_new, y_new)) {
        if (white_turn) {
            if (y_old == 6 && abs(y_old - y_new) == 2 && board[x_old][y_old] == WHITE_PAWN) {
                *old_pawn_x = x_new;
                *old_pawn_y = y_new;
            }
        } else {
            if (y_old == 1 && abs(y_old - y_new) == 2 && board[x_old][y_old] == BLACK_PAWN) {
                *old_pawn_x = x_new;
                *old_pawn_y = y_new;
            }
        }
        if (is_check(board)) {
            char figure = board[x_new][y_new];
            board[x_new][y_new] = board[x_old][y_old];
            board[x_old][y_old] = BLANK;
            if (is_check(board)) {
                board[x_old][y_old] = board[x_new][y_new];
                board[x_new][y_new] = figure;
                return 1;
            }
        } else {
            board[x_new][y_new] = board[x_old][y_old];
            board[x_old][y_old] = BLANK;
        }
    } else return 1;
    *white_turn = (char) !(*white_turn);

    print_board(board);
    return 0;
}

int transform_board_with_castling(char **board, char *white_turn, char long_castling) {
    if (is_check(board)) return 1;
    if (white_turn) {
        if (long_castling) {
            if (board[7][1] == BLANK && board[7][2] == BLANK && board[7][3] == BLANK) {
                board[7][2] = WHITE_KING;
                board[7][4] = BLANK;
                board[7][0] = BLANK;
                board[7][3] = WHITE_ROOK;
            } else return 1;
        } else {
            if (board[7][5] == BLANK && board[7][6] == BLANK) {
                board[7][6] = WHITE_KING;
                board[7][4] = BLANK;
                board[7][7] = BLANK;
                board[7][5] = WHITE_ROOK;
            } else return 1;
        }
    } else {
        if (long_castling) {
            if (board[0][1] == BLANK && board[0][2] == BLANK && board[0][3] == BLANK) {
                board[0][2] = WHITE_KING;
                board[0][4] = BLANK;
                board[0][0] = BLANK;
                board[0][3] = WHITE_ROOK;
            } else return 1;
        } else {
            if (board[0][5] == BLANK && board[0][6] == BLANK) {
                board[0][6] = WHITE_KING;
                board[0][4] = BLANK;
                board[0][7] = BLANK;
                board[0][5] = WHITE_ROOK;
            } else return 1;
        }
    }
    *white_turn = (char) !white_turn;
    return 0;
}

int transform_board_with_en_passant(char **board, char *white_turn, char old_x, char old_y,
                                    char *old_pawn_x, char *old_pawn_y) {
    char new_y = *(white_turn ? old_pawn_y - 1 : old_pawn_y + 1);
    if (*old_pawn_x != -1 && *old_pawn_y != -1 &&
        check_simple_move(board, old_x, old_y, *old_pawn_x, new_y)) {
        board[*old_pawn_x][*old_pawn_y] = BLANK;
        board[*old_pawn_x][new_y] = board[old_x][old_y];
        board[old_x][old_y] = BLANK;
        *old_pawn_x = -1;
        *old_pawn_y = -1;
    } else return 1;
    *white_turn = (char) !white_turn;
    return 0;
}

int transform_board_with_promotion(char **board, char *white_turn, char old_x, char new_x, char new_type) {
    char old_y = white_turn ? 1 : 6;
    char new_y = white_turn ? 0 : 7;
    if (check_simple_move(board, old_x, old_y, new_x, new_y)) {
        board[old_x][old_y] = BLANK;
        board[new_x][new_y] = new_type;
    } else return 1;
    *white_turn = (char) !white_turn;
    return 0;
}

int check_trim(char **board, char old_x, char old_y, char new_x, char new_y) {
    if (old_x == new_x && old_y == new_y) return 0;
    if ((board[old_x][old_y] == WHITE_PAWN || board[old_x][old_y] == BLACK_PAWN) &&
        old_x == new_x &&
        board[new_x][new_y] != BLANK)
        return 0;
    if ((board[old_x][old_y] == WHITE_PAWN || board[old_x][old_y] == BLACK_PAWN) &&
        abs(old_x - new_x) == 1 &&
        board[new_x][new_y] == BLANK)
        return 0;
    position *moves = get_movements(old_x, old_y, new_x, new_y);
    for (position *pos = moves; pos; pos = pos->next) {
        if (pos->x != new_x && pos->y != new_y && board[pos->x][pos->y] != BLANK)
            return 0;
    }
    free_positions(moves);
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
            if (abs(new_x - old_x) <= 1 && abs(new_y - old_y) <= 1)
                return check_trim(board, old_x, old_y, new_x, new_y);
        } else if (board[old_x][old_y] == BLACK_KNIGHT || board[old_x][old_y] == WHITE_KNIGHT) {
            if ((abs(new_x - old_x) == 2 && abs(new_y - old_y) == 1) ||
                (abs(new_x - old_x) == 1 && abs(new_y - old_y) == 2))
                return 1;
        } else if (board[old_x][old_y] == BLACK_PAWN || board[old_x][old_y] == WHITE_PAWN) {
            if (board[old_x][old_y] == BLACK_PAWN && old_y == 1) {
                if (abs(new_x - old_x) <= 1)
                    if (new_y - old_y == 1 || new_y - old_y == 2)
                        return check_trim(board, old_x, old_y, new_x, new_y);
            } else if (board[old_x][old_y] == WHITE_PAWN && old_y == 6) {
                if (abs(new_x - old_x) <= 1)
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