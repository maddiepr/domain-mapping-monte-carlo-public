/// @file reflecting_world.cpp
/// @brief Implementation notes for specular reflections across straight-line walls.
/// @details See reflecting_world.hpp for the public API/contract.

/// Implementation outline (advance_with_reflections):
/// 1) p ← x; v ← d.
/// 2) While |v| > 0 and bounces < MAX_REFLECTIONS:
///    • Scan all walls for earliest time-of-impact t ∈ (0,1]; ignore near-parallel (EPS_DIR)
///      and out-of-segment hits (with small endpoint tolerance).
///    • If none: p += v; break.
///    • Move to contact: p += t·v; nudge inside by EPS_POS·n̂.
///    • Reflect remainder: v ← reflect_across_unit_normal((1−t)·v, n̂).
/// 3) Write back: x ← p.

/// Numerical policy:
///  - Unit normals validated at add time (assert in debug).
///  - EPS_DIR for grazing/parallel rejection; EPS_POS to de-stick after impact.
///  - Small endpoint tolerance for robust corner handling; prefer squared norms where possible.

/// Determinism:
///  - Simultaneous hits resolved by wall id, then insertion order (reproducible with fixed seed).

/// Complexity:
///  - O(#walls × bounces) per call; no dynamic allocation. Add broad-phase only if profiling warrants.

/// Test coverage (see tests/):
///  - Normal/oblique hits, start-on-wall stability, corner/endpoint contacts,
///    multi-bounce in a box, grazing trajectories.


#include "sim/reflecting_world.hpp"
#include <cassert>
#include <cmath>
#include <limits>
#include <algorithm>

namespace sim {

    // ===================================================================
    // Local helpers (file scope only)
    // Purpose: small numerics used by this TU; not part of public API.
    // Notes:
    //  - normalize() returns {0,0} if the input is too small (<= EPS_DIR).
    //    Callers must handle that sentinel explicitly.
    // ===================================================================

    /// Cross product for 2D vectors: a.x * b.y - a.y * b.x
    /// Useful for orientation tests; included here for completeness.
    static inline double cross2(const Vec2& a, const Vec2& b) {
        return a.x * b.y - a.y * b.x;
    }

    /// Euclidean norm (length) of a 2D vector.
    static inline double norm(const Vec2& v) {
        return std::sqrt(v.x * v.x + v.y * v.y);
    }

    /// Return a unit-length copy of v; returns {0,0} if ||v|| <= EPS_DIR.
    static inline Vec2 normalize(const Vec2& v) {
        const double n = norm(v);
        if (n <= EPS_DIR) return Vec2{0.0, 0.0};
        return Vec2{v.x / n, v.y / n};
    }
    
    // ===============================
    // WallSegment builders
    // ===============================

    /**
     * @brief Build a wall segment and auto-compute a unit normal from geometry.
     * 
     * Algorithm:
     *   1. Compute tangent t = b - a and its unit vector t_hat.
     *   2. Take the left-handed normal n = (-t_hat.y , t_hat.x).
     *   3. Optionally flip for inwarde-facing conventions.
     * 
     * Requirements:
     *   - a and b must not coincide (within EPS_DIR), otherwise t_hat = {0,0}.
     *   - The returned normal is unit-length if t_hat is valid.
     */
    WallSegment WallSegment::fromSegmentAutoNormal(const Vec2& a, 
                                                   const Vec2& b,
                                                   bool inward, 
                                                   int id_) {
        Vec2 t     = Vec2{b.x - a.x, b.y - a.y};
        Vec2 t_hat = normalize(t);

        // Enforce non-degenerative segment in debug builds
        assert(!(t_hat.x == 0.0 && t_hat.y == 0.0) &&
                "fromSegmentAutoNormal: degenerate segment (a ~== b)");

        // Left-hand unit normal of the tangent
        Vec2 n = Vec2{-t_hat.y, t_hat.x};
        if (inward) n = Vec2{-n.x, -n.y};   // flip if requested

        #ifndef NDEBUG
            // Defensive: keep the unit-normal invariant honest
            const double nlen = std::sqrt(n.x*n.x + n.y*n.y);
            assert(std::abs(nlen - 1.0) <= 1e-12 && "Auto Normal should be unit length");
        #endif
            return WallSegment{a, b, n, id_};        
    }

