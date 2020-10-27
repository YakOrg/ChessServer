#ifndef CHESS_SERVER_ANTICHEAT_H
#define CHESS_SERVER_ANTICHEAT_H


int check_simple_move(char **board, char old_x, char old_y, char new_x, char new_y);

char** create_board();
void free_board(char** board);

#endif //CHESS_SERVER_ANTICHEAT_H
