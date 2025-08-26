// tests/test_simulation.cpp
#include <gtest/gtest.h>
#include "sim/simulation.hpp"
#include "sim/reflecting_world.hpp"
#include "sim/vec2.hpp"

using sim::Simulation;
using sim::SimulationConfig;
using sim::ReflectingWorld;
using sim::Vec2;
using sim::StepType;

namespace {

// Deterministic specified-step callback: move +1 in x every step.
static Simulation::SpecifiedCallback kStepRight = [](
    std::size_t /*i*/, std::size_t /*k*/, const Vec2& /*pos*/, sim::RNG& /*rng*/) {
    return Vec2{1.0, 0.0};
};

ReflectingWorld makeEmptyWorld() {
    ReflectingWorld w; // no boundaries
    return w;
}

ReflectingWorld makeUnitBox() {
    ReflectingWorld w;
    // box from (0,0) to (1,1), inward normals; wall id = 0
    w.add_inward_box(0.0, 1.0, 0.0, 1.0, /*wall_id=*/0);
    return w;
}

} // namespace

// ------------------- Construction & invariants -------------------

TEST(SimulationBasics, ZeroParticlesNoCrash) {
    auto w = makeEmptyWorld();
    SimulationConfig cfg;
    cfg.n_particles = 0;
    cfg.n_steps = 10;
    Simulation sim(w, cfg);
    sim.run(); // should be a no-op

    EXPECT_EQ(sim.positions().size(), 0u);
    EXPECT_TRUE(sim.history().empty());
}

TEST(SimulationBasics, HistoryDisabledIsEmpty) {
    auto w = makeEmptyWorld();
    SimulationConfig cfg;
    cfg.n_particles = 2;
    cfg.n_steps = 5;
    cfg.record_history = false;

    Simulation sim(w, cfg);
    sim.run();
    EXPECT_TRUE(sim.history().empty());
}

TEST(SimulationBasics, HistoryStartsWithFrameZeroWhenEnabled) {
    auto w = makeEmptyWorld();
    SimulationConfig cfg;
    cfg.n_particles = 2;
    cfg.n_steps = 0; // no stepping
    cfg.record_history = true;

    Simulation sim(w, cfg);
    ASSERT_EQ(sim.history().size(), 2u);
    for (const auto& h : sim.history()) {
        ASSERT_EQ(h.size(), 1u);                // initial frame only
        EXPECT_DOUBLE_EQ(h[0].x, 0.0);
        EXPECT_DOUBLE_EQ(h[0].y, 0.0);
    }
}

// ------------------- Setters -------------------

TEST(SimulationSetters, SetPositionsResetsHistoryFrameZero) {
    auto w = makeEmptyWorld();
    SimulationConfig cfg;
    cfg.n_particles = 3;
    cfg.record_history = true;

    Simulation sim(w, cfg);
    std::vector<Vec2> init = { {1,2}, {3,4}, {5,6} };
    sim.set_positions(init);

    ASSERT_EQ(sim.history().size(), 3u);
    for (std::size_t i = 0; i < 3; ++i) {
        ASSERT_EQ(sim.history()[i].size(), 1u);
        EXPECT_DOUBLE_EQ(sim.history()[i][0].x, init[i].x);
        EXPECT_DOUBLE_EQ(sim.history()[i][0].y, init[i].y);
    }
}

TEST(SimulationSetters, SetPositionOverwritesFrameZeroBeforeRun) {
    auto w = makeEmptyWorld();
    SimulationConfig cfg;
    cfg.n_particles = 1;
    cfg.record_history = true;

    Simulation sim(w, cfg);
    sim.set_position(0, Vec2{7.0, 9.0});
    ASSERT_EQ(sim.history().size(), 1u);
    ASSERT_EQ(sim.history()[0].size(), 1u);
    EXPECT_DOUBLE_EQ(sim.history()[0][0].x, 7.0);
    EXPECT_DOUBLE_EQ(sim.history()[0][0].y, 9.0);
}

// ------------------- Core stepping (specified callback) -------------------

TEST(SimulationRun, SpecifiedCallbackMovesRight_NoBoundaries) {
    auto w = makeEmptyWorld();
    SimulationConfig cfg;
    cfg.n_particles = 1;
    cfg.n_steps = 5;
    cfg.record_history = false;

    Simulation sim(w, cfg);
    sim.set_step_type_all(StepType::Specified);
    sim.set_specified_callback(kStepRight);
    sim.run();

    const auto& p = sim.positions()[0];
    EXPECT_DOUBLE_EQ(p.x, 5.0); // 5 steps * 1.0
    EXPECT_DOUBLE_EQ(p.y, 0.0);
}

TEST(SimulationRun, HistoryStride_RecordsEveryK) {
    auto w = makeEmptyWorld();
    SimulationConfig cfg;
    cfg.n_particles = 1;
    cfg.n_steps = 10;
    cfg.record_history = true;
    cfg.store_every = 3; // record after steps 3,6,9

    Simulation sim(w, cfg);
    sim.set_step_type_all(StepType::Specified);
    sim.set_specified_callback(kStepRight);
    sim.run();

    const auto& H = sim.history()[0];
    ASSERT_EQ(H.size(), 1u + 3u); // initial + steps {3,6,9}
    EXPECT_DOUBLE_EQ(H[0].x, 0.0);
    EXPECT_DOUBLE_EQ(H[1].x, 3.0);
    EXPECT_DOUBLE_EQ(H[2].x, 6.0);
    EXPECT_DOUBLE_EQ(H[3].x, 9.0);
}

// ------------------- Reproducibility (Brownian) -------------------

TEST(SimulationRepro, DeterministicSeedsMatch) {
    auto w = makeEmptyWorld();
    SimulationConfig cfg;
    cfg.n_particles = 2;
    cfg.n_steps = 100;
    cfg.record_history = true;
    cfg.deterministic = true;
    cfg.base_seed = 123456u;

    Simulation a(w, cfg);
    Simulation b(w, cfg);

    a.set_step_type_all(StepType::Brownian);
    b.set_step_type_all(StepType::Brownian);

    a.run();
    b.run();

    ASSERT_EQ(a.positions().size(), b.positions().size());
    for (std::size_t i = 0; i < a.positions().size(); ++i) {
        EXPECT_NEAR(a.positions()[i].x, b.positions()[i].x, 1e-12);
        EXPECT_NEAR(a.positions()[i].y, b.positions()[i].y, 1e-12);
    }

    ASSERT_EQ(a.history().size(), b.history().size());
    for (std::size_t i = 0; i < a.history().size(); ++i) {
        ASSERT_EQ(a.history()[i].size(), b.history()[i].size());
        for (std::size_t f = 0; f < a.history()[i].size(); ++f) {
            EXPECT_NEAR(a.history()[i][f].x, b.history()[i][f].x, 1e-12);
            EXPECT_NEAR(a.history()[i][f].y, b.history()[i][f].y, 1e-12);
        }
    }
}
