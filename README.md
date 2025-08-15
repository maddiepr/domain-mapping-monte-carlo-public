# Domain Mapping Monte Carlo (Skeleton Repository)

**High-performance, SLURM-orchestrated Monte Carlo simulation pipeline for solving PDEs in conformally mapped domains. Redacted for public release while preserving full workflow structure, HPC scalability, and reproducibility features.**

A public-safe version of my thesis project exploring Monte Carlo methods for solving partial differential equations (PDEs) in complex geometries via conformal mapping. While core numerical routines are removed for intellectual property reasons, the repository retains its full orchestration, job scheduling, and modular structure to demonstrate clarity, scalability, and HPC-readiness.

---

## Key Features

- **Conformal mapping framework** for PDEs in complex domains.
- **Modular architecture** supporting multiple geometries and parameter sets
- **Reflection logic** for 1/2, 1/4, and 1/8-plane simulations
- **SLURM-ready** batch execution for HPC scaling
- **Reproducibility-focused** code for organization and parameter management
- **Safe for public-sharing:** numerical stepping code redacted

---

## Workflow Overview

1. **Generate parameters** (```generate_parms.sh```)
    Produces a parameter sweep file (```params_list.txt```) for batch execution.
2. **Run simulations** (```MC_quarter.slurm``` / ```MC_eighth.slurm```)
    Submits a SLURM array job, one task per parameter set.
3. **Visualize results** (```visualize_quarter.slurm``` / ```visualize_eighth.slurm```)
    Post-processing scripts (Python) to visualize simulation outputs.
4. **Orchestrate everything** (```run_all.sh```)
    End-to-end driver for parameter generation, simulation submission, and visualization.

---

## Project Structure

```text
domain-mapping-monte-carlo/
├── .gitignore
├── generate_params.sh
├── MC_eighth.slurm
├── MC_quarter.slurm
├── run_all.sh
├── visualize_eighth.slurm
├── visualize_quarter.slurm
└── README.md

```

---

## About This Repository

- **Purpose:** Showcase my ability to design and implement large-scale scientific computing workflows for HPC environments.
- **Limitations:** Numerical solvers are redacted; scripts are **fundamental templates** that require the original Fortran/Python source files to execute.
- **For Recruiters:** The structure and documentation reflect real-world HPC project practices; the full, runnable version can be provided upon request.

---

## License

This project is open-source under the MIT License. See `LICENSE` for details.

---

## Contact

**Maddie Preston**  
📫 [linkedin.com/in/madeline-preston](https://www.linkedin.com/in/madeline-preston)  
📄 Resume available upon request


