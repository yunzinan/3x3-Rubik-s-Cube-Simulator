#include "solver/Solver.h"

#include "utils/Logger.h"

#include <Magnum/Math/Matrix3.h>
#include <Magnum/Math/Vector3.h>

#include <array>
#include <cmath>
#include <queue>
#include <string>
#include <unordered_map>
#include <utility>

namespace rubik {

using namespace Magnum;
using V3i = Vector3i;

namespace {

// ── Rotation helpers ─────────────────────────────────────────────────────────
// Rotate a face label by +90° around +Y, `count` times.  F→R→B→L→F cycle.
// U and D are unchanged.  Used so that an algorithm written for one "target"
// orientation can be re-applied to a symmetric rotated target.
Face rotFaceY(Face f, int count) {
    int c = ((count % 4) + 4) % 4;
    for (int i = 0; i < c; ++i) {
        switch (f) {
            case Face::F: f = Face::R; break;
            case Face::R: f = Face::B; break;
            case Face::B: f = Face::L; break;
            case Face::L: f = Face::F; break;
            default: break;
        }
    }
    return f;
}

// Rotate an (x, z) pair by +90° around +Y applied `count` times.  (x,z)→(z,−x).
V3i rotPosY(V3i v, int count) {
    int c = ((count % 4) + 4) % 4;
    for (int i = 0; i < c; ++i) v = V3i{v.z(), v.y(), -v.x()};
    return v;
}

// ── Working state ───────────────────────────────────────────────────────────
struct Work {
    CubeState s;
    std::vector<Move> out;

    void apply(Move m) {
        s.applyMove(m);
        out.push_back(m);
    }

    // Apply a character: lowercase = CW, uppercase = CCW (per project convention).
    void apply(char c) {
        if (auto m = Move::fromChar(c)) apply(*m);
    }

    // Apply a sequence of chars.  `yRot` rotates each move's face by +90°·yRot
    // around +Y, so algorithms can be written canonically and reused.
    void applySeq(const char* seq, int yRot = 0) {
        for (const char* p = seq; *p; ++p) {
            if (auto m = Move::fromChar(*p)) {
                apply(Move{rotFaceY(m->face, yRot), m->direction});
            }
        }
    }

    int findByHome(V3i home) const {
        const auto& cs = s.cubies();
        for (int i = 0; i < (int)cs.size(); ++i)
            if (cs[i].homePosition == home) return i;
        return -1;
    }
    int findAt(V3i pos) const {
        const auto& cs = s.cubies();
        for (int i = 0; i < (int)cs.size(); ++i)
            if (cs[i].position == pos) return i;
        return -1;
    }

