#pragma once
/**
 * @file reflecting_world.hpp
 * @brief Geometry + reflection engine for 2D particle motion.
 *
 * What the file is for:
 *   A lightweight "world" that enforces specular reflections (angle in = angle out)
 *   when a particle's proposed displacement crosses one or more straight-line boundaries.
 *   It is agnostic to the physics that generated the displacement 
 *   (Brownian motion, drift, mapping-based steps, etc.) 
 *
 * Why it exists:
 *   - Separate boundary handling from particle stepping/physics.
 *   - Support multiple boundaries and multiple reflections within a single step
 *     (e.g., long steps inside a box).
 *   - Offer a reusable machanism for enforcing geometry across domains without
 *     hard-coding parameters or rules.
 * 
 * Core concepts: 
 *   - Wall segment: a finite line segment with a "unit" outward normal; used as
 *     the primitive reflecting boundary. Normal orientation should be consistent
 *     within a domain (e.g., inward for boxes).
 *   - Reflecting world: a collection of wall segments plus convenience builders
 *     (axis-aligned box, long "half_plane", etc.). Stores only geometry.
 *   - Advance with reflections: given a current position and a proposed 
 *     displacement, computes the final position after reflecting off the easliest 
 *     hit wall, possibly several times, or until a safety limit is reached.
 * 
 * Behavior guarantees:
 *   - Specular reflection: v' = v - 2 (v·n̂) n̂ with *unit* n̂.
 *   - Earliest hit: among all candidate intersections, the smallest positive
 *     travel parameter is taken first; ties are resolved deterministically
 *     (by wall id, then insertion index).
 *   - Multiple reflections per step: repeats the remaining path length 
 *     until exhausted or the reflection cap is hit.
 *   - Physics-agnostic: consumes a displacement vector; does not know about 
 *     dt, D, drift, or the RNG.
 * 
 * Numerical safeguards:
 *   - Unit normals: validated/normalized at add-time to avoid silent errors.
 *   - Epsilons: small tolerances are used for:
 *          - near parallel/grazing incidence,
 *          - shared vertex/endpoint hits, and
 *          - post-hit "nudges" to prevent sticking due to floating-point roundoff.
 *   - Safety cap: MAX_REFLECTIONS per advance (compile-time constant) to prevent 
 *     infinite loops on numerical corner cases.
 * 
 * Corner & endpoints:
 *   - Intersections shared vertices are resolved by a deterministic tie-break,
 *     ensuring run-to-run reproducibility with the same seed.
 *   - True corner normals are undefined; the engine treats the event as a hit on 
 *     one wall per the tie-break rule.
 * 
 * Extensibility:
 *   - More shapes: additional builders (polygons, circles via polygonal approx.)
 *     can compose down to wall segments.
 *   - Other boundary types: absorbing/periodic can be added in parallel modules
 *     without changing particle or RNG code.
 *   - Metadata: the advancer can be extended to return stats (e.g., bounce count
 *     last-hit wall id) for diagnostics.
 * 
 * Interactions with other modules:
 *   - vec2.hpp: 2D vector math (dot products, reflection helper).
 *   - step_generators.hpp: produces the proposed displacement; no boundary checks.
 *   - simulation.hpp: orchestrates time stepping, calls the world to enforce
 *     geometry, and optionally records histories.
 * 
 * Suggested tests (validation):
 *   - Normal/oblique incidence on a single wall (angle preserved).
 *   - Start exactly on a wall (no jitter loops; immediate inward motion).
 *   - Endpoint/corner hits in a box (deterministic outcome).
 *   - Long steps producing multiple bounces (time-of-impact ordering).
 *   - Near-tangent "grazing" steps (stable, no-explosive behavior).
 */

#include <vector>
#include "sim/vec2.hpp"

namespace sim {

// ====================================================
// Numerical tolerances / safety limits (dimensionless)
// Purpose: make intersection/reflection logic robust 
// to FP noise and corner cases.
// Tune only if your coordinate scales differ by many orders of magnitude.
// ====================================================

constexpr double EPS_POS = 1e-12;   // post-hit nudge off the wall to avoid immediate re-collision
constexpr double EPS_DIR = 1e-12;   // near-parallel/grazing threshold for intersection tests
constexpr int MAX_REFLECTIONS = 64; // per-advance cap; stops FP corn loops deterministically

// ===============================
// Wall representation
// ===============================

/**
 * @brief Straight wall segment from p0 to p1 with a unit outward normal n_hat.
 *
 * Invariants / expectations:
 *   - n_hat is normalized (||n_hat|| = 1). The reflection formula v' = v - 2 * (v · n_hat) n_hat
 *     assumes a unit normal; non-unit normals distort angles/speeds.
 *   - The segment is non-degenerative (p0 != p1 within tolerance).
 *   - Normal orientation is consistent across the world (e.g., all inward for a box).
 * 
 * Numerical / algorithm notes:
 *   - Endpoints are treated as part of the segment with a small tolerance;
 *     ties at corners are broken deterministically by wall id then index.
 *   - Orientation (p0 -> p1) determines the "auto" normal via the left-hand rule.
 * 
 * Usage tip:
 *   - Prefer add-time normalization (see add_segment/add_segment_auto) so all segments
 *     stored here are unit-consistent.
 */
struct WallSegment {
    Vec2 p0;     ///< Segment start point
    Vec2 p1;     ///< Segment end point
    Vec2 n_hat;  ///< Unit outward normal (must be normalized)
    int  id{-1}; ///< Optional identifier (used for deterministic tie-breaks).

