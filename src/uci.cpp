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

    iss >> word; // Position
    iss >> word; // This is "startpos" or "fen"

    if (word == "startpos") {
        set_startpos();
    } else if (word == "fen") {
        // Read fen fields until "moves" or end
        std::string fen, tok;
        while (iss >> tok) {
            if (tok == "moves") break;
            if (!fen.empty()) fen += ' ';
            fen += tok;
        }
        if (!set_fen(fen.c_str())) {
            std::cerr << "Invalid FEN\n";
        }
        // Iss is now positioned right after "moves".
    } else {
        // Bad formatting
        return;
    }

    // TODO: Implement move handling from the UCI parse and promo functions in board.cpp
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
        } else if (cmd == "go") {
            std::cout << "bestmove e2e4\n";
        } else if (cmd == "quit") {
            break;
        }
    }
}