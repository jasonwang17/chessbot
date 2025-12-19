#include "defs.h"
#include <cctype>
#include <cstdio>

int board[64] = {0};
int side_to_move = WHITE;

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

// Validate square passed by character value of file (a-h) and rank (1-8)
int parse_square(const char fileChar, const char rankChar) {
    if (fileChar < 'a' || fileChar > 'h') return -1;
    if (rankChar < '1' || rankChar > '8') return -1;
    int file = fileChar - 'a';
    int rank = rankChar - '1';
    return sq(file, rank);
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

bool make_move_basic(const Move& m) {
    int from = m.from;
    int to = m.to;
    if (from < 0 || from >= 64 || to < 0 || to >= 64) return false; // Not a valid square

    int piece = board[from];
    if (piece == EMPTY) return false; // Nothing to move

    board[from] = EMPTY;

    if (m.promo != 0) {
        board[to] = m.promo; // Overwrite with promoted piece
    } else {
        board[to] = piece;
    }

    side_to_move = (side_to_move == WHITE ? BLACK : WHITE); // Flip side
    return true;
}


void init() {}

// This shouldn't be parsing any incomplete FEN (throws false if so?)
bool set_fen(const char* fen) {
    // For now: parse piece placement + side to move only.
    // Format: "<pieces> <side> ..." (we ignore castling/ep/halfmove/fullmove for now)

    // Clear the board
    for (int i = 0; i < 64; ++i) board[i] = EMPTY;

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
    const char* start =
            "rnbqkbnr/pppppppp/8/8/8/8/PPPPPPPP/RNBQKBNR w KQkq - 0 1";

    (void)set_fen(start);
}
