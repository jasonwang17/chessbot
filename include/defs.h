#ifndef CHESSBOT_DEFS_H
#define CHESSBOT_DEFS_H

#pragma once
#include <cstdint>

enum Piece : int { EMPTY, WP, WN, WB, WR, WQ, WK, BP, BN, BB, BR, BQ, BK };
enum Side  : int { WHITE = 0, BLACK = 1 };

extern int board[64];
extern int side_to_move;

struct Move {
    uint8_t from = 0;
    uint8_t to = 0;
    uint8_t promo = 0; // 0 = none
};

void init();
void set_startpos();
void uci_loop();

#endif //CHESSBOT_DEFS_H
