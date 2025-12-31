#include "defs.h"

#include <vector>
#include <string>
#include <cstdlib>
#include <cstdint>

// It should be noted to avoid any confusion that this is flipped from the display.
// White appears on the bottom when asking for a board display (cmd d), but white is at the top of this array.
int board[64] = {0};
int side_to_move = WHITE;

// From what I've seen a vector technically (?) be better than stack or deque here:
static std::vector<Undo> history;

void clear_history() {
    history.clear();
}

// Convert Piece enum from defs to piece character for FEN notation
char piece_to_char(int p) {
    switch (p) {
        case WP: return 'P'; case WN: return 'N'; case WB: return 'B';
        case WR: return 'R'; case WQ: return 'Q'; case WK: return 'K';
        case BP: return 'p'; case BN: return 'n'; case BB: return 'b';
        case BR: return 'r'; case BQ: return 'q'; case BK: return 'k';
        case EMPTY: return '.';
        default: return '?'; // Invalid piece
    }
}

// Other way around
static int char_to_piece(char c) {
    switch (c) {
        case 'P': return WP; case 'N': return WN; case 'B': return WB;
        case 'R': return WR; case 'Q': return WQ; case 'K': return WK;
        case 'p': return BP; case 'n': return BN; case 'b': return BB;
        case 'r': return BR; case 'q': return BQ; case 'k': return BK;
        default:  return -1; // Invalid piece
    }
}

int sq(int file, int rank) { return rank * 8 + file; }

static int file_of(int sq) { return sq & 7; }   // And by 7 (range is 0-7 inclusive) to find file
static int rank_of(int sq) { return sq >> 3; }  // Rightshift 3 bits to find rank

// Validate square passed by character value of file (a-h) and rank (1-8)
int parse_square(const char fileChar, const char rankChar) {
    if (fileChar < 'a' || fileChar > 'h') return -1;
    if (rankChar < '1' || rankChar > '8') return -1;
    int file = fileChar - 'a';
    int rank = rankChar - '1';
    return sq(file, rank);
}

// Movegen helpers
static bool is_white(int p) {
    return p == WP || p == WN || p == WB || p == WR || p == WQ || p == WK;
}
static bool is_black(int p) {
    return p == BP || p == BN || p == BB || p == BR || p == BQ || p == BK;
}

static bool same_side(int p, int side) {
    return (side == WHITE) ? is_white(p) : is_black(p);
}

static bool enemy_side(int p, int side) {
    return (side == WHITE) ? is_black(p) : is_white(p);
}

// Movegen
static const int KNIGHT_DF[8] = { +1, +2, +2, +1, -1, -2, -2, -1 };
static const int KNIGHT_DR[8] = { +2, +1, -1, -2, -2, -1, +1, +2 };

static const int KING_DF[8] = { -1,  0, +1, -1, +1, -1,  0, +1 };
static const int KING_DR[8] = { -1, -1, -1,  0,  0, +1, +1, +1 };

static const int BISHOP_DF[4] = { +1, -1, +1, -1 };
static const int BISHOP_DR[4] = { +1, +1, -1, -1 };

static const int ROOK_DF[4] = {  0,  0, +1, -1 };
static const int ROOK_DR[4] = { +1, -1,  0,  0 };

static const int QUEEN_DF[8] = {  0,  0, +1, -1, +1, -1, +1, -1 };
static const int QUEEN_DR[8] = { +1, -1,  0,  0, +1, +1, -1, -1 };

static int find_king_square(int side) {
    int king = (side == WHITE) ? WK : BK;
    for (int i = 0; i < 64; i++) {
        if (board[i] == king) return i;
    }
    return -1; // Should never happen if a position is valid
}

