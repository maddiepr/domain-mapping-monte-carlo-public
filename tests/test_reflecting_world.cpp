// tests/test_reflecting_world.cpp
#include "sim/reflecting_world.hpp"
#include <gtest/gtest.h>

using namespace sim;

// --------------------------------------------------
// Test suite for advance_with_reflections()
// ---------------------------------------------------

// 1. No wall: displacement is applied directly
TEST(ReflectionTest, NoCollisionMovesFreely) {
    Vec2 pos{0.0, 0.0};
    Vec2 d{1.0, 0.0};       // step to the right
    ReflectingWorld w;      // empty world (no walls)
    advance_with_reflections(pos, d, w);
    EXPECT_DOUBLE_EQ(pos.x, 1.0);
    EXPECT_DOUBLE_EQ(pos.y, 0.0);
}

// 2. Normal incidence: straight in, straight out
TEST(ReflectingWorldTest, NormalIncidenceReflectsBack) {
    Vec2 pos{0.0, 1.0};
    Vec2 d{0.0, -2.0}; // straight down
    ReflectingWorld world;
    // Horizontal wall at y=0, with upward normal
    world.add_segment({-10,0}, {10,0}, {0,1});
    advance_with_reflections(pos, d, world);

    // Should bounce back up to y=2
    EXPECT_NEAR(pos.x, 0.0, 1e-12);
    EXPECT_NEAR(pos.y, 1.0, 1e-9);
}

// 3. Oblique incidence: angle in = angle out
TEST(ReflectionTest, ObliqueIncidenceReflectsSymmetrically) {
    Vec2 pos{0.0, 1.0};
    Vec2 d{1.0, -2.0}; // angled step
    ReflectingWorld w;
    w.add_segment({-10,0}, {10,0}, {0,1});
    advance_with_reflections(pos, d, w);
    // After bouncing, final y should be positive again.
    EXPECT_GT(pos.y, 0.0);
}

// 4. Start exactly on wall: stability (no jitter loops).
TEST(ReflectingWorldTest, StartOnWallMovesInward) {
    Vec2 pos{0.0, 0.0};   // on the floor
    Vec2 d{0.0, 1.0};     // step upward
    ReflectingWorld world;
    world.add_segment({-10,0}, {10,0}, {0,1}); // floor
    advance_with_reflections(pos, d, world);
    // Should move upward without being "stuck" on wall
    EXPECT_GT(pos.y, 0.0);
}

// 5. Corner hit: deterministic tie-break
TEST(ReflectingWorldTest, CornerHitResolvesDeterministically) {
    Vec2 pos{0.0, 1.0};
    Vec2 d{1.0, -1.0}; // aimed at (1,0) corner
    ReflectingWorld world;
    // Box corner at (1,0)
    world.add_inward_box(0.0, 1.0, 0.0, 1.0, 10);
    advance_with_reflections(pos, d, world);

    // Should remain inside the box (y >= 0, x <= 1)
    EXPECT_GE(pos.y, 0.0);
    EXPECT_LE(pos.x, 1.0);
}

// 6. Multi-bounce in a box: long displacement.
TEST(ReflectingWorldTest, MultiBounceInBox) {
    Vec2 pos{0.5, 0.5};
    Vec2 d{5.0, 0.0}; // long step across box
    ReflectingWorld world;
    world.add_inward_box(0.0, 1.0, 0.0, 1.0, 100);
    advance_with_reflections(pos, d, world);

    // Must end up inside the box boundaries
    EXPECT_GE(pos.x, 0.0);
    EXPECT_LE(pos.x, 1.0);
    EXPECT_GE(pos.y, 0.0);
    EXPECT_LE(pos.y, 1.0);
}

// 7. Grazing trajectory: near-parallel skip.
TEST(ReflectingWorldTest, GrazingTrajectoryIsStable) {
    Vec2 pos{0.0, 1.0};
    Vec2 d{10.0, -1e-13}; // almost tangent to floor
    ReflectingWorld world;
    world.add_segment({-10,0}, {10,0}, {0,1}); // floor
    advance_with_reflections(pos, d, world);

    // Expect mostly horizontal motion (x ~ 10)
    EXPECT_NEAR(pos.x, 10.0, 1e-9);
    // Still above the floor
    EXPECT_GT(pos.y, 0.0);
}