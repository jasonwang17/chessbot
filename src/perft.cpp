#include "defs.h"
#include <cstdint>
#include <cstdio>

// Count the number of nodes at a certain depth to make sure movegen is working in full
uint64_t perft(int depth) {
    if (depth == 0) return 1;

    MoveList moves;
    gen_legal_moves(moves);

    uint64_t nodes = 0;
    for (int i = 0; i < moves.count; i++) {
        make_move(moves.moves[i]);
        nodes += perft(depth - 1);
        undo_move();
    }
    return nodes;
}