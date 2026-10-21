// Fixed Expert benchmark metadata. Board references are stored in the CSV below.
#pragma once
#include <array>
#include <cstddef>
#include <cstdint>

namespace expert_benchmark {
inline constexpr size_t board_count = 1000;
inline constexpr std::array<uint64_t, 3> seeds{{
    15415727658250109428ULL, 13781498123793209364ULL, 1443274517951773430ULL
}};
inline constexpr const char* reference_csv = "benchmarks/expert_benchmark_reference.csv";
} // namespace expert_benchmark
