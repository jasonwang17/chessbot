#include "defs.h"

#include <vector>
#include <string>
#include <cstdint>

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
    return p >= WP && p <= WK;
}

static bool is_black(int p) {
    return p >= BP && p <= BK;
}

static bool same_side(int p, int side) {
    return (side == WHITE) ? is_white(p) : is_black(p);
}

static bool enemy_side(int p, int side) {
    return (side == WHITE) ? is_black(p) : is_white(p);
}

// Movegen
static const int knight_offsets[8] = {17, 15, 10, 6,
                                     -6, -10, -15,-17};

static void gen_knight_moves(int from, MoveList& list) {
    int from_file = file_of(from);
    int from_rank = rank_of(from);

    for (int i = 0; i < 8; i++) {
        int to = from + knight_offsets[i]; // Check for this value of to that:
        if (to < 0 || to >= 64) continue; // It is within the valid squares

        // Check that knight move behavior follows 1 by 2 movement
        int to_file = file_of(to);
        int to_rank = rank_of(to);
        int df = to_file - from_file;
        int dr = to_rank - from_rank;

        if (!((abs(df) == 1 && abs(dr) == 2) ||
              (abs(df) == 2 && abs(dr) == 1)))
            continue;

        int target = board[to];
        // Friendly piece blocks
        if (target != EMPTY && same_side(target, side_to_move))
            continue;

        // TODO: Pin, check, other checks before adding to moveset

        // Add move (passed all previous checks)
        Move m;
        m.from = from;
        m.to = to;
        m.promo = 0;
        list.moves[list.count++] = m;
    }
}

void gen_moves(MoveList& list) {
    list.count = 0;
    for (int sq = 0; sq < 64; sq++) {
        int p = board[sq];
        if (p == EMPTY) continue;
        if (!same_side(p, side_to_move)) continue;
        switch (p) {
            case WN:
            case BN:
                gen_knight_moves(sq, list);
                break;
                // other pieces later
        }
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
