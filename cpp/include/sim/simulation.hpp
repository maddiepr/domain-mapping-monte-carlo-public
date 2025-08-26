// SPDX-License_Identifier: MIT
#pragma once
/**
 * @file simulation.hpp
 * @brief Orchestrates multi-particle 2D simulations: step generation, reflections, and optional history.
 *
 * @details For each step and particle:
 *  1) generate a displacement (Brownian or specified),
 *  2) update position, then
 *  3) enforce geometry via ReflectingWorld.
 *
 * History can be recorded at a stride (store_every). Reproducibility uses one RNG per particle;
 * if deterministic, per-particle seed = base_seed + index.
 *
 * Public-skeleton note: heavy implementations live in .cpp files and may be compiled to throw
 * (e.g., when PUBLIC_SKELETON is enabled).
 * 
 * Key types: sim::Simulation, sim::SimulationConfig, sim::StepType, BrownianParams, SpecifiedStepParams.
 * @see sim::reflecting_world.hpp, sim::step_generators.hpp
 */

#include <cstddef>
#include <vector>
#include <functional>

#include "sim/vec2.hpp"
#include "sim/rng.hpp"
#include "sim/reflecting_world.hpp"
#include "sim/step_generators.hpp"

namespace sim {

    /**
     * @enum StepType
     * @brief Selects the step model used to advance a particle.
     * 
     * Use this to choose how each particle's position increment is generated 
     * during a simulation step.
     * 
     * - StepType::Brownian - Use a Brownian/gaussian step generator
     *                         (see @ref BrownianParams for defaults).
     * - StepType::Specified - Use a caller-provided step generator (e.g., a 
     *                         mapping steps).
     */
    enum class StepType {
        Brownian,       ///< Step Brownian (Gaussian) increments.
        Specified       ///< Externally specified step generator per particle.
    };

    /**
     * @struct SimulationConfig
     * @brief Run-wide settings for a simulation.
     * 
     * Notes: 
     * - History storage can be memory-intensive; use 'store_every' to decimate.
     * - If 'deterministic == true', each particle's RNG is seeded with
     *   'base_seed + particle_index', ensuring reproducible runs.
     */
    struct SimulationConfig {
        std::size_t n_particles     {1};        ///< Number of independent particles to simulate. @pre n_particles >=1.
        std::size_t n_steps         {0};        ///< Total number of integration steps to run. @pre n_steps >= 0.

        bool        record_history  {true};     ///< If true, store particle trajectories for post-analysis.
        std::size_t store_every     {1};        ///< Keep 1 of every k frames (decimation factor). @pre store_every >= 1.

        // RNG policy
        unsigned int base_seed      {5489u};    ///< Base seed used to derive per-particle seeds.
        bool         deterministic  {true};     ///< If true, per-particle seed = base_seed + i; if false, use hardware seeding.

        // Default Brownian parameters (can be overridden per particle if desired)
        BrownianParams brownian{};              ///< Default Gaussian step configuration for StepType::Brownian.
    };

    /**
     * @brief Simulation driver: manages particles, applies step generators, and enforces reflections.
     * 
     * Notes:
     *   - Each particle can choose its own StepType.
     *   - By default, BrownianParams from SimulationConfig are used for Brownian particles.
     *   - A custom specified-step callback can be provided (e.g., time-/state-dependent).
     *   - Public-skeleton: heavy logic lives in .cpp and may be compiled to throw when PUBLIC_SKELETON is defined.
     */
    class Simulation {
        public:
            /**
             * @brief Callback that produces a displacement for @c StepType::Specified particles
             * 
             * The callback is invoked once per step for each particle configured with
             * @c StepType::Specified. It receives the particle index, the current step
             * index, the particle's current positions, and a reference to that particle's RNG.
             * 
             * @param particle_index    Index in [0, n_particles]
             * @param step_index        Index in [0, n_steps]
             * @param position          Current particle position (before applying the step).
             * @param rng               Per-particle RNG to use for stochastic models.
             * @return Vec2             The proposed displacement dx to apply before reflections.
             * 
             * @note Reflections against boundaries defined by @ref ReflectingWorld are applied
             *       by the simulation after this displacement is generated
             */
            using SpecifiedCallback = 
                std::function<Vec2(std::size_t particle_index,
                                   std::size_t step_index,
                                   const Vec2& position,
                                   RNG& rng)>;

            /**
             * @brief Construct a simulation bound to a reflecting world.
             * 
             * @param world     Reference to the reflecting world (not owned).
             * @param cfg       Run configuration (copied internally).
             * 
             * @post positions().size() == cfg.n_particles
             * @post history().size     == (cfg.record_history ? cfg.n_particles : 0)
             * 
             * @see SimulationConfig
             * @see ReflectingWorld
             */
            Simulation(const ReflectingWorld& world, const SimulationConfig& cfg);

