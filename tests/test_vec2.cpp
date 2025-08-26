#include "sim/vec2.hpp"
#include <gtest/gtest.h>
#include <cmath>

using sim::Vec2;
using sim::reflect_across_unit_normal;

namespace{
    // Helper for comparing vectors with a tolerance.
    inline void ExpectNear(const Vec2& a, const Vec2& b, double tol = 1e-12) {
        EXPECT_NEAR(a.x, b.x, tol);
        EXPECT_NEAR(a.y, b.y, tol);
    }

    // ---- Compile-time checks for constexpr operations ----
    constexpr Vec2 ca{1.0, 2.0};
    constexpr Vec2 cb{3.0, 4.0};
    constexpr auto cc = ca + cb;
    static_assert(cc.x == 4.0 && cc.y == 6.0, "constexpr add failed");
    constexpr auto cd = 2.0 * ca;
    static_assert(cd.x == 2.0 && cd.y == 4.0, "constexpr scalar mul failed");
    constexpr double cdd = ca.dot(cb);
    static_assert(cdd == 11.0, "constexpr dot failed"); // 1*3 + 2*4 = 11
} // namespace

// 1. 
TEST(Vec2, DefaultConstructor) {
    Vec2 v;
    EXPECT_DOUBLE_EQ(v.x, 0.0);
    EXPECT_DOUBLE_EQ(v.y, 0.0);
}

// 2. 
TEST(Vec2, ValueConstructor) {
    Vec2 v(1.5, -2.5);
    EXPECT_DOUBLE_EQ(v.x, 1.5);
    EXPECT_DOUBLE_EQ(v.y, -2.5);
}

// 3. Check addition
TEST(Vec2, AdditionSubtraction) {
    Vec2 a(1.0, 2.0), b(3.0, -5.0);
    ExpectNear(a + b, {4.0, -3.0});
    ExpectNear((a + b) - b, a);
}

TEST(Vec2, ScalarMultiplyBothOrders) {
    Vec2 a(2.0, -3.0);
    ExpectNear(a * 2.5,  {5.0,  -7.5});
    ExpectNear(2.5 * a,  {5.0,  -7.5}); // friend operator*(s, v)
}

TEST(Vec2, CompoundAssignments) {
    Vec2 a(1.0, 2.0), b(3.0, 4.0);
    (a += b);
    ExpectNear(a, {4.0, 6.0});
    (a -= b);
    ExpectNear(a, {1.0, 2.0});
    (a *= 2.0);
    ExpectNear(a, {2.0, 4.0});
}

TEST(Vec2, DotAndNormBasic) {
    Vec2 a(3.0, 4.0), b(-5.0, 2.0);
    EXPECT_DOUBLE_EQ(a.dot(a), 25.0);
    EXPECT_DOUBLE_EQ(a.norm(), 5.0);        // exact for (3,4)
    EXPECT_DOUBLE_EQ(a.dot(b), b.dot(a));   // commutativity
    EXPECT_DOUBLE_EQ(a.dot(b), 3.0*(-5.0) + 4.0*2.0);
}

TEST(Vec2, NormalizedTypical) {
    Vec2 a(3.0, 4.0);
    Vec2 u = a.normalized();                // default eps = 1e-12
    ExpectNear(u, {0.6, 0.8});
    EXPECT_NEAR(u.norm(), 1.0, 1e-12);
}

TEST(Vec2, NormalizedSmallVectorReturnsZero) {
    Vec2 tiny(1e-14, -1e-14);
    Vec2 u = tiny.normalized();             // ‖tiny‖ ≪ 1e-12 → {0,0}
    ExpectNear(u, {0.0, 0.0});
}

TEST(Vec2, NormalizedCustomEps) {
    Vec2 v(1e-6, 0.0);
    Vec2 u1 = v.normalized();               // 1e-6 > 1e-12 → normalize
    ExpectNear(u1, {1.0, 0.0});
    Vec2 u2 = v.normalized(1e-3);           // 1e-6 ≤ 1e-3 → zero
    ExpectNear(u2, {0.0, 0.0});
}

TEST(Reflect, AcrossXAxis) {
    Vec2 v(1.2, -3.4);
    Vec2 n{0.0, 1.0};                       // x-axis normal
    Vec2 r = reflect_across_unit_normal(v, n);
    ExpectNear(r, {1.2, 3.4});              // y flips
}

TEST(Reflect, AcrossYAxis) {
    Vec2 v(1.2, -3.4);
    Vec2 n{1.0, 0.0};                       // y-axis normal
    Vec2 r = reflect_across_unit_normal(v, n);
    ExpectNear(r, {-1.2, -3.4});            // x flips
}

TEST(Reflect, DoubleReflectionIsIdentity) {
    Vec2 v(0.3, -0.9);
    Vec2 n{0.0, 1.0};
    Vec2 r  = reflect_across_unit_normal(v, n);
    Vec2 rr = reflect_across_unit_normal(r, n);
    ExpectNear(rr, v, 1e-12);
}

TEST(Reflect, PreservesNormWithUnitNormal) {
    Vec2 v(0.3, -0.9);
    Vec2 n{std::cos(0.7), std::sin(0.7)};   // unit-length
    Vec2 r = reflect_across_unit_normal(v, n);
    EXPECT_NEAR(v.norm(), r.norm(), 1e-12);
}

TEST(Reflect, NormalAndTangentComponentsBehave) {
    Vec2 v(2.0, 5.0);
    Vec2 n{0.6, 0.8};                       // unit normal
    Vec2 t{-n.y, n.x};                       // unit tangent
    Vec2 r = reflect_across_unit_normal(v, n);

    double v_n = v.dot(n), r_n = r.dot(n);
    double v_t = v.dot(t), r_t = r.dot(t);

    EXPECT_NEAR(r_n, -v_n, 1e-12);          // normal component flips
    EXPECT_NEAR(r_t,  v_t, 1e-12);          // tangent component unchanged
}
