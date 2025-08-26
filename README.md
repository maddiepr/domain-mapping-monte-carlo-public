# Domain Mapping Monte Carlo (Skeleton Repository)


**High-performance, SLURM-orchestrated Monte Carlo pipeline for solving PDEs in conformally mapped domains.**

*This is a public-safe version of my master's project at UNC Chapel Hill.* Core numerical kernels are intentionally redacted for IP reasons, but the repository preserves the full workflow: orchestration, job scheduling, and modular structure to demonstrate clarity, scalability, and HPC-readiness.

> **Status:** Active modules: `cpp/`, `scripts/`, `tests/`. `python/` visualizations are being prepared for public release.

---

## Key Features

- **Conformal mapping framework** for PDEs on 1/2- 1/4- and 1/8-plane geometries.
- **Modular architecture** separating geometry, RNG, step generators, and the simulation loop.
- **Specular reflection logic** with robust boundary handling.
- **SLURM-ready** batch jobs for cluster scaling (Longleaf-friendly).
- **Reproducibility-oriented** structure (parameter files and scripted pipelines).
- **CI-enabled:** GitHub Actions runs CMake + GoogleTest on pushes/PRs.
- **Public-safe:** numerical stepping code redacted; interfaces and workflows retained.

---

## Quickstart (public-safe)

1. Build & test the C++ components
```bash
mkdir -p build && cd build
cmake -DC_MAKE_BUILD_TYPE=Release ..
cmake --build . -j
ctest --output-on-failure -j
```

2. Generate parameter sweeps
```bash
./generate_params.sh
head -n 5 params_list.txt   # peek at the first few tasks
```

3. SLURM dry-run (validates scripts without submitting)
```bash
sbatch --test-only ../scripts/slurm/MC_quarter.slurm
sbatch --test-only ../scripts/slurm/MC_eighth.slurm
```

**Note**: Active modules today: `cpp/`, `scripts/`, `tests/`
`python/` visualization notebooks/scripts are being prepared for public release. 

---

## Workflow Overview

1. **Generate parameters** (```generate_params.sh```)
    Produces a parameter sweep file (```params_list.txt```) for batch execution.
2. **Run simulations** (```MC_quarter.slurm``` / ```MC_eighth.slurm```)
    Submits a SLURM array job (one task per parameter set). Use ```--test-only``` for validation in this public skeleton.
3. **Visualize results** (```visualize_quarter.slurm``` / ```visualize_eighth.slurm```)
    Post-processing scripts (Python) to visualize outputs - **coming soon in the public repo**.
4. **Orchestrate everything** (```run_all.sh```)
    End-to-end driver for parameter generation, simulation submission, and visualization.

---

## Layout (lean)

```text
cpp/                C++ core (engine, geometry, RNG)
tests/              GoogleTest suites (run via CTest/CI)
scripts/slurm/      Longleaf-ready SLURM jobs + viz
.github/            CI workflow
fortran/            Reference solvers
python/             Visualizations (public release WIP)
```

---

## Full Project Tree

```text
domain-mapping-monte-carlo-public/          <- Project root for domain-mapped Monte Carlo sims
├── .github/workflows/                      <- GitHub Actions CI configs
│   ├── cmake-tests.yml                     <- Build + run tests on pushes/PRs
├── cpp/                                    <- C++ library/executables (primary code)
│   ├── include/                            <- Public headers (installed/exposed API)
│   │   ├── sim/                            <- C++ namespace folder
│   │   │   ├── io.hpp                      <- I/O helpers (params/results, simple file ops)
│   │   │   ├── reflecting_world.hpp        <- Reflecting boundary/world definitions & API
│   │   │   ├── rng.hpp                     <- RNG wrapper(s) and seeding utilities
│   │   │   ├── simulation.hpp              <- Simulation facade (step loop, config, hooks)
│   │   │   ├── step_generators.hpp         <- Step distributions/factories (e.g., Gaussian)
│   │   │   └── vec2.hpp                    <- Minimal 2D vector math (ops, norms, reflect)
│   │   └── .gitkeep                        <- Ensures empty dir tracked by git
│   ├── src/                                <- C++ implementation
│   │   ├── CMakeLists.txt                  <- Targets/sources for this subdir
│   │   ├── reflecting_world.cpp            <- Impl for reflecting geometry & queries
│   │   ├── rng.cpp                         <- Impl for RNG wrapper(s)
│   │   ├── simulation.cpp                  <- Impl for main simulation engine
│   │   └── step_generators.cpp             <- Impl for step generation logic
│   └── CMakeLists.txt                      <- Library/executable definitions for cpp/
├── extern/googletest/                      <- Vendored GoogleTest (for unit tests)
│   └── ....                                <- Upstream contents (managed by CMake/FetchContent)
├── fortran/                                <- Reference/benchmark solvers in Fortran
│   └── src/
│   │   ├── mc_quarter.f                    <- Quarter-plane reference
│   │   └── mc_eighth.f                     <- Eighth-plane reference
├── python/                                 <- Python visualization scripts
│   └── **on way**                          <- Placeholder; incoming scripts/modules
├── scripts/                                <- Automation scripts
│   └── slurm/                              <- SLURM batch jobs for HPC runs/plots
│   │   ├── MC_eight.slurm                  <- Submit eighth-plane simulation
│   │   ├── MC_quarter.slurm                <- Submit quarter-plane simulation
│   │   ├── visualize_eighth.slurm          <- Post-processing/plots for eighth-plane
│   │   └── visualize_quarter.slurm         <- Post-processing/plots for quarter-plane
├── tests/                                  <- Unit/integration tests (GoogleTest + CTest)
│   ├── CMakeLists.txt                      <- Test target definitions
│   ├── test_reflecting_world.cpp           <- Reflecting/boundary behavior tests
│   ├── test_rng.cpp                        <- RNG properties (seed, distribution checks)
│   ├── test_sanity.cpp                     <- Smoke test
│   ├── test_simulation.cpp                 <- End-to-end sim behavior/regression tests
│   ├── test_step_generators.cpp            <- Step generator correctness/variance
│   └── test_vec2.cpp                       <- Vec2 arithmetic/invariants
├── .gitignore                              <- Ignore build artifacts, caches, etc.
├── CMakeLists.txt                          <- Top-level CMake (project, options, externals)
├── CONTRIBUTING.md                         <- How to contribute, style, PR flow
├── generate_params.sh                      <- Produce parameter grids/files for SLURM runs
├── LICENSE                                 <- License for this repo
├── README.md                               <- You are here
└── run_all.sh                              <- Convenience script to execute all SLURM files
```

---

## About This Repository

- **Purpose:** Demonstrate design and implement of large-scale scientific-computing workflows for HPC (SLURM) environments.
- **What runs today:** `cpp/`, builds & tests pass; `scripts/` (SLURM) validate with `--test-only`; `tests/` run via CTest. `python/` visualizations are WIP for the public repo.
- **Redactions:** Select numerical stepping/solver kernels are removed for IP reasons; interfaces, workflow, and orchestration remain intact for evaluation.
- **For recruiters/hiring managers:** The structure, CI, and SLURM ergonomics mirror real HPC projects. A full, runnable version can be shared privately upon request.

---

## License

This project is released under the MIT License. See `LICENSE` for details.
[![License: MIT](https://img.shields.io/badge/License-MIT-lightgrey.svg)](LICENSE)

---

## Contact

**Maddie Preston**  
📫 [linkedin.com/in/madeline-preston](https://www.linkedin.com/in/madeline-preston)  
📄 Resume available upon request
