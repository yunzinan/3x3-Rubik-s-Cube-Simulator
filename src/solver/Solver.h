#pragma once

#include "core/CubeState.h"
#include "core/Move.h"

#include <array>
#include <vector>

namespace rubik {

// Layer-by-layer (beginner's method) cube solver.
//
// Convention: the *solved* state has white on the D face (y = -1) and yellow
// on U (y = +1). The solver produces a sequence of 90° face turns (no 180°
// moves) that transforms `state` into the solved state when applied in order.
//
// Returns an empty vector if the input is already solved.  On failure (which
// should not happen for any legal state reachable from solved via 90° turns)
// the solver logs a warning and returns its partial sequence.
class Solver {
public:
    static std::vector<Move> solve(const CubeState& state);

    // 7-phase LBL subgoal report for diagnostics / testing.
    struct SolveReport {
        std::vector<Move> moves;              // simplified solution
        std::size_t       rawLen = 0;         // pre-simplification length
        std::array<bool, 7> phasePassed{};    // subgoal met after phase k
        bool              solved = false;
    };
    static SolveReport solveWithReport(const CubeState& state);
};

} // namespace rubik
