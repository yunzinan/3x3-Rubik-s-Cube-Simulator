import json
import random
import subprocess
import unittest
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build"
SOLVER_CLI = BUILD_DIR / "Release" / "bin" / "solver_cli"

FACES = "fbudlr"


def random_scramble(seed: int, length: int = 30) -> str:
    rng = random.Random(seed)
    moves = []
    last_face = None
    for _ in range(length):
        face = rng.choice(FACES)
        while face == last_face:
            face = rng.choice(FACES)
        move = face.upper() if rng.randrange(2) else face
        moves.append(move)
        last_face = face
    return "".join(moves)


def run_solver(scramble: str) -> dict:
    if not SOLVER_CLI.exists():
        raise AssertionError(
            f"solver_cli not found at {SOLVER_CLI}. "
            "Build it first with: "
            "cmake -S . -B build -DBUILD_DESKTOP_APP=ON -DBUILD_WEB_APP=OFF "
            "-DBUILD_TESTS=ON -DFETCHCONTENT_UPDATES_DISCONNECTED=ON && "
            "cmake --build build --target solver_cli -j4"
        )

    proc = subprocess.run(
        [str(SOLVER_CLI), scramble],
        cwd=ROOT,
        check=True,
        capture_output=True,
        text=True,
    )
    return json.loads(proc.stdout)


class SolverCLITest(unittest.TestCase):
    def assertSolved(self, result: dict) -> None:
        self.assertTrue(result["solverReportedSolved"], result)
        self.assertTrue(result["verifiedSolved"], result)
        self.assertEqual(result["phasePassed"], [True] * 7, result)
        self.assertGreaterEqual(result["rawLength"], result["solutionLength"], result)

    def test_known_scramble_is_solved(self) -> None:
        scramble = "lBDFRluRLDrdrlBRfUbFbRurbuRdfD"
        result = run_solver(scramble)
        self.assertEqual(result["scramble"], scramble)
        self.assertSolved(result)

    def test_solver_handles_multiple_deterministic_scrambles(self) -> None:
        for seed in range(20):
            with self.subTest(seed=seed):
                result = run_solver(random_scramble(seed))
                self.assertSolved(result)

    def test_solution_length_stays_reasonable(self) -> None:
        lengths = []
        for seed in range(10):
            result = run_solver(random_scramble(1000 + seed))
            self.assertSolved(result)
            lengths.append(result["solutionLength"])

        self.assertLess(max(lengths), 260, lengths)


if __name__ == "__main__":
    unittest.main()