// Return true if at any point an attack upon this square is found
static bool is_square_attacked(int targetSq, int bySide) {
    int fromF = file_of(targetSq);
    int fromR = rank_of(targetSq);

    // Pawn
    if (bySide == WHITE) {
        // White pawn attacks come from (file-1, rank-1) and (file+1, rank-1)
        if (fromR > 0) {
            if (fromF > 0) {
                int to = sq(fromF - 1, fromR - 1);
                if (board[to] == WP) return true;
            }
            if (fromF < 7) {
                int to = sq(fromF + 1, fromR - 1);
                if (board[to] == WP) return true;
            }
        }
    } else {
        // Black pawns (inverse logic)
        if (fromR < 7) {
            if (fromF > 0) {
                int to = sq(fromF - 1, fromR + 1);
                if (board[to] == BP) return true;
            }
            if (fromF < 7) {
                int to = sq(fromF + 1, fromR + 1);
                if (board[to] == BP) return true;
            }
        }
    }

    // Knight
    {
        int knight = (bySide == WHITE) ? WN : BN;
        for (int i = 0; i < 8; i++) {
            int f = fromF + KNIGHT_DF[i];
            int r = fromR + KNIGHT_DR[i];
            if (f < 0 || f >= 8 || r < 0 || r >= 8) continue;

            int to = sq(f, r);
            if (board[to] == knight) return true;
        }
    }

    // King
    {
        int king = (bySide == WHITE) ? WK : BK;
        for (int i = 0; i < 8; i++) {
            int f = fromF + KING_DF[i];
            int r = fromR + KING_DR[i];
            if (f < 0 || f >= 8 || r < 0 || r >= 8) continue;

            int to = sq(f, r);
            if (board[to] == king) return true;
        }
    }

    // Diagonals (Bishop, queen)
    {
        int bishop = (bySide == WHITE) ? WB : BB;
        int queen  = (bySide == WHITE) ? WQ : BQ;

        for (int d = 0; d < 4; d++) {
            int f = fromF + BISHOP_DF[d];
            int r = fromR + BISHOP_DR[d];

            while (f >= 0 && f < 8 && r >= 0 && r < 8) {
                int to = sq(f, r);
                int target = board[to];

                if (target != EMPTY) {
                    if (target == bishop || target == queen) return true;
                    break;
                }

                f += BISHOP_DF[d];
                r += BISHOP_DR[d];
            }
        }
    }

    // Orthogonal (Rook, queen)
    {
        int rook  = (bySide == WHITE) ? WR : BR;
        int queen = (bySide == WHITE) ? WQ : BQ;

        for (int d = 0; d < 4; d++) {
            int f = fromF + ROOK_DF[d];
            int r = fromR + ROOK_DR[d];

            while (f >= 0 && f < 8 && r >= 0 && r < 8) {
                int to = sq(f, r);
                int target = board[to];

                if (target != EMPTY) {
                    if (target == rook || target == queen) return true;
                    break;
                }

                f += ROOK_DF[d];
                r += ROOK_DR[d];
            }
        }
    }
    return false; // Not under attack
}

static void add_move(MoveList& list, int from, int to, int promo = 0) {
    Move m;
    m.from = (uint8_t)from;
    m.to = (uint8_t)to;
    m.promo = (uint8_t)promo;
    list.moves[list.count++] = m;
}

static void gen_pawn_moves(int from, MoveList& list, int side) {
    int f = file_of(from);
    int r = rank_of(from);

    // White pawns:
    if (side == WHITE) {
        int oneUp = from + 8; // Look one above
        if (oneUp < 64 && board[oneUp] == EMPTY) {
            // Pawn can promote if on rank 7 and rank 8 above it is empty
            if (r == 6) {
                add_move(list, from, oneUp, WQ);
                add_move(list, from, oneUp, WR);
                add_move(list, from, oneUp, WB);
                add_move(list, from, oneUp, WN);
            } else { // No promo, just one step ahead is ok
                add_move(list, from, oneUp, 0);

                // Double push from rank 2
                if (r == 1) {
                    int twoUp = from + 16; // Look two ahead
                    if (twoUp < 64 && board[twoUp] == EMPTY) {
                        add_move(list, from, twoUp, 0);
                    }
                }
            }
        }

        // Captures
        if (f > 0) { // If we can look to the left:
            int cap = from + 7; // Look to the left and above
            if (cap < 64 && enemy_side(board[cap], side)) {
                if (r == 6) { // Add all promotion moves
                    add_move(list, from, cap, WQ);
                    add_move(list, from, cap, WR);
                    add_move(list, from, cap, WB);
                    add_move(list, from, cap, WN);
                } else {
                    add_move(list, from, cap, 0);
                }
            }
        }
        if (f < 7) { // If we can look to the right
            int cap = from + 9; // Look above and to the right
            if (cap < 64 && enemy_side(board[cap], side)) {
                if (r == 6) { // Does this capture also promote the pawn?
                    add_move(list, from, cap, WQ);
                    add_move(list, from, cap, WR);
                    add_move(list, from, cap, WB);
                    add_move(list, from, cap, WN);
                } else { // Just the capture move
                    add_move(list, from, cap, 0);
                }
            }
        }
    } else { // Black pieces (same logic as white, minus instead of plus for indexing).
        int one = from - 8; // Look one below
        if (one >= 0 && board[one] == EMPTY) {
            if (r == 1) {
                add_move(list, from, one, BQ);
                add_move(list, from, one, BR);
                add_move(list, from, one, BB);
                add_move(list, from, one, BN);
            } else {
                add_move(list, from, one, 0);
                if (r == 6) {
                    int two = from - 16;
                    if (two >= 0 && board[two] == EMPTY) {
                        add_move(list, from, two, 0);
                    }
                }
            }
        }

        // Captures
        if (f > 0) {
            int cap = from - 9;
            if (cap >= 0 && enemy_side(board[cap], side)) {
                if (r == 1) {
                    add_move(list, from, cap, BQ);
                    add_move(list, from, cap, BR);
                    add_move(list, from, cap, BB);
                    add_move(list, from, cap, BN);
                } else {
                    add_move(list, from, cap, 0);
                }
            }
        }
        if (f < 7) {
            int cap = from - 7;
            if (cap >= 0 && enemy_side(board[cap], side)) {
                if (r == 1) {
                    add_move(list, from, cap, BQ);
                    add_move(list, from, cap, BR);
                    add_move(list, from, cap, BB);
                    add_move(list, from, cap, BN);
                } else {
                    add_move(list, from, cap, 0);
                }
            }
        }
    }
}