    // =====================================
    // ReflectingWorld: convenience builders
    // =====================================

    /**
     * @brief Add a finite segment with an explicit outward norm.
     * 
     * Notes:
     *   - The provided normal is normalized here so stored normals are unit.
     *   - Insertion order is preserved for deterministic tie-breaks.
     *   - Debug-only assertions catch invalid inputs early; no runtime cost in release.
     */
    void ReflectingWorld::add_segment(const Vec2& a,
                                      const Vec2& b,
                                      const Vec2& n_unit,
                                      int id) {
        #ifndef NDEBUG
            const Vec2 ab{ b.x - a.x, b.y - a.y};
            const bool nearly_zero = (std::abs(ab.x) <= EPS_DIR && std::abs(ab.y) <= EPS_DIR);
            assert(!nearly_zero && "add_segment: degenerate segment (a~==b)");
        #endif
            const Vec2 n_hat = normalize(n_unit);
            assert(!(n_hat.x == 0.0 && n_hat.y == 0.0) &&
                    "add_segment: provided normal is near zero");
            
            walls.push_back(WallSegment{ a, b, n_hat, id});
    }

    /**
     * @brief Add a finite segment and auto-compute its outward normal using LHR 
     *  (optionally flipped inward).
     */

    void ReflectingWorld::add_segment_auto(const Vec2& a,
                                           const Vec2& b,
                                           bool inward,
                                           int id) {
        walls.push_back(WallSegment::fromSegmentAutoNormal(a, b, inward, id));
    }

    /**
     * @brief Add an inward-facing axis-aligned box [xmin, xmax] x [ymin, ymax].
     * 
     * Face normals (inward):
     *   - Bottom  : (0, +1)
     *   - Right   : (-1, 0)
     *   - Top     : (0, -1)
     *   - Left    : (+1, 0)
     * 
     * IDs are assigned deterministically: base_id ... base_id+3
     */
    void ReflectingWorld::add_inward_box(double xmin,
                                        double xmax,
                                        double ymin,
                                        double ymax,
                                        int base_id) {
        #ifndef NDEBUG
            assert(xmin < xmax && ymin < ymax && "add_inward_box: invalid bounds.");
        #endif
            // Bottom
            add_segment(Vec2{xmin, ymin}, Vec2{xmax, ymin}, Vec2{0.0,  1.0}, base_id + 0);
            // Right
            add_segment(Vec2{xmax, ymin}, Vec2{xmax, ymax}, Vec2{-1.0, 0.0}, base_id + 1);
            // Top
            add_segment(Vec2{xmax, ymax}, Vec2{xmin, ymax}, Vec2{0.0, -1.0}, base_id + 2);
            // Left
            add_segment(Vec2{xmin, ymax}, Vec2{xmin, ymin}, Vec2{1.0,  0.0}, base_id + 3);
    }

    /**
     * @brief Approximate the infinite boundary { x | n·x = c } with a long finite segment.
     * 
     * Parameters:
     *   - n_unit   : unit normal pointing towards the allowed side (normalized here).
     *   - c        : line offset so that n·x = c
     *   - span     : total length of the finite segment centered at c·n_hat.
     * 
         * Guidance:
 *   - Choose 'span' comfortably larger than your simulatione extent to avoid end hits.
     */
    void ReflectingWorld::add_half_plane_strip(const Vec2& n_unit,
                                           double c,
                                           double span,
                                           int id) {
        Vec2 n = normalize(n_unit);
        assert(!(n.x == 0.0 && n.y == 0.0) &&
               "add_half_plane_strip: provided normal is near-zero");
        
        // A point on the line ( ||n|| = 1: x0 = c·n_hat)
        Vec2 x0{ n.x * c, n.y * c };

        // Any unit tangent perpendicular to n_hat (rotate +90 degrees)
        Vec2 t_hat{ -n.y, n.x };        
        
        // Long finite segment centered at x0 along the line
        Vec2 a{ x0.x - 0.5 * span * t_hat.x, x0.y - 0.5 * span * t_hat.y };
        Vec2 b{ x0.x + 0.5 * span * t_hat.x, x0.y + 0.5 * span * t_hat.y };
        add_segment(a, b, n, id);
    }

    // ===============================
    // Advance with specular reflections
    // ===============================

