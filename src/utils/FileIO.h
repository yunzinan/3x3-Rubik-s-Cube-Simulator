#pragma once

#include "core/Move.h"

#include <string>
#include <vector>

namespace rubik {

// Read / write control-sequence JSON files.
//
// File format (see interface-specification.md):
// {
//     "control_sequence": ["f", "F", "b", ...]
// }
namespace FileIO {

// Returns empty vector on failure; logs errors via Logger.
std::vector<Move> loadControlSequence(const std::string& path);

// Returns true on success.
bool saveControlSequence(const std::string& path,
                         const std::vector<Move>& moves);

} // namespace FileIO
} // namespace rubik
