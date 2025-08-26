// test_rng.cpp
#include "sim/rng.hpp"
#include <gtest/gtest.h>

// 1. Check for reproducibility
TEST(RNGTest, Reproducibility) {
    sim::RNG rng1(42), rng2(42);
    for (int i = 0; i < 1000; i++) {
        EXPECT_EQ(rng1.gauss(), rng2.gauss());
    }
}

// 2. Check distribution
TEST(RNGTest, DistributionSanity) {
    sim::RNG rng3(123);
    double sum = 0.0, sumsq = 0.0;
    const int N = 100000;
    for (int i = 0; i < N; ++i) {
        double x = rng3.gauss();
        sum += x;
        sumsq += x * x;
    }
    double mean = sum / N;
    double var = sumsq / N - mean * mean;

    EXPECT_NEAR(mean, 0.0, 0.05);
    EXPECT_NEAR(var, 1.0, 0.05);
}