    /**
     * @brief Advance a 2D point by a proposed displacement with specular reflections.
     * 
     * Traces the path p -> p + d through the ReflectingWorld, reflecting off the
     * earliest-hat wall(s) in time-of-impact order until the displacement is exhausted
     * or the reflection cap is reached.
     * 
     * Numerical safeguards:
     *   - Parallel/grazing cases are rejected with EPS_DIR.
     *   - Small EPS_POS tolerance is used both for endpoint/corner tests and to
     *     "nudge" the position off a wall after impact (prevents immediate self-
     *     collision due to floating-point roundoff).
     * 
     * Determinism:
     *   - If multiple walls are hit at nearly the same time, ties are broken
     *     deterministically by wall id and then by insertion order.
     * 
     * Safety:
     *   - At most MAX_REFLECTIONS bounces are processed. If this cap is reached,
     *     the function halts deterministically at the last computed point and
     *     discards any leftover displacement.
     */
    void advance_with_reflections(Vec2& x, Vec2 d, const ReflectingWorld& world) {
        Vec2 p = x;   // current position
        Vec2 v = d;   // remaining displacement

        // Early exit: no meaningful displacement
        if (std::abs(v.x) <= EPS_DIR && std::abs(v.y) <= EPS_DIR) {
            x = p;
            return;
        }

        int bounces = 0;
        while (bounces < MAX_REFLECTIONS) {
            double best_t   = std::numeric_limits<double>::infinity(); // earliest hit along v
            int    best_idx = -1;                                      // index of hit wall
            int    hit_id   = -1;                                      // id of hit wall
            Vec2   hit_n{0.0, 0.0};                                    // normal of hit wall

            // -------------------------------------------------
            // Scan all walls for the earliest valid intersection
            // -------------------------------------------------
            for (std::size_t i = 0; i < world.walls.size(); ++i) {
                const WallSegment& w = world.walls[i];
                const Vec2 a = w.p0;
                const Vec2 b = w.p1;
                const Vec2 s{ b.x - a.x, b.y - a.y }; // segment direction

                const double denom = cross2(v, s);
                if (std::abs(denom) <= EPS_DIR) continue; // reject parallel/near-parallel

                // Solve intersection: p + t v = a + u s
                const Vec2 ap{ a.x - p.x, a.y - p.y };
                const double t = cross2(ap, s) / denom;
                
                // Require hit strictly ahead (t > 0) and within this displacement (t <= 1).
                if (t <= EPS_POS || t > 1.0 + EPS_POS) continue;

                const double u = cross2(ap, v) / denom;
                // Require intersection point to lie on the finite segment (0 <= u <= 1)
                if (u < -EPS_POS || u > 1.0 + EPS_POS) continue;

                // If this is the earliest hit so far, keep it.
                // Ties (nearly equal t) are broken by wall id, then insertion index.
                if (t < best_t - 1e-15 ||
                    (std::abs(t - best_t) <= 1e-15 && 
                    (w.id < hit_id || (w.id == hit_id && static_cast<int>(i) < best_idx)))) {
                    best_t  = t;
                    best_idx = static_cast<int>(i);
                    hit_n   = w.n_hat;
                    hit_id  = w.id;
                }
            }
            
            // ---------------------------------------------------
            // No wall hit: finish remaining displacement and exit
            // ---------------------------------------------------
            if (best_idx < 0 || !std::isfinite(best_t)) {
                p += v;
                break;
            }

            // ------------------------------------------------
            // Process hit: move to impact point, nudge inside,
            // then reflect remaining displacement.
            // ------------------------------------------------
            // Move p to exact contact point
            p += Vec2{ v.x * best_t, v.y * best_t };

            // Nudge slightly along normal to prevent "sticking"
            p += Vec2{ hit_n.x * EPS_POS, hit_n.y * EPS_POS };

            // Remaining displacement after consuming fraction best_t
            const Vec2 v_remain{ v.x * (1.0 - best_t), v.y * (1.0 - best_t) };
            
            // Reflect remainder across wall normal (specular reflection)
            v = reflect_across_unit_normal(v_remain, hit_n);

            ++bounces;

            // Early exit: nothing left to move
            if (std::abs(v.x) <= EPS_DIR && std::abs(v.y) <= EPS_DIR) {
                break;
            }
        }

        // Write back final position (whether natural end or safet stop)
        x = p;
    }

} // namespace sim 