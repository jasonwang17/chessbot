#pragma once
#include <string>
#include <cstdint>

enum Piece {
    EMPTY = 0,
    WP, WN, WB, WR, WQ, WK,
    BP, BN, BB, BR, BQ, BK
};
enum Side  : int { WHITE = 0, BLACK = 1 };

extern int board[64];
extern int side_to_move;

struct Move {
    uint8_t from = 0;
    uint8_t to = 0;
    uint8_t promo = 0; // 0 = none, otherwise piece enum (WQ/WR/WB/WN or BQ/...)
};

struct Undo {
    uint8_t from = 0;
    uint8_t to = 0;
    int moved = EMPTY;
    int captured = EMPTY;
    uint8_t promo = 0; // Same encoding as Move
    int prev_side = WHITE;
    int prev_castling = 0; // Added to undo castling
};

// Lifecycle
void init();
void set_startpos();
bool set_fen(const char* fen);

// UCI
void uci_loop();

// Board helpers
int sq(int file, int rank); // File a-h, rank 1-8

// Move parsing
bool parse_uci_move(const std::string& s, Move& out);
int parse_square(char fileChar, char rankChar); // Returns 0..63 or -1
int promo_char_to_piece(char c, int side); // Returns piece enum or 0 if none/invalid

// Move list
struct MoveList {
    Move moves[256];
    int count = 0;
};

// Add legal filter over move list
struct LegalMoveList {
    Move moves[256];
    int count = 0;
};

void gen_moves(MoveList& list);
void gen_legal_moves(MoveList& legal);

// Make/undo stack (search foundation)
void clear_history();
bool make_move_basic(const Move &m); // Will remove later
bool make_move(const Move& m);
bool undo_move();

// Debug
char piece_to_char(int p);