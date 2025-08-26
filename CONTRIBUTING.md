# Contributing Guidelines

Thank you for your interest! This repo is a **public, redacted skeleton** of a Monte Carlo framework for PDEs in conformally mapped domains. While some numerical kernels are intentionally omitted, contributions that improve the **build, tests, CI, SLURM ergonomics, docs, and example scaffolding** are welcome.

---

## Scope & Redactions

- **In scope:** C++ build system (CMake), unit tests (GoogleTest/CTest), CI workflows, SLURM scripts, docs/readme, example parameter generation.
- **Out of scope:** Core/redacted numerical stepping & proprietary solver code (PRs changing those interfaces will be declined).
- **For transparency:** The repository mirrors real HPC project structure; a full runnable version can be reviewed privately on request.

---

## Getting Started

### Clone & build (C++)

```bash
git clone https://github.com/maddiepr/domain-mapping-monte-carlo-public.git
cd domain-mapping-monte-carlo-public
mkdir -p build && cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . -j
ctest --output-on-failure -j
```

### SLURM validation (no submission)

```bash
./generate_params.sh
sbatch --test-only scripts/slurm/MC_quarter.slurm
sbatch --test-only scripts/slurm/MC_eighth.slurm
```

**Note** `python/` visualizations are being prepared for public release. Please don't open issues about missing Python notebooks yet.

### Fortran (optional references)

If you're working with the reference solvers:
```bash
gfortran -O3 -o mc_quarter fortran/src/mc_quarter.f
gfortran -O3 -o mc_eighth fortran/src/mc_eighth.f
```

---

## How to Contribute

1. **Fork** the repo and create a topic branch:
    ```bash
    git checkout -b chore/ci-fix    # or docs/... , test/... fix/...
    ```
2. **Make small focused changes** with clear commit messages
   (Conventional Commits appreciated: `docs:`, `tests:`, `ci:`, `fix:`, `chore:`).
3. **Include tests** when changing C++ behavior or SLURM logic.
4. **Run locally:** build + `ctest`; validate SLURM with `--test-only`.
5. **Open a PR** with a concise description and rationale.

PRs are reviewed as time allows (personal project).

---

## Code Style

- **C++17:** header-only where sensible; `#pragma once`; prefer `std::` facilities; keep functions small & testable.
- **Formatting:** if a `.clang-format` exists, run it; otherwise, match surrounding style.
- **Tests:** use GoogleTest; keep tests deterministic (fixed seeds when checking RNG properties).
- **Shell/SLURM:** comment inputs/outputs; avoid cluster-specific hard-coding; support `--test-only`.

(If you propose adding a formatter or CI lint step, include the config in your PR.)

---

## Issues & Feedback

Please include:
- Repro steps (commands, OS/comilers, cmake version)
- Expected vs. actual behavior
- `CMakeCache.txt` and relevant build logs (trimmed)
- For SLURM: show `sbatch --test-only` output

Feature requests are welcome if they fit the **in-scope** areas above.

---

## Code of Conduct

Be respectful and constructive. Disagreements are fine; harassment is not.

--- 

## License & Third-Party Code

By contributing, you agree your changes are licensed under the repository's **MIT License** (see `LICENSE`). Third party components (e.g., GoogleTest) remain under their respective licenses.

---

## Contact

**Maddie Preston**

[LinkedIn:](www.linkedin.com/in/madeline-preston)