    // World direction currently occupied by the sticker that originally faced
    // `homeDir` on this cubie.  This is our stand-in for sticker colors.
    V3i stickerFacing(int idx, V3i homeDir) const {
        const Matrix3 rot3 = s.cubies()[idx].rotation.rotationScaling();
        const Vector3 fd{Float(homeDir.x()), Float(homeDir.y()), Float(homeDir.z())};
        const Vector3 wd = rot3 * fd;
        return V3i{Int(std::round(wd.x())),
                   Int(std::round(wd.y())),
                   Int(std::round(wd.z()))};
    }
};

void applySeq(CubeState& state, const char* seq, int yRot = 0) {
    for (const char* p = seq; *p; ++p) {
        if (auto m = Move::fromChar(*p)) {
            state.applyMove(Move{rotFaceY(m->face, yRot), m->direction});
        }
    }
}

// ── Coordinate helpers ──────────────────────────────────────────────────────
const V3i kDownDir{0, -1, 0};
const V3i kUpDir  {0,  1, 0};

// Which side face is a D-edge's home on?  home = (x, -1, z) with |x|+|z| == 1.
Face sideOfDEdge(V3i home) {
    if (home.z() ==  1) return Face::F;
    if (home.z() == -1) return Face::B;
    if (home.x() ==  1) return Face::R;
    return Face::L;
}

// Rotation count for a given D-edge home so that +90°·count around +Y rotates
// the FD edge (0,-1,1) to that home.
int countForDEdge(V3i home) {
    if (home.z() ==  1 && home.x() == 0) return 0; // FD
    if (home.x() ==  1 && home.z() == 0) return 1; // RD  (rotPosY once of FD = (1,-1,0))
    if (home.z() == -1 && home.x() == 0) return 2; // BD
    return 3; // LD
}

// Similarly for D-corners.  Canonical FRD home = (1,-1,1); count k rotates to
// the k-th slot going +Y: FRD(0)→BRD(1)→BLD(2)→FLD(3).
int countForDCorner(V3i home) {
    if (home.x() ==  1 && home.z() ==  1) return 0; // FRD
    if (home.x() ==  1 && home.z() == -1) return 1; // BRD
    if (home.x() == -1 && home.z() == -1) return 2; // BLD
    return 3;                                        // FLD
}

// And for middle-layer edges (y == 0).
int countForEEdge(V3i home) {
    if (home.x() ==  1 && home.z() ==  1) return 0; // FR (canonical)
    if (home.x() ==  1 && home.z() == -1) return 1; // BR
    if (home.x() == -1 && home.z() == -1) return 2; // BL
    return 3;                                        // FL
}

int countForUEdge(V3i home) {
    if (home.z() ==  1 && home.x() == 0) return 0; // UF
    if (home.x() ==  1 && home.z() == 0) return 1; // UR
    if (home.z() == -1 && home.x() == 0) return 2; // UB
    return 3;                                      // UL
}

int countForUCorner(V3i home) {
    if (home.x() ==  1 && home.z() ==  1) return 0; // UFR
    if (home.x() ==  1 && home.z() == -1) return 1; // UBR
    if (home.x() == -1 && home.z() == -1) return 2; // UBL
    return 3;                                        // UFL
}

// ── Phase 1: White cross on D ───────────────────────────────────────────────
void solveOneWhiteEdge(Work& w, V3i target) {
    const V3i aboveTarget{target.x(), 1, target.z()};
    const V3i targetSideDir{target.x(), 0, target.z()};
    const int tc = countForDEdge(target);

    for (int iter = 0; iter < 20; ++iter) {
        int idx = w.findByHome(target);
        V3i pos = w.s.cubies()[idx].position;
        V3i whiteDir = w.stickerFacing(idx, kDownDir);

        if (pos == target && whiteDir == kDownDir) return;

        // ── Case D: in D layer but wrong slot or wrong orientation.
        if (pos.y() == -1) {
            Face side;
            if (pos.z() ==  1)       side = Face::F;
            else if (pos.z() == -1)  side = Face::B;
            else if (pos.x() ==  1)  side = Face::R;
            else                     side = Face::L;
            w.apply(Move{side, Direction::CW});
            w.apply(Move{side, Direction::CW});
            continue;
        }

        // ── Case E: in middle layer.  Lift to U using a 3-move pattern that
        // restores the adjacent D-edge (side · U · side').  A single face
        // quarter-turn would permanently kick out a previously-placed D-edge.
        if (pos.y() == 0) {
            if (pos.x() == 1) {
                if (pos.z() == 1) w.applySeq("ruR");   // FR: R U R'
                else              w.applySeq("RUr");   // BR: R' U' R
            } else {
                if (pos.z() == 1) w.applySeq("LUl");   // FL: L' U' L
                else              w.applySeq("luL");   // BL: L U L'
            }
            continue;
        }

        // ── Case U: in top layer.
        if (whiteDir == kUpDir) {
            // White is up.  Rotate U until above target, then insert with
            // side-face CW twice (equivalent to F2 but using two 90° turns).
            for (int k = 0; k < 4; ++k) {
                if (w.s.cubies()[w.findByHome(target)].position == aboveTarget) break;
                w.apply(Move{Face::U, Direction::CW});
            }
            Face side = sideOfDEdge(target);
            w.apply(Move{side, Direction::CW});
            w.apply(Move{side, Direction::CW});
        } else {
            // White facing a side direction.  Rotate U until the white sticker
            // points outward at the target's side direction, then apply the
            // flipped-insertion algorithm.  Canonical alg for FD target:
            //   U' R' F R  → in our chars: "URfr".
            for (int k = 0; k < 4; ++k) {
                int ii = w.findByHome(target);
                if (w.stickerFacing(ii, kDownDir) == targetSideDir) break;
                w.apply(Move{Face::U, Direction::CW});
            }
            w.applySeq("URfr", tc);
        }
    }
    LOG_WARN("solveOneWhiteEdge: iteration cap hit");
}

void solveWhiteCross(Work& w) {
    solveOneWhiteEdge(w, V3i{ 0, -1,  1}); // FD
    solveOneWhiteEdge(w, V3i{ 1, -1,  0}); // RD
    solveOneWhiteEdge(w, V3i{ 0, -1, -1}); // BD
    solveOneWhiteEdge(w, V3i{-1, -1,  0}); // LD
}

// ── Phase 2: White corners ──────────────────────────────────────────────────
void solveOneWhiteCorner(Work& w, V3i target) {
    const V3i aboveTarget{target.x(), 1, target.z()};
    const int tc = countForDCorner(target);

    for (int iter = 0; iter < 30; ++iter) {
        int idx = w.findByHome(target);
        V3i pos = w.s.cubies()[idx].position;
        V3i whiteDir = w.stickerFacing(idx, kDownDir);

        if (pos == target && whiteDir == kDownDir) return;

        if (pos.y() == -1) {
            // Misplaced D-corner.  Kick it to U layer using sexy move rotated
            // to that slot: canonical R U R' U' ("ruRU") applied with yRot of
            // the slot count.  Result: corner now in U.
            int pc = countForDCorner(pos);
            w.applySeq("ruRU", pc);
            continue;
        }

        // In U layer.  Rotate U until corner is directly above its target slot.
        for (int k = 0; k < 4; ++k) {
            if (w.s.cubies()[w.findByHome(target)].position == aboveTarget) break;
            w.apply(Move{Face::U, Direction::CW});
        }
        // One sexy move rotated for the target slot.  Each application cycles
        // the corner's orientation; loop will terminate once aligned correctly.
        w.applySeq("ruRU", tc);
    }
    LOG_WARN("solveOneWhiteCorner: iteration cap hit");
}

void solveWhiteCorners(Work& w) {
    solveOneWhiteCorner(w, V3i{ 1, -1,  1}); // FRD
    solveOneWhiteCorner(w, V3i{ 1, -1, -1}); // BRD
    solveOneWhiteCorner(w, V3i{-1, -1, -1}); // BLD
    solveOneWhiteCorner(w, V3i{-1, -1,  1}); // FLD
}

// ── Phase 3: Middle-layer edges ─────────────────────────────────────────────
// Target edges: FR, BR, BL, FL (y == 0, no yellow sticker).
// Strategy for each target slot T with canonical rotation tc:
//   - If edge is already placed and oriented, skip.
//   - If edge is in middle layer but wrong, kick it out with a right-insert at
//     its current slot.  The edge is now somewhere in the U layer.
//   - Otherwise the edge is in U layer.  Rotate U (0..3 times) until one of
//     the two ready-states holds, then apply the matching insert:
//       Right-insert (edge at UF, A-sticker up, B-sticker forward):
//           U R U' R' U' F' U F          = "urURUFuf"
//       Left-insert  (edge at UR, A-sticker right, B-sticker up):
//           U' F' U F U R U' R'          = "UFufurUR"
//   After 4 rotations one of the two ready-states must hold because the two
//   insert-pre-orbits partition the 8 (position × orientation) states.
void solveOneMiddleEdge(Work& w, V3i target) {
    const int tc = countForEEdge(target);

    for (int iter = 0; iter < 20; ++iter) {
        int idx = w.findByHome(target);
        V3i pos = w.s.cubies()[idx].position;

        // The rotated canonical algorithms distinguish the two stickers on the
        // target edge. For odd y-rotations, the slot still matches the same
        // edge piece but the "right" and "front" sticker roles swap, so we
        // must rotate the canonical home directions instead of deriving them
        // directly from the target coordinates.
        const V3i canonRightHome = rotPosY(V3i{1, 0, 0}, tc);
        const V3i canonFrontHome = rotPosY(V3i{0, 0, 1}, tc);

        if (pos == target) {
            V3i sRight = w.stickerFacing(idx, canonRightHome);
            V3i sFront = w.stickerFacing(idx, canonFrontHome);
            if (sRight == canonRightHome && sFront == canonFrontHome) return;
        }

        if (pos.y() == 0) {
            // Edge is in middle layer but not solved.  Kick it out.
            int pc = countForEEdge(pos);
            w.applySeq("urURUFuf", pc);
            continue;
        }

        const V3i posRight     = rotPosY(V3i{0, 1, 1}, tc); // canonical UF
        const V3i posLeft      = rotPosY(V3i{1, 1, 0}, tc); // canonical UR
        const V3i sideDirFront = rotPosY(V3i{0, 0, 1}, tc); // target's +Z world dir
        const V3i sideDirRight = rotPosY(V3i{1, 0, 0}, tc); // target's +X world dir

        bool inserted = false;
        for (int k = 0; k < 4; ++k) {
            int ii = w.findByHome(target);
            V3i cp  = w.s.cubies()[ii].position;
            V3i csRight = w.stickerFacing(ii, canonRightHome);
            V3i csFront = w.stickerFacing(ii, canonFrontHome);

            // Right-insert ready state: edge at UF (posRight) with
            //   homeA sticker up (+Y), homeB sticker facing target's +Z.
            // Canonical FR: R-color up, F-color forward.
            if (cp == posRight && csRight == kUpDir && csFront == sideDirFront) {
                w.applySeq("urURUFuf", tc);
                inserted = true;
                break;
            }
            // Left-insert ready state: edge at UR (posLeft) with
            //   homeA sticker facing target's +X, homeB sticker up (+Y).
            // Canonical FR: R-color right, F-color up.
            if (cp == posLeft && csRight == sideDirRight && csFront == kUpDir) {
                w.applySeq("UFufurUR", tc);
                inserted = true;
                break;
            }
            w.apply(Move{Face::U, Direction::CW});
        }
        if (!inserted) {
            // Shouldn't happen: the two ready-orbits cover all U-layer states.
            // Apply a kick to perturb and retry.
            w.applySeq("urURUFuf", tc);
        }
    }
    LOG_WARN("solveOneMiddleEdge: iteration cap hit");
}

void solveMiddleEdges(Work& w) {
    solveOneMiddleEdge(w, V3i{ 1, 0,  1}); // FR
    solveOneMiddleEdge(w, V3i{ 1, 0, -1}); // BR
    solveOneMiddleEdge(w, V3i{-1, 0, -1}); // BL
    solveOneMiddleEdge(w, V3i{-1, 0,  1}); // FL
}

// ── Phase 4: Yellow cross on U ──────────────────────────────────────────────
// Count/detect yellow-up slots on the U face. At this point the whole top
// layer consists of U-layer cubies, but the pieces are still permuted, so the
// pattern must be read from positions, not from cubie home slots.
int countYellowUpEdges(const Work& w) {
    int n = 0;
    for (V3i pos : {V3i{0,1,1}, V3i{1,1,0}, V3i{0,1,-1}, V3i{-1,1,0}}) {
        int i = w.findAt(pos);
        if (w.stickerFacing(i, kUpDir) == kUpDir) ++n;
    }
    return n;
}

bool yellowUpAtSlot(const Work& w, V3i pos) {
    int i = w.findAt(pos);
    return w.stickerFacing(i, kUpDir) == kUpDir;
}

void solveYellowCross(Work& w) {
    // Canonical algorithm: F R U R' U' F' = "fruRUF" (6 chars).
    for (int iter = 0; iter < 8; ++iter) {
        int n = countYellowUpEdges(w);
        if (n == 4) return;

        if (n == 0) {
            // Dot.  Apply once, any orientation.
            w.applySeq("fruRUF");
            continue;
        }
        if (n == 2) {
            // Line or L.  Rotate U to bring into canonical orientation.
            // Line: two opposite yellow-up edges.  Canonical: UL and UR (x=±1, z=0).
            // L:    two adjacent yellow-up edges.  Canonical: UB and UL.
            bool LR = yellowUpAtSlot(w, V3i{ 1,1, 0}) && yellowUpAtSlot(w, V3i{-1,1,0});
            bool FB = yellowUpAtSlot(w, V3i{ 0,1, 1}) && yellowUpAtSlot(w, V3i{ 0,1,-1});
            if (FB) { w.apply(Move{Face::U, Direction::CW}); continue; }
            if (LR) { w.applySeq("fruRUF"); continue; }
            // L-shape.  Find which two adjacent edges are up, rotate U until
            // they are at UB and UL.  Canonical L = yellow at UB (0,1,-1) and
            // UL (-1,1,0).
            for (int k = 0; k < 4; ++k) {
                if (yellowUpAtSlot(w, V3i{0,1,-1}) && yellowUpAtSlot(w, V3i{-1,1,0})) break;
                w.apply(Move{Face::U, Direction::CW});
            }
            w.applySeq("fruRUF");
            continue;
        }
        // n == 1 or 3: inconsistent parity.  Still try the algorithm.
        w.applySeq("fruRUF");
    }
    LOG_WARN("solveYellowCross: iteration cap hit");
}

// ── Phase 5: Yellow edges permutation ───────────────────────────────────────
// After phase 4, all four U-edges have yellow up.  Permute so that each edge's
// side-sticker matches the adjacent center.  Canonical alg: R U R' U R U2 R'
// in chars "ruRuruuR" — cycles UL → UR → UB, fixing UF.
//
// Strategy: count matching edges; rotate U to bring matches to UB; apply.
int countMatchedUEdges(const Work& w) {
    int n = 0;
    for (V3i home : {V3i{0,1,1}, V3i{1,1,0}, V3i{0,1,-1}, V3i{-1,1,0}}) {
        int i = w.findByHome(home);
        if (w.s.cubies()[i].position == home) ++n;
    }
    return n;
}
bool matchedAt(const Work& w, V3i home) {
    int i = w.findByHome(home);
    return w.s.cubies()[i].position == home;
}

std::string uEdgePermKey(const CubeState& state) {
    std::string key;
    key.reserve(12);
    for (V3i home : {V3i{0,1,1}, V3i{1,1,0}, V3i{0,1,-1}, V3i{-1,1,0}}) {
        int idx = -1;
        const auto& cs = state.cubies();
        for (int i = 0; i < (int)cs.size(); ++i) {
            if (cs[i].homePosition == home) { idx = i; break; }
        }
        const auto& pos = cs[idx].position;
        key.push_back(char('1' + pos.x()));
        key.push_back(char('1' + pos.y()));
        key.push_back(char('1' + pos.z()));
    }
    return key;
}

void permuteYellowEdges(Work& w) {
    struct Node {
        CubeState state;
        int prev = -1;
        const char* seq = nullptr;
        int rot = 0;
    };
    struct Action { const char* seq; int rot; };
    std::vector<Action> actions = {
        {"u", 0},
        {"U", 0}
    };
    for (int rot = 0; rot < 4; ++rot) {
        actions.push_back({"ruRuruuR", rot});
        actions.push_back({"ruuRUrUR", rot});
    }

    std::vector<Node> nodes;
    nodes.push_back(Node{w.s, -1, nullptr, 0});
    std::unordered_map<std::string, int> seen;
    seen.emplace(uEdgePermKey(w.s), 0);
    std::queue<int> q;
    q.push(0);

    int solvedIndex = -1;
    while (!q.empty()) {
        const int current = q.front();
        q.pop();

        Work probe;
        probe.s = nodes[current].state;
        if (countMatchedUEdges(probe) == 4) {
            solvedIndex = current;
            break;
        }

        for (const auto& action : actions) {
            CubeState next = nodes[current].state;
            applySeq(next, action.seq, action.rot);
            auto [it, inserted] = seen.emplace(uEdgePermKey(next), (int)nodes.size());
            if (!inserted) continue;
            nodes.push_back(Node{std::move(next), current, action.seq, action.rot});
            q.push(it->second);
        }
    }

    if (solvedIndex < 0) {
        LOG_WARN("permuteYellowEdges: search failed");
        return;
    }

    std::vector<std::pair<const char*, int>> path;
    for (int i = solvedIndex; nodes[i].prev >= 0; i = nodes[i].prev) {
        path.push_back({nodes[i].seq, nodes[i].rot});
    }
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        w.applySeq(it->first, it->second);
    }
}

// ── Phase 6: Yellow corners permutation ─────────────────────────────────────
// Canonical alg: U R U' L' U R' U' L → "urULuRUl".  Cycles 3 U-corners while
// fixing UFR.  Strategy: find a correctly-positioned corner, rotate the
// algorithm to keep that slot fixed, apply; repeat until done.
bool cornerInPlace(const Work& w, V3i home) {
    int i = w.findByHome(home);
    return w.s.cubies()[i].position == home;
}
int countMatchedUCorners(const Work& w) {
    int n = 0;
    for (V3i home : {V3i{1,1,1}, V3i{1,1,-1}, V3i{-1,1,-1}, V3i{-1,1,1}}) {
        if (cornerInPlace(w, home)) ++n;
    }
    return n;
}

std::string uCornerPermKey(const CubeState& state) {
    std::string key;
    key.reserve(12);
    for (V3i home : {V3i{1,1,1}, V3i{1,1,-1}, V3i{-1,1,-1}, V3i{-1,1,1}}) {
        int idx = -1;
        const auto& cs = state.cubies();
        for (int i = 0; i < (int)cs.size(); ++i) {
            if (cs[i].homePosition == home) { idx = i; break; }
        }
        const auto& pos = cs[idx].position;
        key.push_back(char('1' + pos.x()));
        key.push_back(char('1' + pos.y()));
        key.push_back(char('1' + pos.z()));
    }
    return key;
}

void permuteYellowCorners(Work& w) {
    struct Node {
        CubeState state;
        int prev = -1;
        const char* seq = nullptr;
        int rot = 0;
    };
    struct Action { const char* seq; int rot; };

    std::vector<Action> actions;
    for (int rot = 0; rot < 4; ++rot) {
        actions.push_back({"urULuRUl", rot});
    }

    std::vector<Node> nodes;
    nodes.push_back(Node{w.s, -1, nullptr, 0});
    std::unordered_map<std::string, int> seen;
    seen.emplace(uCornerPermKey(w.s), 0);
    std::queue<int> q;
    q.push(0);

    int solvedIndex = -1;
    while (!q.empty()) {
        const int current = q.front();
        q.pop();

        Work probe;
        probe.s = nodes[current].state;
        if (countMatchedUCorners(probe) == 4) {
            solvedIndex = current;
            break;
        }

        for (const auto& action : actions) {
            CubeState next = nodes[current].state;
            applySeq(next, action.seq, action.rot);
            auto [it, inserted] = seen.emplace(uCornerPermKey(next), (int)nodes.size());
            if (!inserted) continue;
            nodes.push_back(Node{std::move(next), current, action.seq, action.rot});
            q.push(it->second);
        }
    }

    if (solvedIndex < 0) {
        LOG_WARN("permuteYellowCorners: search failed");
        return;
    }

    std::vector<std::pair<const char*, int>> path;
    for (int i = solvedIndex; nodes[i].prev >= 0; i = nodes[i].prev) {
        path.push_back({nodes[i].seq, nodes[i].rot});
    }
    for (auto it = path.rbegin(); it != path.rend(); ++it) {
        w.applySeq(it->first, it->second);
    }
}

// ── Phase 7: Yellow corners orientation ─────────────────────────────────────
// Canonical alg: R' D' R D → "RDrd".  Applied in UFR slot, this twists the UFR
// corner by 120° while disturbing the D layer; 2 or 4 applications bring it
// back to correct twist AND restore D.  After all 4 corners twisted correctly,
// U-rotation may need a final adjustment.
void orientYellowCorners(Work& w) {
    // Walk 4 corners around U, always operating on the UFR slot (1,1,1).
    // After twisting each corner, rotate U to bring the next corner in.
    for (int k = 0; k < 4; ++k) {
        // Twist UFR until yellow is up.
        int guard = 0;
        while (guard < 6) {
            int i = w.findAt(V3i{1,1,1});
            if (w.stickerFacing(i, kDownDir /*home dir for this corner's yellow is its -y if any; actually U-corners have yellow at home +Y*/) == kDownDir) {
                // Hmm: yellow is what's on home +Y of the U-corner that lives at (1,1,1).
                // Let's re-check the correct predicate below.
                break;
            }
            V3i up = w.stickerFacing(i, V3i{0,1,0});
            if (up == V3i{0,1,0}) break;  // yellow (home +Y for top corners) is up
            w.applySeq("RDrd");           // R' D' R D
            ++guard;
        }
        // Rotate U to bring next corner to UFR (skip last iter).
        if (k < 3) w.apply(Move{Face::U, Direction::CW});
    }

    // D layer is restored modulo a U-alignment.  Fix: rotate U until all top
    // edges match their side centers (same check as phase 5).
    for (int k = 0; k < 4; ++k) {
        if (countMatchedUEdges(w) == 4) return;
        w.apply(Move{Face::U, Direction::CW});
    }
    LOG_WARN("orientYellowCorners: final U adjustment failed");
}

// ── Subgoal checkers (pure — no mutation) ───────────────────────────────────
// These verify that the state after each phase meets the phase's postcondition.
// Used both for logging diagnostics and for the test harness.

bool cubieFullyHome(const Work& w, int i) {
    const auto& c = w.s.cubies()[i];
    if (c.position != c.homePosition) return false;
    for (int a = 0; a < 3; ++a) {
        if (c.homePosition[a] == 0) continue;
        V3i d{0, 0, 0}; d[a] = c.homePosition[a];
        if (w.stickerFacing(i, d) != d) return false;
    }
    return true;
}

bool phase1OK(const Work& w) {  // white cross on D
    for (V3i home : {V3i{0,-1,1}, V3i{1,-1,0}, V3i{0,-1,-1}, V3i{-1,-1,0}}) {
        int i = w.findByHome(home);
        if (w.s.cubies()[i].position != home) return false;
        if (w.stickerFacing(i, kDownDir) != kDownDir) return false;
    }
    return true;
}

bool phase2OK(const Work& w) {  // whole D layer solved
    if (!phase1OK(w)) return false;
    for (V3i home : {V3i{1,-1,1}, V3i{1,-1,-1}, V3i{-1,-1,-1}, V3i{-1,-1,1}}) {
        int i = w.findByHome(home);
        if (!cubieFullyHome(w, i)) return false;
    }
    return true;
}

bool phase3OK(const Work& w) {  // bottom two layers solved
    for (int i = 0; i < kCubieCount; ++i) {
        if (w.s.cubies()[i].homePosition.y() == 1) continue; // skip U layer
        if (!cubieFullyHome(w, i)) return false;
    }
    return true;
}

bool phase4OK(const Work& w) {  // yellow cross on U (edges with +Y up)
    if (!phase3OK(w)) return false;
    for (V3i home : {V3i{0,1,1}, V3i{1,1,0}, V3i{0,1,-1}, V3i{-1,1,0}}) {
        int i = w.findByHome(home);
        if (w.stickerFacing(i, kUpDir) != kUpDir) return false;
    }
    return true;
}

bool phase5OK(const Work& w) {  // U edges permuted (still yellow up)
    if (!phase4OK(w)) return false;
    for (V3i home : {V3i{0,1,1}, V3i{1,1,0}, V3i{0,1,-1}, V3i{-1,1,0}}) {
        int i = w.findByHome(home);
        if (w.s.cubies()[i].position != home) return false;
    }
    return true;
}

bool phase6OK(const Work& w) {  // U corners at home positions (orientation free)
    if (!phase5OK(w)) return false;
    for (V3i home : {V3i{1,1,1}, V3i{1,1,-1}, V3i{-1,1,-1}, V3i{-1,1,1}}) {
        int i = w.findByHome(home);
        if (w.s.cubies()[i].position != home) return false;
    }
    return true;
}

bool phase7OK(const Work& w) { return w.s.isSolved(); }

// Collapse adjacent same-face moves.  The phase loops apply canned algorithms
// repeatedly without coordinating across iterations, which regularly produces
// pairs like `R R'`, `U U U U`, etc.  We walk the sequence once, merging any
// run whose top-of-stack shares a face with the new move.  Since the project
// does not use 180° turns, a net of two quarter-turns is kept as two CW moves.
std::vector<Move> simplifyAdjacent(const std::vector<Move>& moves) {
    std::vector<Move> out;
    out.reserve(moves.size());
    for (const Move& m : moves) {
        int net = (m.direction == Direction::CW ? 1 : -1);
        while (!out.empty() && out.back().face == m.face) {
            net += (out.back().direction == Direction::CW ? 1 : -1);
            out.pop_back();
        }
        net = ((net % 4) + 4) % 4; // 0..3
        if (net == 1) {
            out.push_back({m.face, Direction::CW});
        } else if (net == 2) {
            out.push_back({m.face, Direction::CW});
            out.push_back({m.face, Direction::CW});
        } else if (net == 3) {
            out.push_back({m.face, Direction::CCW});
        }
        // net == 0: nothing emitted
    }
    return out;
}

} // namespace

