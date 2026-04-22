#include "core/CubeState.h"
#include "core/Move.h"
#include "solver/Solver.h"
#include "utils/Logger.h"

#include <nlohmann/json.hpp>
#include <spdlog/spdlog.h>

#include <cstdio>
#include <string>
#include <vector>

using namespace rubik;

namespace {

std::vector<Move> parseSequence(const std::string& sequence) {
    std::vector<Move> moves;
    moves.reserve(sequence.size());
    for (char c : sequence) {
        auto move = Move::fromChar(c);
        if (!move) {
            std::fprintf(stderr, "invalid move character: %c\n", c);
            std::exit(2);
        }
        moves.push_back(*move);
    }
    return moves;
}

std::string movesToString(const std::vector<Move>& moves) {
    std::string out;
    out.reserve(moves.size());
    for (Move move : moves) out.push_back(move.toChar());
    return out;
}

} // namespace

int main(int argc, char** argv) {
    Logger::init();
    spdlog::set_level(spdlog::level::err);

    if (argc != 2) {
        std::fprintf(stderr, "usage: solver_cli <scramble-sequence>\n");
        return 2;
    }

    const std::string scrambleArg = argv[1];
    const auto scramble = parseSequence(scrambleArg);

    CubeState scrambled;
    for (Move move : scramble) scrambled.applyMove(move);

    const auto report = Solver::solveWithReport(scrambled);

    CubeState verified = scrambled;
    for (Move move : report.moves) verified.applyMove(move);

    nlohmann::json json = {
        {"scramble", scrambleArg},
        {"solution", movesToString(report.moves)},
        {"solutionLength", report.moves.size()},
        {"rawLength", report.rawLen},
        {"phasePassed", report.phasePassed},
        {"solverReportedSolved", report.solved},
        {"verifiedSolved", verified.isSolved()}
    };

    std::puts(json.dump().c_str());
    return 0;
}
