#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include "sim/step_generators.hpp"
#include "sim/rng.hpp" 
#include "test_config.hpp"

using sim::Vec2;
using sim::RNG;
using sim::BrownianParams;
using sim::SpecifiedStepParams;
using sim::brownian_step;

// --------------------------- Brownian: pure drift ----------------------------
TEST(BrownianStep, PureDriftWhenDZero) {
    BrownianParams p;
    p.dt = 0.5;
    p.D = 0.0;
    p.mu_x = 1.2;
    p.mu_y = -0.8;

    RNG rng(12345);
    Vec2 d = brownian_step(p, rng);

    EXPECT_DOUBLE_EQ(d.x, p.mu_x * p.dt);
    EXPECT_DOUBLE_EQ(d.y, p.mu_y * p.dt);
}

// ----------------------- Brownian: mean & variance ---------------------------
TEST(BrownianStep, MeanVarianceMatchTheory) {
    BrownianParams p;
    p.dt = 0.2;
    p.D  = 1.3;
    p.mu_x = 0.0;
    p.mu_y = 0.0;

    const std::size_t N = 200000;  // reasonably fast & stable in CI
    RNG rng(777);

    long double sumx = 0.0L, sumy = 0.0L, sumx2 = 0.0L, sumy2 = 0.0L;
    for (std::size_t i = 0; i < N; ++i) {
        Vec2 d = brownian_step(p, rng);
        sumx  += d.x;
        sumy  += d.y;
        sumx2 += d.x * d.x;
        sumy2 += d.y * d.y;
    }

    const long double mx = sumx / N;
    const long double my = sumy / N;
    // Unbiased sample variance
    const long double vx = (sumx2 - N * mx * mx) / (N - 1);
    const long double vy = (sumy2 - N * my * my) / (N - 1);

    const double var_theory = 2.0 * p.D * p.dt;

    // Mean should be close to 0 within ~5 standard errors
    const double se_mean = std::sqrt(var_theory / N);
    EXPECT_NEAR(mx, 0.0, 5.0 * se_mean);
    EXPECT_NEAR(my, 0.0, 5.0 * se_mean);

    // Sample variance concentration: stddev[ s^2 ] / Var ≈ sqrt(2/(N-1))
    const double rel_sigma_s2 = std::sqrt(2.0 / (N - 1));
    const double tol_var = 6.0 * rel_sigma_s2 * var_theory; // ~6σ buffer
    EXPECT_NEAR(vx, var_theory, tol_var);
    EXPECT_NEAR(vy, var_theory, tol_var);
}

// ------------------------- Brownian: reproducibility -------------------------
TEST(BrownianStep, ReproducibleWithSameSeed) {
    BrownianParams p;
    p.dt = 0.1; p.D = 0.7; p.mu_x = 0.3; p.mu_y = -0.1;

    RNG r1(42), r2(42);
    for (int i = 0; i < 1000; ++i) {
        Vec2 a = brownian_step(p, r1);
        Vec2 b = brownian_step(p, r2);
        EXPECT_DOUBLE_EQ(a.x, b.x);
        EXPECT_DOUBLE_EQ(a.y, b.y);
    }
}

TEST(BrownianStep, DifferentSeedsUsuallyDiffer) {
    BrownianParams p;
    p.dt = 0.1; p.D = 0.7; p.mu_x = 0.3; p.mu_y = -0.1;

    RNG r1(42), r2(43);
    bool any_diff = false;
    for (int i = 0; i < 20; ++i) {
        Vec2 a = brownian_step(p, r1);
        Vec2 b = brownian_step(p, r2);
        if (a.x != b.x || a.y != b.y) { any_diff = true; break; }
    }
    EXPECT_TRUE(any_diff);
}

TEST(SpecifiedStep, RedactedThrowsInPublicBuild) {
#ifdef PUBLIC_SKELETON
  sim::RNG rng(123);
  sim::SpecifiedStepParams p{};
  EXPECT_REDACTED_THROW(sim::specified_step(p, 0, {1.0,1.0}, rng));
#else
  // original numeric assertions for private build
#endif
}

// test_simulation.cpp (if any Specified-step E2E exists)
TEST(Simulation, E2E_WithSpecifiedStep) {
#ifdef PUBLIC_SKELETON
  SKIP_REDACTED();
#else
  // original E2E assertions
#endif
}