Solver::SolveReport Solver::solveWithReport(const CubeState& state) {
    SolveReport r;
    if (state.isSolved()) {
        r.phasePassed.fill(true);
        r.solved = true;
        return r;
    }

    Work w;
    w.s = state;

    using PhaseFn   = void (*)(Work&);
    using SubgoalFn = bool (*)(const Work&);
    const PhaseFn   phases[]   = { solveWhiteCross, solveWhiteCorners,
                                   solveMiddleEdges, solveYellowCross,
                                   permuteYellowEdges, permuteYellowCorners,
                                   orientYellowCorners };
    const SubgoalFn subgoals[] = { phase1OK, phase2OK, phase3OK, phase4OK,
                                   phase5OK, phase6OK, phase7OK };
    const char* names[] = { "whiteCross", "whiteCorners", "middleEdges",
                            "yellowCross", "permYellowEdges",
                            "permYellowCorners", "orientYellowCorners" };

    for (int p = 0; p < 7; ++p) {
        const std::size_t before = w.out.size();
        phases[p](w);
        r.phasePassed[p] = subgoals[p](w);
        const std::size_t after = w.out.size();
        LOG_INFO("Phase {} ({}): +{} moves, subgoal {}",
                 p + 1, names[p], after - before,
                 r.phasePassed[p] ? "OK" : "FAIL");
    }
    r.solved = w.s.isSolved();

    r.rawLen = w.out.size();
    r.moves  = simplifyAdjacent(w.out);
    LOG_INFO("Solver produced {} moves (raw {}, after simplification)",
             r.moves.size(), r.rawLen);
    return r;
}

std::vector<Move> Solver::solve(const CubeState& state) {
    return solveWithReport(state).moves;
}

} // namespace rubik
