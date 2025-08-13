# Domain Mapping Monte Carlo (Skeleton Repo)

A redacted version of my thesis project exploring Monte Carlo methods for solving partial differential equations (PDEs) in conformally mapped domains. This repository outlines the modular simulation architecture used for domain-mapped Monte Carlo solvers, including geometric reflection logic and simulation pipeline orchestration. Core numerical methods have been redacted for intellectual property reasons but full documentation and structure remain to highlight reproducibility, HPC readiness, and multi-domain adaptability.

---

## Features

- Conformal mapping strategy for solving PDEs in complex domains
- Modular architecture supporting multiple geometries and parameter sets
- Reflection logic for 1/2, 1/4, and 1/8-plane simulations
- SLURM-ready batch execution for HPC scaling
- Code structured for clarity, extensibility, and reproducibility
- Public-safe: all sensitive numerical code removed, safe for sharing

---

## Project Structure

```text
domain-mapping-monte-carlo/
├── README.md
├── Parameters/
│ ├── create_param_grid.py
│ └── param_sets/
│ └── quarter_plane_set.txt
├── Run_scripts/
│ ├── run_all_simulations.sh
│ └── slurm_visualize.sh
├── Simulations/
│ ├── simulate_reflection.py
│ └── simulate_weighted_walks.py
├── Fortran_kernels/
│ ├── reflect_walk.f90
│ └── update_weights.f90
├── Visualization/
│ ├── visualize_histogram.py
│ ├── compute_error.py
│ └── convergence_plots.py
├── Data/
│ ├── raw_output/
│ └── processed_results/
├── Utils/
│ ├── io_helpers.py
│ └── job_tracker.py
└── SLURM_jobs/
├── job_array_sim.sh
└── logs/
```

---

## Disclaimer on Redacted Content

This repository provides only the scaffolding and architecture of the simulation pipeline. All core stepping and numerical integration routines have been removed or replaced with placeholders to protect research code that is currently unpublished.

If you're an employer or collaborator interested in the full codebase, I’m happy to provide access upon request.

---

## License

This project is open-source under the MIT License. See `LICENSE` for details.

---

## Contact

**Maddie Preston**  
📫 [linkedin.com/in/madeline-preston](https://www.linkedin.com/in/madeline-preston)  
📄 Resume available upon request


