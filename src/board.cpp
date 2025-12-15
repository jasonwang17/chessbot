#include "defs.h"

int board[64] = {0};
int side_to_move = WHITE;

void init() {}

void set_startpos() {
    for (int i = 0; i < 64; ++i) board[i] = EMPTY;
    side_to_move = WHITE;
}
