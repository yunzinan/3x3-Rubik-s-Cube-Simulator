#include "utils/FileIO.h"
#include "utils/Logger.h"

#include <nlohmann/json.hpp>

#include <fstream>
#include <string>

namespace rubik {

using json = nlohmann::json;

std::vector<Move> FileIO::loadControlSequence(const std::string& path) {
    std::ifstream ifs(path);
    if (!ifs.is_open()) {
        std::string prefix;
        for (int up = 1; up <= 3; ++up) {
            prefix += "../";
            ifs.open(prefix + path);
            if (ifs.is_open()) break;
        }
    }
    if (!ifs.is_open()) {
        LOG_ERROR("FileIO: cannot open '{}'", path);
        return {};
    }

    try {
        json j = json::parse(ifs);
        auto strings = j.at("control_sequence").get<std::vector<std::string>>();
        auto moves = parseMovesFromStrings(strings);
        LOG_INFO("FileIO: loaded {} moves from '{}'", moves.size(), path);
        return moves;
    } catch (const json::exception& e) {
        LOG_ERROR("FileIO: JSON parse error in '{}': {}", path, e.what());
        return {};
    }
}

bool FileIO::saveControlSequence(const std::string& path,
                                 const std::vector<Move>& moves) {
    json j;
    j["control_sequence"] = movesToStrings(moves);

    std::ofstream ofs(path);
    if (!ofs.is_open()) {
        LOG_ERROR("FileIO: cannot write to '{}'", path);
        return false;
    }

    ofs << j.dump(4) << '\n';
    LOG_INFO("FileIO: saved {} moves to '{}'", moves.size(), path);
    return true;
}

} // namespace rubik
