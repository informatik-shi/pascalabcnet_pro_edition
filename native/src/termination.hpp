#pragma once
#include <cstdint>
namespace abc {
// Internal control transfer caught at the execution boundary, never a diagnostic.
struct ProgramTermination { std::int32_t code; };
}
