// rng.cpp
// Implementation of RNG class defined in rng.hpp.

#include "sim/rng.hpp"
#include <random>
#include <cstdint>

namespace sim {

    // Default constructor: seed from hardware entropy.
    // Uses seed_seq to expand the entropy into the full mt19937 state.
    RNG::RNG() {
        std::random_device rd;
        std::seed_seq seq{rd(), rd(), rd(), rd(), rd(), rd(), rd(), rd()};
        gen.seed(seq);
    }

    // Deterministic seeding constructor: reproducible streams.
    // Note: mt19937 expects a 32-bit seed.
    RNG::RNG(std::uint32_t seed): gen(seed) {}
    
    // Draw one sample from the standard normal distribution N(0, 1).
    double RNG::gauss() { 
        return dist(gen);
    }

} // namespace sim