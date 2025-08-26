#pragma once
/**
 * @file step_generators.hpp
 * @brief Small set of pluggable "step models" that propose per-step 2D displacements.
 * 
 * Provides pure functions that, given parameter structs and an RNG, return a displacement
 * to apply before any geometry/reflection logic (handled elsewhere).
 * 
 * Included models:
 *   - Brownian (Euler-Maruyama): dx = mu * dt + sqrt(2*d*dt) ξ,  ξ ~ N(0, I₂).
 *   - Specified (mapped/user-supplied) step: interface only in the public skeleton.
 * 
 * Responsibilities:
 *   - Read step params (dt, D, drift, k, etc.) and current state if needed.
 *   - Use caller-provided RNG (no global state) and return a 2D increment.
 * 
 * Non-responsibilities:
 *   - No boundary handling, seeding policy, or multiple-particle orchestration.
 * 
 * Public-skeleton notes:
 *   - Implementation for mapped/specified steps are intentionally redacted; only
 *     API surface is provided. When PUBLIC_SKELETON is defined, these may be 
 *     stubbed to throw at runtime.
 * 
 * Testing hints (Brownian only): mean ~ mu*dt; per-axis var ~2*d*dt; D=0 -> pure drift;
 *  fixed seed -> identical sequences.
 */

 #include <cstddef>
 #include <cmath>
 #include "sim/vec2.hpp"
 #include "sim/rng.hpp"

 namespace sim {

    /**
     * @brief Parameters for a 2D Brownian (Euler-Maruyama) increment.
     * 
     * Models dX = mu * dt + sqrt(2*D) dW with isotropic diffusion.
     * All fields are in consistent units of position/time.
     */
    struct BrownianParams {
        double dt   {1.0};      ///< Time step > 0
        double D    {1.0};      ///< Diffusion coefficient >= (isotropic)
        double mu_x {0.0};      ///< Drift x-component
        double mu_y {0.0};      ///< Drift y-component
    };

    /**
     * @brief Generate a single Brownian displacement dx.
     * 
     * Draws dx = mu * dt + sqrt(2 * D * dt) * ξ, with ξ ~ N(0, I₂). Returns a displacement
     * (not an absolute position). Never throws.
     * 
     * @param p     Brownian parameters (dt, D, mu)
     * @param rng   RNG producing independent standard normal variates.
     * @return      2D displacement to add to the current position.
     */
    Vec2 brownian_step(const BrownianParams& p, ::sim::RNG& rng) noexcept;

    /**
     * @brief Parameters for a position-dependent "specified" step (public skeleton).
     * 
     * Notes:
     *   - Intended for user/mapping-driven steps where magnitude and/or direction
     *     may depend on the current position.
     *   - Implementation details are intentionally redacted in the public build;
     *     only the API surface is provided here.
     * 
     * If diff_scale <= 0, an effective scale is chosen automatically.
     */
    struct SpecifiedStepParams {
        double dt           {1.0};              ///< Time step dt > 0
        double D            {1.0};              ///< Base diffusion coefficient D >= 0
        double diff_scale   {1.0};              ///< Optional scale hint; if <= 0, use an implentation fallback.
        int    k            {1};              ///< Generic model parameter (integer >= 1).
    };

    /**
     * @brief Propose a position-dependent displacement for "specified" steps.
     * 
     * Returns a displacement (du, dv) to be applied before geometry/reflection logic.
     * Uses the caller-supplied RNG; no global state. Directional properties and scaling
     * are implementation-defined in the private version and redacted in the public skeleton.
     * 
     * Public-skeleton behavior:
     *   - When build with PUBLIC_SKELETON, this function may be stubbed for raise a runtime error.
     * 
     * @param p             Parameters (dt, D, diff_scale, k)
     * @param step_index    Optional index for time-dependent schemes (unused here).
     * @param pos_uv        Current position where the step is applied.
     * @param rng           RNG producing independent draws for the step model.
     * @return              2D displacement.
     */
    Vec2 specified_step(const SpecifiedStepParams& p,
                        std::size_t step_index,
                        const Vec2& pos_uv,
                        ::sim::RNG& rng);

 } //namespace sim