static void gen_slider_moves(int from, MoveList& list, int side,
                             const int* DF, const int* DR, int dirCount) {
    int fromF = file_of(from);
    int fromR = rank_of(from);

    for (int d = 0; d < dirCount; d++) {
        int f = fromF + DF[d];
        int r = fromR + DR[d];

        while (f >= 0 && f < 8 && r >= 0 && r < 8) {
            int to = sq(f, r);
            int target = board[to];

            if (target == EMPTY) {
                add_move(list, from, to, 0);
            } else {
                if (enemy_side(target, side)) {
                    add_move(list, from, to, 0);
                }
                break;
            }

            f += DF[d];
            r += DR[d];
        }
    }
}

// Updated: Moves away from square delta check to mitigate reconverting file and rank:
static void gen_knight_moves(int from, MoveList& list) {
    int fromF = file_of(from);
    int fromR = rank_of(from);

    for (int i = 0; i < 8; i++) {
        int f = fromF + KNIGHT_DF[i];
        int r = fromR + KNIGHT_DR[i];
        if (f < 0 || f >= 8 || r < 0 || r >= 8) continue;

        int to = sq(f, r);
        int target = board[to];
        if (target != EMPTY && same_side(target, side_to_move)) continue;
        // Passed all checks
        add_move(list, from, to, 0);
    }
}

// Updated: Defined global DF and DR for sliding pieces, king and knight to pass:
static void gen_bishop_moves(int from, MoveList& list, int side) {
    gen_slider_moves(from, list, side, BISHOP_DF, BISHOP_DR, 4);
}

static void gen_rook_moves(int from, MoveList& list, int side) {
    gen_slider_moves(from, list, side, ROOK_DF, ROOK_DR, 4);
}

static void gen_queen_moves(int from, MoveList& list, int side) {
    gen_slider_moves(from, list, side, QUEEN_DF, QUEEN_DR, 8);
}

static void gen_king_moves(int from, MoveList& list, int side) {
    int fromF = file_of(from);
    int fromR = rank_of(from);

    // All 8 cardinal directions
    for (int i = 0; i < 8; i++) {
        int f = fromF + KING_DF[i];
        int r = fromR + KING_DR[i];
        if (f < 0 || f >= 8 || r < 0 || r >= 8) continue;

        int to = sq(f, r);
        int target = board[to];
        if (target != EMPTY && same_side(target, side)) continue;

        add_move(list, from, to, 0);
    }
}

void gen_moves(MoveList& list) {
    list.count = 0;
    for (int sq = 0; sq < 64; sq++) {
        int p = board[sq];
        if (p == EMPTY) continue;
        // Piece must be the same color as the side to move to generate the move:
        if (!same_side(p, side_to_move)) continue;
        if (p == WN || p == BN) gen_knight_moves(sq, list); // Knight
        else if (p == WP || p == BP) gen_pawn_moves(sq, list, side_to_move); // Pawn
        else if (p == WB || p == BB) gen_bishop_moves(sq, list, side_to_move); // Bishop
        else if (p == WR || p == BR) gen_rook_moves(sq, list, side_to_move); // Rook
        else if (p == WQ || p == BQ) gen_queen_moves(sq, list, side_to_move); // Queen
        else if (p == WK || p == BK) gen_king_moves(sq, list, side_to_move); // King
        // TODO: Castling, en passant, legality check
    }
}