    WallSegment() = default;

    /// Construct with explicit (already unit) normal.
    WallSegment(const Vec2& a, const Vec2& b, const Vec2& n_unit, int id_ = -1)
        : p0(a), p1(b), n_hat(n_unit), id(id_) {}

    /**
     * @brief Build a segment and auto-compute a unit normal from its geometry.
     * Uses the left-hand normal of the tangent (p0 -> p1).
     */
    static WallSegment fromSegmentAutoNormal(const Vec2& a,
                                             const Vec2& b,
                                             bool inward = false,
                                             int id_ = -1);
};

/**
 * @brief Lightweight container of reflecting line segments with convience builders.
 * 
 * Responsibilities:
 *   - Store finite wall segments with "unit" outward normals.
 *   - Provide helpers to add commone geometries (single segment, auto-normal segemnt,
 *     inward box, half-plane strip).
 * 
 * Invariants & determinism:
 *   - All stored normals are normalized at add time (||n_hat|| = 1).
 *   - Segments are expected non-generate (p0 != p1 within tolerance).
 *   - When multiple walls are hit at the same time (within tolerance),
 *     the advancer breaks ties deterministically by 'id' then by insertion index.
 * 
 * Scope:
 *   - No broad-phase acceleration (AABB/BVH) here; naive O(#walls) is used in the advancer.
 *   - No self-intersection checks; callers ensure a coherent world.
 *   - Mutation is not thread-safe during stepping.
 */
struct ReflectingWorld {
    std::vector<WallSegment> walls;

    /**
     * @brief Add a segment with an explicit outward normal.
     * @param a,b       Endpoints of the finite segment (must not coincide).
     * @param n_unit    Outward normal; will be normalized internally.
     * @param id        Optional identifier for debugger/tie-breaks.
     * 
     * Notes: 
     *   - Orientation of 'n_unit' defines which side is "interior/allowed".
     *   - Prefer consistent normal orientation across all walls (e.g., inward for boxes).
     */
    void add_segment(const Vec2& a, const Vec2& b, const Vec2& n_unit, int id = -1);

    /**
     * @brief Add a segment and auto-compute its outward normal from geometry.
     * Uses the left-hand normal of the tangent (p0 -> p1). If 'inward==true', flips it.
     * @param a,b       Endpoints of the segment
     * @param inward    Flip the auto normal (useful when building inward-facing boxes).
     * @param id        Optional identifier for debugging/tie-breaks.
     */
    void add_segment_auto(const Vec2& a, const Vec2& b, bool inward = false, int id = -1);

    /**
     * @brief Add an axis-aligned reflecting box [xmin, xmax] x [ymin, ymax] with inward normals.
     * @param xmin, xmax    Horizontal bounds   (require xmin < xmax)
     * @param ymin, ymax    Vertical bounds     (require ymin < ymax)
     * @param base_id       Base id for the four walls; walls use base_id..base_id+3
     */
    void add_inward_box(double xmin, double xmax, double ymin, double ymax, int base_id = 100);

    /**
     * @brief Approximate the infinite half-plane boundary { x | n·x = c} with a very long finite
     *        segment. The allowed side is the direction of the provided unit normal 'n_unit'.
     *
     * @param n_unit Unit normal of the boundary line (points to allowed side); will be normalized
     * @param c      Offset so that the line is { x | n·x = c }
     * @param span   Length of finite segment used to approximate the infinite line (centered on c·n).
     * @param id     Optional id for debugging/tie-breaks.
     * 
     * Guidance:
     *   - Choose 'span' much larger than the horizontal extent you expect (e.g., 1e8 or 1e9) to avoid
     *     artificial end hits. For a true infinite boundary, consider a dedicated type in the future.
     *   - Typical half-plane for (u, v) with v>=0: n_unit=(0,1), c=0.
     */
    void add_half_plane_strip(const Vec2& n_unit, double c, double span = 1e6, int id = 200);
};

// ===============================
// Public API
// ===============================

/**
 * @brief Advance a 2D position by a proposed displacement with specular reflections.
 *
 * Traces the path x -> x + d through the scene, reflecting on the earliest-hit wall(s)
 * in time-of-impact order until the remaining displacement is exhausted or a safety cap 
 * is reached.
 * Tangential components are preserved; the normal component is mirrored (specular).
 *
 * @param x      [in,out] Current position; overwritten with the final position after all reflections.
 * @param d      Proposed displacement for this time step (e.g., drift + diffusion). Not modified.
 * @param world  Geometry container with finite line segments and "unit" outward normals.
 * 
 * @pre     All wall normals are unit length; segments are non-degenerate. (Enforced at add time.)
 * @post    If no wall is hit, x <- x + d. Otherwise x is the endpoint after zero or more refections.
 * @post    If multiple walls are hit at the same time (within tolerance), a deterministic tie-break
 *          is used (by wall id, then insertion index).
 * 
 * Numerical policy:
 *   - Uses small epsilons to 
 *      1. reject near-parallel intersections and
 *      2. nudge off a wall after impact.
 *   - Endpoints are treated with a small positional tolerance for corner robustness.
 * 
 * Safety:
 *   - Processes at most MAX_REFLECTIONS bounce per call. If the cap is reached, the function
 *     stops at the last computed point and drops the leftover displacement (deterministic fail-safe).
 * 
 * Complexity:
 *      O(#wall x number_of_bounces) per call. No dynamic allocation. Thread-safe w.r.t. world (read-only).
 */
void advance_with_reflections(Vec2& x, Vec2 d, const ReflectingWorld& world);

} // namespace sim 
