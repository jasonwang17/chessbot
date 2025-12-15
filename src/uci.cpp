#include "defs.h"
#include <iostream>
#include <sstream>
#include <string>

static void dump_board() {
    for (int r = 7; r >= 0; --r) {
        for (int f = 0; f < 8; ++f) {
            std::cout << board[r * 8 + f] << ' ';
        }
        std::cout << "\n";
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
        } else if (cmd == "go") {
            std::cout << "bestmove e2e4\n";
        } else if (cmd == "quit") {
            break;
        }
    }
}