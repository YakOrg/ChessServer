#ifndef CHESS_SERVER_ANTICHEAT_H
#define CHESS_SERVER_ANTICHEAT_H

char **create_board();

void free_board(char **board);

int transform_board_with_move(
        char **board,
        char *white_turn,
        char x_old, char y_old,
        char x_new, char y_new,
        char *old_pawn_x, char *old_pawn_y
);

int transform_board_with_castling(char **board, char *white_turn, char long_castling);

int transform_board_with_en_passant(char **board, char *white_turn, char x, char y, char *old_pawn_x, char *old_pawn_y);

int transform_board_with_promotion(char **board, char *white_turn, char old_x, char new_x, char new_type);

#endif //CHESS_SERVER_ANTICHEAT_H