// Return piece promoted
int promo_char_to_piece(char c, int side) {
    // UCI uses lowercase promotion letters q r b and n
    // Return the piece enum to place on destination square after a pawn promotes.
    // Piece color distinction matters here
    if (side == WHITE) {
        switch (c) {
            case 'q': return WQ;
            case 'r': return WR;
            case 'b': return WB;
            case 'n': return WN;
            default: return 0;
        }
    } else {
        switch (c) {
            case 'q': return BQ;
            case 'r': return BR;
            case 'b': return BB;
            case 'n': return BN;
            default: return 0;
        }
    }
}

bool parse_uci_move(const std::string& s, Move& out) {
    // Ex. input e2e4 or e7e8q (two squares back to back and optional promo square)
    if (s.size() != 4 && s.size() != 5) return false;

    int from = parse_square(s[0], s[1]);
    int to = parse_square(s[2], s[3]);
    if (from < 0 || to < 0) return false; // Either parse_square is invalid

    // Move struct declared in defs earlier - from, to, promo
    out.from = static_cast<uint8_t>(from);
    out.to = static_cast<uint8_t>(to);
    out.promo = 0; // No promo unless...

    if (s.size() == 5) { // Size is 5 -> promo
        int promoPiece = promo_char_to_piece(s[4], side_to_move);
        if (promoPiece == 0) return false;
        // Store promotion piece enum
        out.promo = static_cast<uint8_t>(promoPiece);
    }
    return true;
}

// Updated make_move that uses Undo struct and pushes back onto history stack
bool make_move(const Move& m) {
    int from = m.from;
    int to   = m.to;
    if (from < 0 || from >= 64 || to < 0 || to >= 64) return false; // Not a valid square

    int piece = board[from];
    if (piece == EMPTY) return false; // Nothing to move

    Undo u;
    u.from = (uint8_t)from; u.to = (uint8_t)to;
    u.moved = piece;
    u.captured = board[to];
    u.promo = m.promo;
    u.prev_side = side_to_move;

    // Apply
    board[from] = EMPTY;
    if (m.promo != 0) {
        board[to] = (int)m.promo; // Not sure if int cast needed here, take look later
    } else {
        board[to] = piece;
    }

    side_to_move = (side_to_move == WHITE ? BLACK : WHITE);

    history.push_back(u);
    return true;
}

// Undoes make_move from above by popping back of history vector:
bool undo_move() {
    if (history.empty()) return false; // Nothing to pop

    Undo u = history.back();
    history.pop_back();

    // Restore side first (important for promotion undo)
    side_to_move = u.prev_side;
    // Restore destination
    board[u.to] = u.captured;

    // Restore source
    if (u.promo != 0) {
        // If a promotion happened, the piece that moved was a pawn of inactive player:
        board[u.from] = (side_to_move == WHITE) ? WP : BP;
    } else {
        board[u.from] = u.moved; // Restore position
    }
    return true;
}

void init() {}

// This shouldn't be parsing any incomplete FEN (throws false if so?)
bool set_fen(const char* fen) {
    // For now: parse piece placement + side to move only.
    // Format: "<pieces> <side> ..." (we ignore castling/ep/halfmove/fullmove for now)

    // Clear the board and history
    for (int i = 0; i < 64; ++i) board[i] = EMPTY;
    clear_history();

    int file = 0;
    int rank = 7; // FEN starts at rank 8, for this code it is 0-7 inclusive
    const char* p = fen;

    // Place pieces:
    while (*p && *p != ' ') { // Stop at end of string or when hitting space character
        char c = *p++;
        if (c == '/') {
            if (file != 8) return false;  // Complete file
            rank--;
            file = 0;
            if (rank < 0) return false;
            continue;
        }

        if (std::isdigit(static_cast<unsigned char>(c))) {
            int empty = c - '0';
            if (empty < 1 || empty > 8) return false;
            file += empty;
            if (file > 8) return false;
            continue;
        }

        int piece = char_to_piece(c);
        if (piece < 0) return false;

        if (file >= 8 || rank < 0) return false;
        board[sq(file, rank)] = piece;
        file++;
    }

    // Must end last rank cleanly
    if (rank != 0 || file != 8) {
        // If we haven't processed all squares, something is wrong:
        return false;
    }

    if (*p != ' ') return false;
    p++; // Skip the space

    // Which side?
    if (*p == 'w') side_to_move = WHITE;
    else if (*p == 'b') side_to_move = BLACK;
    else return false;

    return true;
}

void set_startpos() {
    // Standard start position FEN
    const char* start = "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";
    (void)set_fen(start);
}
