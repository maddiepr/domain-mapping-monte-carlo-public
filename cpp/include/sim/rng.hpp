/* 
* @file rng.hpp
* @brief Gaussian random number generator using Mersenne Twister.
*/

#pragma once
#include <cstdint>
#include <random>

/*
* @class RNG
* @brief Generates standard normal random numbers (mean 0, stddev 1).
* 
* Backed by 'std::mt19937'. Default construction is intended to seed from
* hardware entropy (see .cpp), and an overload allows deterministic seeding
* for reproducible simulations.
*
* @note: Not thread-safe. Prefer one RNG instance per thread
*/

namespace sim {

class RNG {
    public:
        /// Seed from hardware entropy (implementation in .cpp).
        RNG();

        /// Deterministic seeding for reproducibility.
        explicit RNG(unsigned int seed);

        /// Draw a standard normal sample N(0, 1).
        [[nodiscard]] double gauss();

        /// Convenience: same as gauss().
        double operator()() { return gauss(); }

    private:
        std::mt19937 gen;
        std::normal_distribution<double> dist{0.0, 1.0};
    };

} // namespace sim