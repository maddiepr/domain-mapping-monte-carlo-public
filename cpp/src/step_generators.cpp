// SPDX-License-Identifier: MIT
// cpp/src/step_generators.cpp
//
// Implementation notes for step generators (public skeleton).
//
// Public contract lives in step_generators.hpp. This TU intentionally
// avoids exposing proprietary mapping details. Brownian remains implemented;
// the mapping-driven "specified" step is stubbed when PUBLIC_SKELETON is set.

#include "sim/step_generators.hpp"
#include "sim/rng.hpp"

#include <cmath>
#include <stdexcept>

namespace sim {

// -----------------------------------------------------------------------------
// Brownian step (Euler–Maruyama):
//   dx = mu * dt + sqrt(2*D*dt) * ξ,   with ξ ~ N(0, I₂).
// Draws two independent standard normals per call (x then y).
// O(1), no allocations, deterministic given RNG state.
// -----------------------------------------------------------------------------
Vec2 brownian_step(const BrownianParams& p, ::sim::RNG& rng) noexcept {
  const double sigma = std::sqrt(2.0 * p.D * p.dt);
  const double dx = p.mu_x * p.dt + sigma * rng.gauss();
  const double dy = p.mu_y * p.dt + sigma * rng.gauss();
  return Vec2{dx, dy};
}

// -----------------------------------------------------------------------------
// Specified (position-dependent / mapped) step.
// Public-skeleton behavior: redacted implementation.
// When PUBLIC_SKELETON is defined, this function throws to indicate
// that the private mapping kernel is not available in the public build.
// -----------------------------------------------------------------------------
Vec2 specified_step(const SpecifiedStepParams& /*p*/,
                    std::size_t /*step_index*/,
                    const Vec2& /*pos_uv*/,
                    ::sim::RNG& /*rng*/)
{
#ifdef PUBLIC_SKELETON
  throw std::runtime_error("sim::specified_step is redacted in the public skeleton build");
#else
  // Private implementation goes here in non-public builds.
  // return <computed displacement>;
  (void)0; // placeholder to suppress empty function warnings
#endif
}

} // namespace sim
