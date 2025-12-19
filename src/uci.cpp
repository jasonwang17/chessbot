#include "defs.h"
#include <iostream>
#include <sstream>
#include <string>

static void dump_board() {
    for (int r = 7; r >= 0; --r) {
        std::cout << (r + 1) << "  ";
        for (int f = 0; f < 8; ++f) {
            std::cout << piece_to_char(board[r * 8 + f]) << ' ';
        }
        std::cout << '\n';
    }
    std::cout << "\n   a b c d e f g h\n";
    std::cout << "side: " << (side_to_move == WHITE ? "w" : "b") << '\n';
}

static void handle_position(const std::string& line) {
    std::istringstream iss(line);
    std::string word;
    std::string tok;
    iss >> word; iss >> word; // Position, then "startpos" or "fen"
    bool hasMoves = false; // Detect if we are given "moves" afterwards
    if (word == "startpos") {
        set_startpos();
        while (iss >> tok) {
            if (tok == "moves") { hasMoves = true; break; }
        }
        if (!hasMoves) return;
    } else if (word == "fen") {
        // Read fen fields until "moves" or end
        std::string fen;
        while (iss >> tok) {
            if (tok == "moves") { hasMoves = true; break; }
            if (!fen.empty()) fen += ' ';
            fen += tok; // Construct FEN
        }
        if (!set_fen(fen.c_str())) {
            std::cerr << "Invalid FEN\n"; // Debug
            return;
        }
        // Iss is now positioned right after "moves".
    } else {
        // Bad formatting
        return;
    }
    if (hasMoves) {
        while (iss >> tok) {
            Move m; // Create Move struct and validate move
            // TODO: Validate piece moveset later
            if (!parse_uci_move(tok, m)) return;
            if (!make_move(m)) return; // TODO: Should become the newer ver. later
        }
    }
}

void uci_loop() {
    std::string line;
    while (std::getline(std::cin, line)) {
        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if (cmd == "uci") {
            std::cout << "id name ChessBot\n";
            std::cout << "uciok\n";
        } else if (cmd == "isready") {
            std::cout << "readyok\n";
        } else if (cmd == "ucinewgame") {
            set_startpos();
        } else if (cmd == "d") {
            dump_board();
        } else if (cmd == "position") {
            handle_position(line);
        } else if (cmd == "u") {
            undo_move();
        } else if (cmd == "go") {
            std::cout << "bestmove e2e4\n";
        } else if (cmd == "quit") {
            break;
        }
    }
}