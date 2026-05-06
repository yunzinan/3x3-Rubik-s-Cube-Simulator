#include "core/CubeState.h"
#include "core/Move.h"
#include "solver/Solver.h"
#include "utils/Logger.h"

#include <algorithm>
#include <climits>
#include <cstdio>
#include <random>
#include <string>
#include <vector>

using namespace rubik;

namespace {

std::vector<Move> randomScramble(std::mt19937& rng, int len) {
    static const Face faces[] = {Face::F, Face::B, Face::U, Face::D, Face::L, Face::R};
    std::uniform_int_distribution<int> faceDist(0, 5);
    std::uniform_int_distribution<int> dirDist(0, 1);
    std::vector<Move> out;
    out.reserve(len);
    int lastFace = -1;
    for (int i = 0; i < len; ++i) {
        int f;
        do { f = faceDist(rng); } while (f == lastFace);
        lastFace = f;
        Move m{faces[f], dirDist(rng) ? Direction::CW : Direction::CCW};
        out.push_back(m);
    }
    return out;
}

std::string movesToString(const std::vector<Move>& ms) {
    std::string s;
    s.reserve(ms.size());
    for (auto m : ms) s.push_back(m.toChar());
    return s;
}

void applySeq(CubeState& state, const char* seq) {
    for (const char* p = seq; *p; ++p) {
        if (auto m = Move::fromChar(*p)) state.applyMove(*m);
    }
}

bool sameFacelets(const FaceletColor a[6][9], const FaceletColor b[6][9]) {
    for (int f = 0; f < 6; ++f)
        for (int s = 0; s < 9; ++s)
            if (a[f][s] != b[f][s]) return false;
    return true;
}

bool hasNineOfEachColor(const FaceletColor facelets[6][9]) {
    int counts[6] = {};
    for (int f = 0; f < 6; ++f)
        for (int s = 0; s < 9; ++s)
            ++counts[int(facelets[f][s])];
    for (int i = 0; i < 6; ++i) {
        if (counts[i] != 9) {
            for (int j = 0; j < 6; ++j)
                std::printf("facelet color %d count: %d\n", j, counts[j]);
            return false;
        }
    }
    return true;
}

} // namespace

int main() {
    Logger::init();
    spdlog::set_level(spdlog::level::warn);

    const int kTrials    = 50;
    const int kScrambLen = 30;

    std::mt19937 rng(0xC0FFEE);

    int passed  = 0;
    long long totalLen = 0;
    int minLen = INT_MAX;
    int maxLen = 0;
    int phasePassCount[7] = {0};
    std::vector<Move> firstFailScramble;
    std::vector<Move> firstFailSolution;
    int firstFailFirstBadPhase = -1;

    for (int trial = 0; trial < kTrials; ++trial) {
        CubeState state;
        auto scramble = randomScramble(rng, kScrambLen);
        for (auto m : scramble) state.applyMove(m);

        FaceletColor facelets[6][9];
        state.toFacelets(facelets);
        if (!hasNineOfEachColor(facelets)) {
            std::printf("facelet export produced invalid counts for scramble: %s\n",
                        movesToString(scramble).c_str());
            return 1;
        }
        CubeState reconstructed;
        if (!reconstructed.applyFaceletInput(facelets)) {
            std::printf("facelet reconstruction rejected scramble: %s\n",
                        movesToString(scramble).c_str());
            return 1;
        }
        FaceletColor roundTrip[6][9];
        reconstructed.toFacelets(roundTrip);
        if (!sameFacelets(facelets, roundTrip)) {
            std::printf("facelet reconstruction changed scramble: %s\n",
                        movesToString(scramble).c_str());
            return 1;
        }

        auto report = Solver::solveWithReport(state);

        for (int p = 0; p < 7; ++p) if (report.phasePassed[p]) ++phasePassCount[p];

        CubeState verify = state;
        for (auto m : report.moves) verify.applyMove(m);

        const int len = (int)report.moves.size();
        totalLen += len;
        if (len < minLen) minLen = len;
        if (len > maxLen) maxLen = len;

        if (verify.isSolved()) {
            ++passed;
        } else if (firstFailScramble.empty()) {
            firstFailScramble = scramble;
            firstFailSolution = report.moves;
            for (int p = 0; p < 7; ++p) {
                if (!report.phasePassed[p]) { firstFailFirstBadPhase = p + 1; break; }
            }
        }
    }

    const char* names[] = { "whiteCross", "whiteCorners", "middleEdges",
                            "yellowCross", "permYellowEdges",
                            "permYellowCorners", "orientYellowCorners" };

    std::printf("=== Solver harness ===\n");
    std::printf("trials:  %d\n", kTrials);
    std::printf("passed:  %d / %d  (%.1f%%)\n",
                passed, kTrials, 100.0 * passed / kTrials);
    std::printf("solution length — min=%d  avg=%.1f  max=%d\n",
                minLen == INT_MAX ? 0 : minLen,
                (double)totalLen / kTrials, maxLen);
    std::printf("\nPer-phase subgoal pass counts:\n");
    for (int p = 0; p < 7; ++p) {
        std::printf("  phase %d (%s): %d / %d\n",
                    p + 1, names[p], phasePassCount[p], kTrials);
    }

    if (!firstFailScramble.empty()) {
        std::printf("\nFirst failing case (first-bad phase = %d):\n",
                    firstFailFirstBadPhase);
        std::printf("  scramble (%zu): %s\n",
                    firstFailScramble.size(),
                    movesToString(firstFailScramble).c_str());
        std::printf("  solution (%zu): %s\n",
                    firstFailSolution.size(),
                    movesToString(firstFailSolution).c_str());
    }

    return (passed == kTrials) ? 0 : 1;
}
