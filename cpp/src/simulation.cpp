// cpp/src/simulation.cpp

/**
 * @file simulation.cpp
 * @brief Implementation of sim::Simulation (step generation, reflections, history).
 * 
 * @details
 * Responsibilities implemented here (public contract in simulation.hpp):
 *   - Initialize per-particle RNGs
 *      - deterministic: seed = base_seed + particle_index
 *      - non-deterministic: hardware seeding
 *   - Per step:
 *      1. select the particle's step model (Brownian or Specified)
 *      2. generate a proposed displacement (uses RNG for Brownian)
 *      3. apply dx, then enforce geometry via advance_with_reflections(...)
 *      4. record history when (recorded_history && step_index % store_every == 0)
 * 
 * Invariants & policies
 *   - Sizes match:
 *      pos_.size() == step_type.size() == brownian_params_.size() == rngs_.size()
 *   - Reproducibility: same config + seeds -> identical histories
 *   - History memory ~ O(n_particles * n_steps / store_every); reserve when possible
 * 
 * Performance
 * -----------
 *   - Reflection usually dominates runtime; history can dominate memory
 *   - Keep callbacks lightweight; avoid per-step allocations
 * 
 * Error handling
 * --------------
 *   - Constructors/setters validate inputs; run() assumes preconditions
 *   - Indexing guarded in debug builds; fail fast with clear messages
 */

#include "sim/simulation.hpp"
#include <cassert>

namespace sim {
    
    Simulation::Simulation(const ReflectingWorld& world, const SimulationConfig& cfg)
        : world_(&world), cfg_(cfg)
    {
        const std::size_t n = cfg_.n_particles;

        // Basic config sanity (debug-only)
        assert(cfg_.store_every >= 1 && "SimulationConfig::store_every must be >= 1");

        // ---- Initialize particle state ----

        // Positions: start at the origin unless the caller overrides via set_positions(s).
        pos_.assign(n, Vec2{0.0, 0.0});

        // Step types: default all to Brownian; callers may override per particle.
        step_type_.assign(n, StepType::Brownian);

        // Per-particle parameters: start from config defaults (Brownian) and
        // a default-constructed specified-step params object.
        brownian_params_.assign(n, cfg_.brownian);
        spec_params_.assign(n, SpecifiedStepParams{});

        // ---- RNG setup ----
        // One RNG per particle:
        //  - Deterministic: per-particle seed = base_seed + particle_index (mod 2^32).
        //  - Non-deterministic: hardware/entropy-based seeding via RNG default ctor.
        rngs_.clear();
        rngs_.reserve(n);
        if (cfg_.deterministic) {
            for (std::size_t i = 0; i < n; ++i) {
                // Note: cast clarifies the intended 32-bit wraparound semantics if RNG uses uint32_t seeds.
                const unsigned int seed = static_cast<unsigned int>(cfg_.base_seed + static_cast<unsigned int>(i));
                rngs_.emplace_back(seed);
            }
        } else {
            for (std::size_t i = 0; i < n; ++i) {
                rngs_.emplace_back(); // hardware-seeded
            }
        }

        // ---- History containers ----
        if (cfg_.record_history) {
            hist_.assign(n, {});

            // Reserve to avoid frequent reallocations:
            // frames = 1 (initial) + ceil(n_steps / store_every)
            const std::size_t frames = 1 + (cfg_.n_steps + cfg_.store_every - 1) / cfg_.store_every;
            for (auto& h : hist_) h.reserve(frames);

            // Record initial positions (time index 0) unconditionally.
            for (std::size_t i = 0; i < n; ++i) {
                hist_[i].push_back(pos_[i]);
            }
        }

        #ifndef NDEBUG
            // Invariants: all per-particle arrays must have the same size.
            const bool sizes_ok = 
                pos_.size() == step_type_.size() &&
                pos_.size() == brownian_params_.size() &&
                pos_.size() == rngs_.size() &&
                (!cfg_.record_history || hist_.size() == pos_.size());
            assert(sizes_ok && "Per-particle containers must be the same length.");
        #endif
    }

    void Simulation::set_step_type_all(StepType type) {
        // Assign the same step model to every particle. O(n_particles).
        for (auto& t : step_type_) {
            t = type;
        }
    }

    void Simulation::set_step_type(std::size_t i, StepType type) {
        assert(i < step_type_.size() && "set_step_type: particle index out of range");
        step_type_[i] = type;
    }

    void Simulation::set_brownian_params(std::size_t i, const BrownianParams& p) {
        assert(i < brownian_params_.size() && "set_brownian_params: particle index out of range");
        brownian_params_[i] = p;
    }

    void Simulation::set_specified_params(std::size_t i, const SpecifiedStepParams& p) {
        assert(i < spec_params_.size() && "set_specified_params: particle index out of range");
        spec_params_[i] = p;
    }

    void Simulation::set_specified_params_all(const SpecifiedStepParams& p) {
        // Broadcast the same specified-step params to all particles. O(n_particles)
        for (auto& sp : spec_params_) {
            sp = p;
        }
    }

