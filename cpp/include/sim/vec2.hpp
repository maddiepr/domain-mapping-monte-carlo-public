#pragma once
/**
 * @file vec2.hpp
 * @brief Tiny header-only 2D vector for simulation/geometry.
 * 
 * Provides a POD-style 'Vec2' with double components and the minimal math
 * used by reflection/intersection code: arithmetic, dot product, Euclidean
 * norm, safe normalization, and 'reflect_across_unit_normal'.
 * 
 * Design notes:
 *   - Lightweight and dependency-light (<cmath>).
 *   - Trivial ops are constexpr/inline; no allocations.
 *   - Policy-free: callers own tolerances; 'normalized(eps)' guards small norms.
 * 
 * Contracts:
 *   - 'reflect_across_unit_normal(v, n_hat)' assumes ||n_hat|| = 1.
 *   - 'normalized(eps)' returns {0, 0} when ||v|| <= eps.
 * 
 * See also: reflecting_world.hpp (normals/geometry), step_generators.hpp (steps).
 */

#include <cmath>

namespace sim {

/**
 * @brief Simple POD-like 2D vector (double precision)
 * 
 * Lightweight utility for geometry and numerics. Provided basic arithmetic,
 * dot product, Euclidean norm, and safe normalization.
 */
struct Vec2 {
    double x{0.0};  ///< x-component (defaults to 0)
    double y{0.0};  ///< y-component (defaults to 0)

    /// Default constructor (0,0)
    constexpr Vec2() = default;

    /**
     * @brief Construct with explicit components.
     * @param X X component.
     * @param Y Y component.
     */
    constexpr Vec2(double X, double Y) : x(X), y(Y) {}

    // ---- Arithmetic (non-mutating) ----

    /// @brief Vector addition.
    /// @return {x + r.x, y + r.y}
    constexpr Vec2 operator+(const Vec2& r) const { return {x + r.x, y + r.y}; }

    /// @brief Vector subtraction/
    /// @return {x - r.x, y - r.y}
    constexpr Vec2 operator-(const Vec2& r) const { return {x - r.x, y - r.y}; }

    /// @brief Scalar multiplication.
    /// @return {x * s, y * s}
    constexpr Vec2 operator*(double s) const { return {x * s, y * s}; }

    /// @brief Scalar multiplication (commutative form).
    friend constexpr Vec2 operator*(double s, const Vec2& v) { return {v.x * s, v.y * s}; }

    // ---- Compound assignments (mutating) ----

    /// @brief In-place addition.
    /// @return *this.
    Vec2& operator+=(const Vec2& r) { x += r.x; y += r.y; return *this; }

    /// @brief In-place subtraction.
    /// @return *this.
    Vec2& operator-=(const Vec2& r) { x -= r.x; y -= r.y; return *this; }

    /// @brief In-place scaling by s.
    /// @return *this.
    Vec2& operator*=(double s) { x *= s;   y *= s;   return *this; }

    /// @brief Dot product with r.
    /// @return x*r.x + y*r.y
    constexpr double dot(const Vec2& r) const { return x * r.x + y * r.y; }

    /// @brief Euclidean norm (length).
    /// @return sqrt(x^2 + y^2).
    double norm() const { return std::sqrt(x * x + y * y); }

    /**
     * @brief Unit vector in the same direction.
     * @param eps Threshold below which the vector is treated as zero-length.
     * @return Normalized vector, or {0, 0} if norm <= eps.
     * @note Guards against division by very small magnitudes.
     */
    Vec2 normalized(double eps = 1e-12) const {
        double n = norm();
        if (n <= eps) return {0.0, 0.0};
        return {x / n, y / n};
    };
};

/**
 * @brief Reflect vector across a unit normal.
 * 
 * Computes v' = v - 2 (v · n_hat) n_hat, which flips the component of @p v
 * along @p n_hat and leaves the orthogonal component unchanged.
 * 
 * @param v     Incident vector.
 * @param n_hat Unit-length normal vector (||n_hat|| = 1).
 * @return      Reflected vector.
 * 
 * @pre @p n_hat should be normalized. If it is not, the result is a scaled reflection.
 * @note When @p n_hat is unit-length, the reflection preserves ||v||.
 */
inline Vec2 reflect_across_unit_normal(const Vec2& v, const Vec2& n_hat) {
    double k = v.dot(n_hat);
    return v - (2.0 * k) * n_hat;
};

} // namespace sim