            /**
             * @brief Set the same step model for all particles.
             * @param type Step model to assign to every particle.
             * @complexity O(n_particles)
             */
            void set_step_type_all(StepType type);

            /**
             * @brief Set the step model for a single particle.
             * @param i     Particle index in [0, n_particles).
             * @param type  Step model for particle @p i.
             * @pre i < positions().size()
             */
            void set_step_type(std::size_t i, StepType type);

            /**
             * @brief Override Brownian parameters for a single particle.
             * @param i     Particle index in [0, n_particles).
             * @param p     Parameters to use when @c StepType::Brownian  is active for @p i.
             * @pre i < positions().size()
             * @note If not set, the per-particle defaults come from @ref SimulationConfig::brownian
             */
            void set_brownian_params(std::size_t i, const BrownianParams& p);

            /**
             * @brief Set specified-step parameters for a single particle.
             * @param i Particle index in [0, n_particles).
             * @param p Parameters consumed by the specified-step generator.
             * @pre i < positions().size()
             */
            void set_specified_params(std::size_t i, const SpecifiedStepParams& p);

            /**
             * @brief Set specified-step parameters for all particles.
             * @param p Parameters consumed by the specified-step generator.
             * @complexity O(n_particles)
             */
            void set_specified_params_all(const SpecifiedStepParams& p);

            /**
             * @brief Set the callback used for @c StepType::Specified particles.
             * @param cd Function that returns a displacement for a given particle and step.
             * @note If not set, specified-step particles will fall back to @ref SpecifiedStepParams
             *       and any compiled-in behavior of the specified-step generator.
             */
            void set_specified_callback(SpecifiedCallback cd);

            /**
             * @brief Initialize all particle positions.
             * @param positions Vector of length @c n_particles with world-space coordinates.
             * @pre positions.size() == config().n_particles
             * @note Positions are interpreted in the coordinates of @ref ReflectingWorld.
             *       Behavior for positions outside the world's valid region is determined by
             *       the world's reflection rules.
             */
            void set_positions(const std::vector<Vec2>& positions);

            /**
             * @brief Initialize a single particle position.
             * @param i Particle index in [0, n_particles).
             * @param p World-space position.
             * @pre i < config().n_particles
             */
            void set_position(std::size_t i, const Vec2& p);

            /**
             * @brief Run the simulation for @c config().n_steps steps.
             * 
             * For each step and particle:
             *   1. Generate a proposed displacement via the chosen step model
             *      (Brownian with @ref BrownianParams, or @ref SpecifiedCallback / SpecifiedStepParams).
             *   2. Apply reflections against @ref ReflectingWorld boundaries.
             *   3. Optionally record into @ref history() according to @ref SimulationConfig::record_history.
             * 
             * @complexity O(n_particles * n_steps)
             * @note RNG seeding follows @ref SimulationConfig::deterministic and
             *       @ref SimulationConfig::base_seed (per-particle @c base_seed + i).
             */
            void run();

            // ---- Accessors ----

            /**
             * @brief Current particle positions.
             * @return Reference to a vector of size @c n_particles.
             */
            const std::vector<Vec2>& positions() const noexcept;

            /**
             * @brief Recorded trajectories (if enabled).
             * @return Vector of per-particle polylines; empty if @c record_history == false.
             * @note Decimated by @ref SimulationConfig::stor_every.
             */
            const std::vector<std::vector<Vec2>>& history() const noexcept;
            
            /**
             * @brief Effective configuration for this simulation.
             * @return The configuration copied at construction time.
             */
            const SimulationConfig& config() const noexcept;

        private:
            // Not owned; world geometry and reflection policy.
            const ReflectingWorld*  world_;

            // Effective configuration for this run.
            SimulationConfig        cfg_;

            // State
            std::vector<Vec2>                   pos_;               ///< Current positions.
            std::vector<std::vector<Vec2>>      hist_;              ///< Trajectories (optional).
            std::vector<StepType>               step_type_;         ///< Per-particle step model.
            std::vector<BrownianParams>         brownian_params_;   ///< Per-particle Brownian overrides.

            // Randomness & specified-step configuration
            std::vector<RNG>                    rngs_;              ///< Rer-particle RNGs
            std::vector<SpecifiedStepParams>    spec_params_;       ///< Per-particle specified-step params
            SpecifiedCallback                   specified_cb_{};    ///< Optional specified-step callback.
    };

} // namespace sim