    void Simulation::set_specified_callback(SpecifiedCallback cb) {
        // Passing an empty std::function clears the callback; specified-step particles
        // will then rely on SpecifiedStepParams / generator defaults.
        specified_cb_ = std::move(cb);
    }

    void Simulation::set_positions(const std::vector<Vec2>& positions) {
        assert(positions.size() == pos_.size() && "set_positions: size mismatch with n_particles");
        pos_ = positions;

        // If recording, reset history to start from these positions (time index 0).
        if (cfg_.record_history) {
            const std::size_t n = pos_.size();
            hist_.assign(n, {});

            // Reserve: 1 initial frame + ceil(n_steps / store_every)
            const std::size_t frames = 1 + (cfg_.n_steps + cfg_.store_every - 1) / cfg_.store_every;
            for (auto& h : hist_) h.reserve(frames);

            for (std::size_t i = 0; i < pos_.size(); ++i) {
                hist_[i].push_back(pos_[i]);
            }
        }

        #ifndef NDEBUG
            // Invariants: when recording, history vectors exist and sizes match.
            if (cfg_.record_history) {
                assert(hist_.size() == pos_.size() && "set_positions: history size must match n_particles");
            }

        #endif
    }

    void Simulation::set_position(std::size_t i, const Vec2& p) {
        assert(i < pos_.size() && "set_position: particle index out of range");
        pos_[i] = p;

        if (cfg_.record_history) {
            // Ensure history exists; this keeps the "frame 0 = initial positions" contract.
            if (hist_.empty()) {
                const std::size_t n = pos_.size();
                hist_.assign(n, {});
                const std::size_t frames = 1 + (cfg_.n_steps + cfg_.store_every - 1) / cfg_.store_every;
                for (auto& h : hist_) h.reserve(frames);
            } else {
    #ifndef NDEBUG
                // If any particle already has >1 frames, likely started stepping-change frame 0
                // would desync the timeline. Encourage using set_positions() or a dedicated reset().
                for (const auto& h : hist_) {
                    assert(h.size() <= 1 && "set_position: called after stepping; use set_positions() or reset()");
                }
    #endif
                assert(hist_.size() == pos_.size() && "set_position: history size must match n_particles");
            }   

        // Keep consistent: ensure/overwrite the initial frame for particle i.
        if (hist_[i].empty()) {
            hist_[i].push_back(p);
        } else {
            hist_[i][0] = p;
            }
        }
    }

    void Simulation::run() {
        const std::size_t n = pos_.size();
        if (n == 0 || cfg_.n_steps == 0) return;

        assert(world_ != nullptr && "run: world_ must be set");
    #ifndef NDEBUG
        // Sanity: per-particle containers must align; history stride must be valid.
        assert(step_type_.size()       == n && "run: step_type_ size mismatch");
        assert(brownian_params_.size() == n && "run: brownian_params_ size mismatch");
        assert(rngs_.size()            == n && "run: rngs_ size mismatch");
        assert(cfg_.store_every >= 1        && "run: store_every must be >=1");
    #endif

        // History setup (policy): frame 0 = initial positions.
        if (cfg_.record_history && hist_.size() != n) {
            hist_.assign(n, {});
            const std::size_t frames = 1 + (cfg_.n_steps + cfg_.store_every - 1) / cfg_.store_every;
            for (auto& h : hist_) h.reserve(frames);
            for (std::size_t i = 0; i < n; ++i) hist_[i].push_back(pos_[i]); // frame 0
        }

        const bool record = cfg_.record_history;
        const std::size_t stride = cfg_.store_every; // record every 'stride' steps

        // ---- Main integration loop ---
        for (std::size_t k = 0; k < cfg_.n_steps; ++k) {
            // HOT PATH: per-particle step selection + reflection enforcement.
            for (std::size_t i = 0; i < n; ++i) {
                Vec2 d{0.0, 0.0};

                // Step model dispatch: Brownian uses RNG with BrownianParams;
                // Specified uses a callback if provided, else SpecifiedStepParams;
                switch (step_type_[i]) {
                    case StepType::Brownian:
                        d = brownian_step(brownian_params_[i], rngs_[i]);
                        break;
                    case StepType::Specified:
                        d = specified_cb_
                            ? specified_cb_(i, k, pos_[i], rngs_[i])
                            : specified_step(spec_params_[i], k, pos_[i], rngs_[i]);
                        break;
                    default: 
                        assert(false && "run: unknown StepType");
                        break;
                }

                // Geometry policy: reflect proposed displacement inside world.
                advance_with_reflections(pos_[i], d, *world_);
            }

            // History policy: append positions every 'stride' steps (no forced final frame).
            if (record && ((k+1) % stride == 0)) {
                for (std::size_t i = 0; i < n; i++) {
                    hist_[i].push_back(pos_[i]);
                }
            }
        }
    }

    const std::vector<Vec2>& Simulation::positions() const noexcept {
        // Current particle positions (size == config().n_particles).
        return pos_;
    }

    const std::vector<std::vector<Vec2>>& Simulation::history() const noexcept {
        // Trajectories (may be empty if record_history == false).
        return hist_;
    }

    const SimulationConfig& Simulation::config() const noexcept {
        // Effective configuration copied at construction.
        return cfg_;
    }
} // namespace sim